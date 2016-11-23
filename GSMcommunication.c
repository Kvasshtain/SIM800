// Функции для передачи и приема данных по интерфейсу GSM
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stm32f10x_gpio.h"

#include "phisic.h"
#include "flash.h"
#include "REG74HC165.h"
#include "adc.h"
#include "SIM800.h"
#include "GSMcommunication.h"

// префиксы строк входящих SMS содержащих команды
const char ECHO[] = "echo";
const char SAVE_TEL_CMD[] = "tel";
const char SAVE_ALARM_T1_CMD[] = "vhod text1 ";
const char SAVE_ALARM_T2_CMD[] = "vhod text2 ";
const char SAVE_ACT_STATE_0_CMD[] = "akt 0 vhoda:";
const char SAVE_ACT_STATE_1_CMD[] = "akt 1 vhoda:";
const char SAVE_ACT_STATE_RNG_0_CMD[] = "akt 0 vhodov:";
const char SAVE_ACT_STATE_RNG_1_CMD[] = "akt 1 vhodov:";

// тексты SMS отправляемые при пренижении/превышении порогов контролируемых напряжений
const char DEC_TH_EXT_BAT_MSG[] = "No 220V";
const char INC_TH_EXT_BAT_MSG[] = "Yes 220V";
const char DEC_MAIN_VOLT_MSG[]  = "No extbat";
const char INC_MAIN_VOLT_MSG[]  = "Yes extbat";
const char DEC_TH_BACKUP_MSG[]  = "No backup";
const char INC_TH_BACKUP_MSG[]  = "Yes backup";
const char DEC_TH_BAT_MSG[]     = "No intbat";
const char INC_TH_BAT_MSG[]     = "Yes intbat";

const char* POWER_DOWN_MSG[NUM_OF_ADC_CHANNEL] = {DEC_MAIN_VOLT_MSG, DEC_TH_EXT_BAT_MSG, DEC_TH_BACKUP_MSG, DEC_TH_BAT_MSG};
const char* POWER_UP_MSG[NUM_OF_ADC_CHANNEL]   = {INC_MAIN_VOLT_MSG, INC_TH_EXT_BAT_MSG, INC_TH_BACKUP_MSG, INC_TH_BAT_MSG};

struct GSM_communication_state GSM_com_state; // структура хранящая текущее состояние коммуникационного GSM итерфеса

// функция инициализации коммуникационного интерфейса
void GSM_Com_Init(struct sim800_current_state * GSMmod)
{
    GSM_com_state.send_SMS_text[0] = '\0';
    GSM_com_state.status_mes_send = SMS_send_stop;
    GSM_com_state.status_mes_rec  = SMS_rec_stop;
    GSM_com_state.status_mes_rec  = SMS_del_stop;
    GSM_com_state.phone_num[0] = '\0';
    GSM_com_state.Status_of_mailing = free;
    GSM_com_state.Status_of_readSMS = free;
    GSM_com_state.current_abonent = 0;
    GSM_com_state.max_num_of_abonent = NUM_OF_ABONENTS; // можно его ограничить числом меньшим NUM_OF_ABONENTS
    GSM_com_state.number_of_failures = 0;
}

// Функция отправки SMS
// Функция пытается отправить SMS используя соответсвующие низкоуровневые функции
void sendSMS(void)
{
    if (state_of_sim800_num1.communication_stage != proc_completed)     // если GSM-модуль занят обработкой предыдущего запроса
    {
        return;
    }

    if (GSM_com_state.status_mes_send == SMS_send_stop)  // мы еще не начали отсылать SMS
    {
        do
        {
            FLASH_Read_Phone_Num(GSM_com_state.current_abonent, GSM_com_state.phone_num, MAX_SIZE_STR_PHONE_8);
            GSM_com_state.current_abonent++;
            if (GSM_com_state.current_abonent == GSM_com_state.max_num_of_abonent) // если пробежались по всем ячейкам флеш памяти и телефонных номеров больше нет
            {
                GSM_com_state.current_abonent = 0;
                GSM_com_state.Status_of_mailing = free;
                GSM_com_state.send_SMS_text[0] = '\0'; // стираем SMS
                GSM_com_state.number_of_failures = 0;  // сбрасываем счетчик неудачных попыток
                return;
            }
        }
        while (GSM_com_state.phone_num[0] == 0xFF);
        GSM_com_state.current_abonent--;

        GSM_com_state.status_mes_send = SMS_send_start; // отправили запрос на отправку SMS
        sim800_ATplusCMGS_request(&state_of_sim800_num1, GSM_com_state.phone_num, GSM_com_state.send_SMS_text);
        return;
    }

    GSM_com_state.status_mes_send = SMS_send_stop;
    if (state_of_sim800_num1.result_of_last_execution == OK)
    {
        GSM_com_state.current_abonent++; // следующий абонент
    }
    if (state_of_sim800_num1.result_of_last_execution == fail) // если неудача, то пробуем еще MAX_NUM_OF_FAIL - 1 раз и бросаем это занятие
    {
        GSM_com_state.number_of_failures ++;
        if (GSM_com_state.number_of_failures >= MAX_NUM_OF_FAIL)
        {
            GSM_com_state.number_of_failures = 0;
            GSM_com_state.current_abonent++; // следующий абонент
        }
    }
    return;
}

// Функция обработки принятых SMS
void recSMS(void)
{
    if (state_of_sim800_num1.communication_stage != proc_completed)     // если GSM-модуль занят обработкой предыдущего запроса
    {
        return;
    }

    if (GSM_com_state.status_mes_del == SMS_del_start) // если был отправлен запрос на удаление SMS
    {
        if (state_of_sim800_num1.result_of_last_execution == OK)
        {
            GSM_com_state.status_mes_del = SMS_del_stop;
            GSM_com_state.status_mes_rec = SMS_rec_stop;
            GSM_com_state.Status_of_readSMS = free;    // процесс чтения и последующего удаления SMS завершон
            GSM_com_state.number_of_failures = 0;      // сбрасываем счетчик неудачных попыток
            return;
        }
        else
        {
            sim800_ATplusCMGD_request(&state_of_sim800_num1, 1, 4); // пробуем удалить SMS еще раз
        }
    }

    if (GSM_com_state.status_mes_rec == SMS_rec_stop) // мы еще не начали читать SMS
    {
        GSM_com_state.status_mes_rec = SMS_rec_start; // отправили запрос на чтение SMS
        sim800_ATplusCMGR_request(&state_of_sim800_num1, 1, 0);
        return;
    }

    GSM_com_state.status_mes_rec = SMS_rec_stop;
    if (state_of_sim800_num1.result_of_last_execution == OK)
    {
        // парсим принятое SMS сообщение
        SMS_parse();
        GSM_com_state.status_mes_del = SMS_del_start;
        sim800_ATplusCMGD_request(&state_of_sim800_num1, 1, 4); // все SMS прочитаны их можно удалить
        return;

    }
    else
    {
        GSM_com_state.status_mes_del = SMS_del_start;
        sim800_ATplusCMGD_request(&state_of_sim800_num1, 1, 4); // удаляем нечитаемые SMS
        return;
    }
    return;
}

// функция проверки состояния цифровых входов и рассылки сообщений
void Dig_Signals_Check(void)
{
    uint8_t i;
    uint8_t temp_str[MAX_SIZE_STRING_8];

    for (i = 0; i < NUM_OF_INPUT; i++)
    {

        if ((reg74hc165_current_state_num1.arr_res[i].status.bf.is_meander == 1) &&  // если на одном из входов появился меандр или одиночный импульс
                (reg74hc165_current_state_num1.arr_res[i].status.bf.meandr_already_sent == 0)) // и об этом еще не разослано SMS-сообщение
        {
            if (i < NUM_OF_INPUT_SIGNAL)  // обязательно проверяем есть ли у данного входного сигнала строка во флеш
            {
                FLASH_Read_Msg_String(i, 1, temp_str, MAX_SIZE_STRING_8);
            }
            if (strlen((char*)GSM_com_state.send_SMS_text) + strlen((char*)temp_str) + 5 < SEND_SMS_DATA_SIZE) // 5 - это 2 пробела в конце + 1 нулевой символ + 2-а прозапас
            {
                reg74hc165_current_state_num1.arr_res[i].status.bf.meandr_already_sent = 1;   // помечаем соответсвующий вход как отправленный на рассылку
                // что бы SMS разослалось один раз
                strcat((char*)GSM_com_state.send_SMS_text, (char*)temp_str);
                strncat((char*)GSM_com_state.send_SMS_text, ". ", 3);
            }
        }

        if ((reg74hc165_current_state_num1.arr_res[i].status.bf.is_const_sig == 1) &&  // если на одном из входов появился постоянный уровень
                (reg74hc165_current_state_num1.arr_res[i].status.bf.const_already_sent == 0)) // и об этом еще не разослано SMS-сообщение
        {
            if (i < NUM_OF_INPUT_SIGNAL)
            {
                FLASH_Read_Msg_String(i, 0, temp_str, MAX_SIZE_STRING_8);
            }
            if (strlen((char*)GSM_com_state.send_SMS_text) + strlen((char*)temp_str) + 5 < SEND_SMS_DATA_SIZE)
            {
                reg74hc165_current_state_num1.arr_res[i].status.bf.const_already_sent = 1;
                // что бы SMS разослалось один раз
                strcat((char*)GSM_com_state.send_SMS_text, (char*)temp_str);
                strncat((char*)GSM_com_state.send_SMS_text, ". ", 3);
            }
        }
    }
}

// функция проверки состояния аналоговых входов и рассылки сообщений
void Analog_Signals_Check(void)
{
    uint8_t i;

    for (i = 0; i < NUM_OF_ADC_CHANNEL; i++)
        //for (i = 0; i < 3; i++)
    {
        if ((ADC_current_state_num1.result[i].analog_ch_status.is_normal == 1) &&          // если напряжение выше порога
                (ADC_current_state_num1.result[i].analog_ch_status.incr_th_already_sent == 0)) // и об этом еще не разослано SMS-сообщение
        {
            if (strlen((char*)GSM_com_state.send_SMS_text) + strlen(POWER_UP_MSG[i]) + 5 < SEND_SMS_DATA_SIZE) // 5 - это 2 пробела в конце + 1 нулевой символ + 2 прозапас
            {
                ADC_current_state_num1.result[i].analog_ch_status.incr_th_already_sent = 1;   // помечаем соответсвующий вход как отправленный на рассылку
                // что бы SMS разослалось один раз
                strcat((char*)GSM_com_state.send_SMS_text, POWER_UP_MSG[i]);
                strncat((char*)GSM_com_state.send_SMS_text, ". ", 3);
            }
        }

        if ((ADC_current_state_num1.result[i].analog_ch_status.is_normal == 0) &&          // если напряжение ниже порога
                (ADC_current_state_num1.result[i].analog_ch_status.decr_th_already_sent == 0)) // и об этом еще не разослано SMS-сообщение
        {
            if (strlen((char*)GSM_com_state.send_SMS_text) + strlen(POWER_DOWN_MSG[i]) + 5 < SEND_SMS_DATA_SIZE) // 5 - это 2 пробела в конце + 1 нулевой символ + 2 прозапас
            {
                ADC_current_state_num1.result[i].analog_ch_status.decr_th_already_sent = 1;   // помечаем соответсвующий вход как отправленный на рассылку
                // что бы SMS разослалось один раз
                strcat((char*)GSM_com_state.send_SMS_text, POWER_DOWN_MSG[i]);
                strncat((char*)GSM_com_state.send_SMS_text, ". ", 3);
            }
        }
    }
}

// периодическая проверка SIM800 на жизнеспособность
void Is_SIM800_alive(void)
{
    uint32_t count = 0;
    uint8_t result = OK;
    static uint16_t dalay_count;

    dalay_count ++;

    if (dalay_count <= 1000)
    {
        return;
    }

    dalay_count = 0;

    sim800_AT_request(&state_of_sim800_num1); // пробуем отправить тестовую команду
    while (state_of_sim800_num1.communication_stage != proc_completed) // ждем пока не ответит OK
    {
        count ++;
        if (count == REQ_TIMEOUT)
        {
            result = ATfail;
        }
    }
    if (state_of_sim800_num1.result_of_last_execution == fail)
    {
        result = ATfail;
    }
    if (result == ATfail)
    {
        SysReset(); // если модуль не отвечает пробуем перезагрузится
    }
}

// проверка состояния связи по GSM каналу (просто проверяется колличество ошибок при передачи сообщений и других действиях)
void Communication_check(void)
{
	if (state_of_sim800_num1.num_of_fail <=2)
	{
		return;
	}

	state_of_sim800_num1.num_of_fail = 0; // если колличество ошибок превысило порог в 2, то пробуем переключится на второую SIM-карту

	PWR->CR |=  PWR_CR_DBP;                 //разрешить запись в область BKP
	if (state_of_sim800_num1.current_SIM_card == 1) // пробуем перключить SIM-карту
    {
		BKP->DR1 =  2;                        //сохранить данные о рабочей SIM-карте
    }
    else
    {
    	BKP->DR1 =  1;                        //сохранить данные о рабочей SIM-карте
    }
	PWR->CR &= ~PWR_CR_DBP;                 //запретить запись в область BKP
	SysReset(); // пробуем перезапуститься
}

// главная коммуникационная функция GSM
// может вызываться из обработчика прерывания (например таймера)
// или из одного из потоков операционной системы (например FreeRTOS, но не проверял пока)
void GSM_Communication_routine(void)
{
    static uint16_t GSM_counter; // счетчик задержки работы коммуникационной функции GSM

    GSM_counter++;
    if (GSM_counter < 400)
    {
        return;
    }
    GSM_counter = 0;

    if (state_of_sim800_num1.Status == not_ready)
    {
        return;
    }

    if (GSM_com_state.Status_of_mailing == busy) // если есть неразосланные SMS
    {
        sendSMS();
        return;
    }

    if (GSM_com_state.Status_of_readSMS == busy) // если есть непрочитанные SMS
    {
        recSMS();
        return;
    }

    GPIOA->ODR ^= GPIO_Pin_0; // светодиод индикации работы

    // проверяем цифровые входы
    Dig_Signals_Check();

    // проверяем входы АЦП
    Analog_Signals_Check();

    // SIM800 живой?
    Is_SIM800_alive();

    // Проверка связи
    //Communication_check();

    if (GSM_com_state.send_SMS_text[0] != '\0') // если есть что рассылать
    {
        GSM_com_state.Status_of_mailing = busy;
        return;
    }

    if (state_of_sim800_num1.num_of_sms) // если есть что прочитать
    {
        GSM_com_state.Status_of_readSMS = busy;
        return;
    }
}

// функция отправки эхо (что GSM извещатель находится в рабочем состоянии)
void Echo(void)
{
    strcat((char*)GSM_com_state.send_SMS_text, ECHO); // отправляем в ответ эхо
}

// функция поиска в строке текста SMS телефонного номера абонета и его запись во флеш (телефонную книгу)
// функция поиска в строке текста SMS телефонного номера абонета и его запись во флеш (телефонную книгу)
void Save_Phone_Num(void)
{
    // SMS с текстом: "telN:telnumber", где N - порядковый номер абонента (1,2,3,...), telnumber - его телефон
    uint8_t * start_pos_num; // адрес первого символа телефонного номера абонента
    uint8_t phone_num_len;
    uint8_t abonent_num;

    abonent_num = atoi((char*)state_of_sim800_num1.rec_SMS_data + (sizeof(SAVE_TEL_CMD)-1)) - 1;

    if ((abonent_num < 0) || (abonent_num >= NUM_OF_ABONENTS)) // если пользователь ввел некорректный номер абонента
    {
        return;
    }

    start_pos_num = (uint8_t *)strchr((char*)state_of_sim800_num1.rec_SMS_data, ':'); // сразу после двоеточия идет номер телефона абонента

    if (start_pos_num == NULL) // если пользователь забыл ввести ':'
    {
        return;
    }

    start_pos_num++;

    phone_num_len = strlen((char*)start_pos_num);

    if (phone_num_len >= MAX_SIZE_STR_PHONE_8) // если пользователь ввел слишком длинный телефонный номер
    {
        return;
    }

    // Функция FLASH_Write_Phone_Num и FLASH_Write_Msg_String - работают довольно медленно,
    // а в это время может прийдти новая sms, по-этому копируем сохраняемый текст
    memcpy(Flash_routine_state.phone_num, start_pos_num, phone_num_len);
    Flash_routine_state.phone_len = phone_num_len;
    Flash_routine_state.abonent_num = abonent_num;
    Flash_routine_state.need_write.phone = 1;

    return;
}

// функция поиска в строке текста SMS тревожного сообщения выдаваемого при периодической активности на цифровом входе (наличии меандра)
// и его запись во флеш
// принимает тип сообщения (1 или 2)
void Save_Alarm_Text(uint8_t type)
{
    // SMS с текстом: "vhod text1(2) N:text", где N - порядковый номер входа (1,2,3,...), text - текст подлежащий сохранению
    int8_t msg_num;
    uint8_t * start_pos_msg; // первый символ записываемого сообщения
    uint8_t text_len;

    if ((type != 1) && (type != 2))
    {
        return;
    }

    if (type == 1)
    {
        msg_num = atoi((char*)state_of_sim800_num1.rec_SMS_data + (sizeof(SAVE_ALARM_T1_CMD)-1)) - 1;
    }
    else if (type == 2)
    {
        msg_num = atoi((char*)state_of_sim800_num1.rec_SMS_data + (sizeof(SAVE_ALARM_T2_CMD)-1)) - 1;
    }

    if ((msg_num < 0) || (msg_num >= NUM_OF_INPUT_SIGNAL)) // если пользователь ввел некорректный номер входа
    {
        return;
    }

    start_pos_msg = (uint8_t *)strchr((char*)state_of_sim800_num1.rec_SMS_data, ':'); // сразу после двоеточия идет текст сохраняемого сообщения

    if (start_pos_msg == NULL) // если пользователь забыл ввести ':'
    {
        return;
    }

    start_pos_msg++;

    text_len = strlen((char*)start_pos_msg);

    if (text_len >= MAX_SIZE_STRING_8) // если пользователь ввел слишком длинный текст сообщения
    {
        return;
    }

    Flash_routine_state.text_len = text_len;
    Flash_routine_state.msg_num = msg_num;

    if (type == 1)
    {
        memcpy(Flash_routine_state.Text1, start_pos_msg, text_len);
        Flash_routine_state.need_write.alarm_text1 = 1;
    }
    else if (type == 2)
    {
        memcpy(Flash_routine_state.Text2, start_pos_msg, text_len);
        Flash_routine_state.need_write.alarm_text2 = 1;
    }

    return;
}

// функция сохраниения во флеш для указанных в SMS входов нулевого активного состояния
// принимает признак активного состояния (0 или 1)
void Save_Alarm_State(uint8_t state)
{
    // SMS с текстом: "akt sost 0(1) vhoda:N1,...,Ni", где Ni - порядковый номер входа (1,2,3,...), для которых активным состоянием будет 0(или 1).
    uint8_t input_num;   // номер входа
    uint8_t * input_num_pos;   // позиция символа - номера входа в сообщении

    input_num_pos = state_of_sim800_num1.rec_SMS_data;

    input_num_pos = (uint8_t *)strchr((char*)input_num_pos, ':'); // сразу после двоеточия идет текст сохраняемого сообщения

    if (input_num_pos == NULL) // если пользователь забыл ввести ':'
    {
        return;
    }

    input_num_pos++;

    input_num = atoi((char*)input_num_pos) - 1; // вычитываем вход

    if ((input_num >= 0) && (input_num < NUM_OF_INPUT)) // принимаем изменнеия только для корректных номеров входов
    {
        reg74hc165_current_state_num1.arr_res[input_num].config.bf.alarm_state = state;
    }

    input_num_pos = (uint8_t *)strchr((char*)input_num_pos, ',');

    while(input_num_pos) // если в строке встретилось или ','
    {
        input_num_pos++;  // значит следующие символы - это номер входа
        input_num = atoi((char*)input_num_pos) - 1; // вычитываем вход
        if ((input_num >= 0) && (input_num < NUM_OF_INPUT)) // принимаем изменнеия только для корректных номеров входов
        {
            reg74hc165_current_state_num1.arr_res[input_num].config.bf.alarm_state = state;
        }
        input_num_pos = (uint8_t *)strchr((char*)input_num_pos, ',');
    }

    Flash_routine_state.need_write.alarm_state = 1;

    return;
}

// функция сохраниения во флеш для указанных в SMS входов нулевого активного состояния (введенных через дефис)
// принимает признак активного состояния (0 или 1)
void Save_range_Alarm_State(uint8_t state)
{
    // SMS с текстом: "akt sost 0(1) vhodov:Ni-Nj", где Ni - Nj - диапазон номеров входов (напимер 2-5), для которых активным состоянием будет 0(или 1).
    uint8_t i;
    int8_t start_input_num;   // начальный номер входа
    int8_t stop_input_num;    // конечный номер входа
    uint8_t * input_num_pos;   // позиция символа - номера входа в сообщении

    input_num_pos = state_of_sim800_num1.rec_SMS_data;

    input_num_pos = (uint8_t *)strchr((char*)input_num_pos, ':'); // сразу после двоеточия идет текст сохраняемого сообщения

    if (input_num_pos == NULL) // если пользователь забыл ввести ':'
    {
        return;
    }

    input_num_pos++;

    start_input_num = atoi((char*)input_num_pos) - 1; // начальный вычитываем вход

    if((start_input_num < 0) && (start_input_num >= NUM_OF_INPUT)) // если пользователь ввел некорректный номер входа
    {
        return;
    }

    input_num_pos = (uint8_t *)strchr((char*)input_num_pos, '-'); // сразу после двоеточия идет текст сохраняемого сообщения

    if (input_num_pos == NULL) // если пользователь забыл ввести '-'
    {
        return;
    }

    input_num_pos++;

    stop_input_num = atoi((char*)input_num_pos) - 1; // конечный вычитываем вход

    if((stop_input_num < 0) && (stop_input_num >= NUM_OF_INPUT)) // если пользователь ввел некорректный номер входа
    {
        return;
    }

    if (start_input_num > stop_input_num) // если пользователь ввел некорректный диапазон
    {
        return;
    }

    for (i = start_input_num; i <= stop_input_num; i++)
    {
        reg74hc165_current_state_num1.arr_res[i].config.bf.alarm_state = state;
    }

    Flash_routine_state.need_write.alarm_state = 1;

    return;
}

// Функция парсинга приходящих SMS - сообщений
void SMS_parse(void)
{
    //	// Пример управления светодиодом с помощью SMS
    //	if (strstr(state_of_sim800_num1.rec_SMS_data,"LED ON"))
    //    {
    //        GPIOA->ODR &= ~GPIO_Pin_0;
    //        return;
    //    }
    //	else if (strstr(state_of_sim800_num1.rec_SMS_data,"LED OFF"))
    //    {
    //        GPIOA->ODR |=  GPIO_Pin_0;
    //        return;
    //    }

	// команда ЭХО для проверки связи
    if (stristr(state_of_sim800_num1.rec_SMS_data, (uint8_t *)ECHO))
    {
        Echo();
        return;
    }

    //*****************************************************************
    // !!!Ниже идет парсинг конфигурационных SMS, выше пользовательских
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_15)) // опрашиваем DIP - переключатель разрешения конфигурирования по SMS.
    {
        return; // если DIP - перключатель в нижнем положении дальнейший парсинг не происходит
    }

    if (stristr(state_of_sim800_num1.rec_SMS_data, (uint8_t *)SAVE_TEL_CMD))
        // если пользователь хочет ввести новый номер целевого абонента
    {
        // SMS с текстом: "telN:telnumber", где N - порядковый номер абонента (1,2,3,...), telnumber - его телефон
        Save_Phone_Num();
        return;
    }
    else if (stristr(state_of_sim800_num1.rec_SMS_data, (uint8_t *)SAVE_ALARM_T1_CMD))
        // если пользователь хочет поменять текст тревожного сообщения выдаваемого при периодической активности на цифровом входе (наличии меандра)
    {
        // SMS с текстом: "vhod text1 N:text", где N - порядковый номер входа (1,2,3,...), text - текст подлежащий сохранению
        Save_Alarm_Text(1);
        return;
    }
    else if (stristr(state_of_sim800_num1.rec_SMS_data, (uint8_t *)SAVE_ALARM_T2_CMD))
        // если пользователь хочет поменять текст тревожного сообщения выдаваемого при непрерывной активности на цифровом входе (длительный постоянный уровень)
    {
        // SMS с текстом: "vhod text2 N:text", где N - порядковый номер входа (1,2,3,...), text - текст подлежащий сохранению
        Save_Alarm_Text(2);
        return;
    }
    else if (stristr(state_of_sim800_num1.rec_SMS_data, (uint8_t *)SAVE_ACT_STATE_0_CMD))
        // если пользователь хочет поменять уровень активного состояний на входе на нулевой
    {
        Save_Alarm_State(0);
        return;
    }
    else if (stristr(state_of_sim800_num1.rec_SMS_data, (uint8_t *)SAVE_ACT_STATE_1_CMD))
        // если пользователь хочет поменять уровень активного состояний на входе на +питания
    {
        Save_Alarm_State(1);
        return;
    }
    else if (stristr(state_of_sim800_num1.rec_SMS_data, (uint8_t *)SAVE_ACT_STATE_RNG_0_CMD))
        // если пользователь хочет поменять уровень активного состояний на входах (вводимых через тире) на нулевой
    {
        Save_range_Alarm_State(0);
        return;
    }
    else if (stristr(state_of_sim800_num1.rec_SMS_data, (uint8_t *)SAVE_ACT_STATE_RNG_1_CMD))
        // если пользователь хочет поменять уровень активного состояний на входах (вводимых через тире) на +питания
    {
        Save_range_Alarm_State(1);
        return;
    }
    else
    {
        return; // а это все остальные тексты SMS сообщений
    }
}
