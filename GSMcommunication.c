// Функции для передачи и приема данных по интерфейсу GSM
#include <stdio.h>
#include "stm32f10x_gpio.h"

#include "flash.h"
#include "REG74HC165.h"
#include "SIM800.h"
#include "GSMcommunication.h"

const char SAVE_TEL_CMD[] = "tel";

const char SAVE_ALARM_T1_CMD[] = "vhod text1 ";

const char SAVE_ALARM_T2_CMD[] = "vhod text2 ";

const char SAVE_ACT_STATE_CMD[] = "akt sost vhod";

// Глобальная структура описывает текущее состояние комуникационного GSM интерфейса
struct GSM_communication_state{
    uint8_t Status_of_mailing;                 // статус рассылки SMS сообщений: занят (busy)- рассылка сообщения идет,
    //                  свободен (free) - рассылка сообщения закончена,
    uint8_t Status_of_readSMS;                 // статус чтения SMS сообщений: занят (busy)- чтение сообщения идет,
    //                  свободен (free) - чтение сообщения закончена,
    uint8_t status_mes_send;                   // флаг статуса отправки SMS может принимать SMS_send_stop = 0 или SMS_send_start = 1
    uint8_t status_mes_rec;                    // флаг статуса приема SMS может принимать SMS_rec_stop = 0 или SMS_rec_start = 1
    uint8_t status_mes_del;                    // флаг статуса удаления SMS может принимать SMS_del_stop = 0 или SMS_del_start = 1
    uint8_t current_abonent;                   // тикущий номер абонента
    uint8_t max_num_of_abonent;                // максимальное число абонентов для рассылки
    uint8_t num_of_read_SMS;                   // число SMS, которые будут вычитываться из памяти SIM-карты
    uint8_t current_read_SMS;                  // текущее читаемое SMS сообщение
    uint8_t send_SMS_text[SEND_SMS_DATA_SIZE]; // текущий текс SMS для отправки
    //uint8_t rec_SMS_text[REC_SMS_DATA_SIZE];   // текущий текс принятого SMS //!!! Данный буфер пока не нужен, вполне хватит приемного буфера драйвера SIM800 (или другого модуля)
    uint8_t phone_num[MAX_SIZE_STR_PHONE_8];   // номер телефона текущего абонента
    volatile uint8_t number_of_failures;                // счетчик неудачных попыток (используется для защиты от зацикливания)
};

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
    GSM_com_state.num_of_read_SMS = 5;
    GSM_com_state.current_read_SMS = 1;
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
		if (GSM_com_state.current_read_SMS == GSM_com_state.num_of_read_SMS + 1)
		{
			GSM_com_state.status_mes_del = SMS_del_start;
			sim800_ATplusCMGD_request(&state_of_sim800_num1, 1, 4); // все SMS прочитаны их можно удалить
			GSM_com_state.current_read_SMS = 1;
			return;
		}
        GSM_com_state.status_mes_rec = SMS_rec_start; // отправили запрос на чтение SMS
        sim800_ATplusCMGR_request(&state_of_sim800_num1, GSM_com_state.current_read_SMS, 0);
        return;
	}

    GSM_com_state.status_mes_rec = SMS_rec_stop;
    if (state_of_sim800_num1.result_of_last_execution == OK)
    {
        // парсим принятое SMS сообщение
    	SMS_parse();
    	GSM_com_state.current_read_SMS++; // следующее SMS
    }
    if (state_of_sim800_num1.result_of_last_execution == fail) // если неудача, то пробуем еще MAX_NUM_OF_FAIL - 1 раз и бросаем это занятие
    {
    	GSM_com_state.number_of_failures ++;
    	if (GSM_com_state.number_of_failures >= MAX_NUM_OF_FAIL)
    	{
    		GSM_com_state.number_of_failures = 0;
    		GSM_com_state.current_read_SMS++; // следующее SMS
    	}
    }
    return;
}

// функция проверки состояния цифровых входов и рассылки сообщений
void Dig_Signals_Check(void)
{
	uint8_t i;
	uint8_t temp_str[MAX_SIZE_STRING_8];

	GSM_com_state.send_SMS_text[0]='\0';

    for (i = 0; i < NUM_OF_INPUT; i++)
   	{

    	if ((reg74hc165_current_state_num1.arr_res[i].status.bf.is_meander == 1) &&  // если на одном из входов появился меандр или одиночный импульс
            (reg74hc165_current_state_num1.arr_res[i].status.bf.meandr_already_sent == 0)) // и об этом еще не разослано SMS-сообщение
	    {
			if (i < NUM_OF_INPUT_SIGNAL)  // обязательно проверяем есть ли у данного входного сигнала строка во флеш
   		    {
   		        FLASH_Read_Msg_String(i, 1, temp_str, MAX_SIZE_STRING_8);
   		    }
   		    if (strlen(GSM_com_state.send_SMS_text) + strlen(temp_str) + 5 < SEND_SMS_DATA_SIZE) // 5 - это 2 пробела в конце + 1 нулевой символ + 2-а прозапас
   		    {
       		    reg74hc165_current_state_num1.arr_res[i].status.bf.meandr_already_sent = 1;   // помечаем соответсвующий выход как отправленный на рассылку
       		    // что бы SMS разослалось один раз
       		    strcat(GSM_com_state.send_SMS_text, temp_str);
       		    strncat(GSM_com_state.send_SMS_text, "  ", 3);
   		    }
   	    }

		if ((reg74hc165_current_state_num1.arr_res[i].status.bf.is_const_sig == 1) &&  // если на одном из входов появился постоянный уровень
            (reg74hc165_current_state_num1.arr_res[i].status.bf.const_already_sent == 0)) // и об этом еще не разослано SMS-сообщение
	        {
			if (i < NUM_OF_INPUT_SIGNAL)
		    {
		        FLASH_Read_Msg_String(i, 0, temp_str, MAX_SIZE_STRING_8);
		    }
		    if (strlen(GSM_com_state.send_SMS_text) + strlen(temp_str) + 5 < SEND_SMS_DATA_SIZE)
		    {
    		    reg74hc165_current_state_num1.arr_res[i].status.bf.const_already_sent = 1;
    		    // что бы SMS разослалось один раз
    		    strcat(GSM_com_state.send_SMS_text, temp_str);
    		    strncat(GSM_com_state.send_SMS_text, "  ", 3);
		    }
	    }
    }
}

// главная коммуникационная функция GSM
// может вызываться из обработчика прерывания (например таймера)
// или из одного из потоков операционной системы (например FreeRTOS, но не проверял пока)
void GSM_Communication_routine(void)
{
    // проверяем дискретные входы
    static uint8_t cur_dig_input;
    static GSM_counter; // счетчик задержки работы коммуникационной функции GSM

    GSM_counter++;
    if (GSM_counter < 300)
    {
        return;
    }
    GSM_counter = 0;

    if (state_of_sim800_num1.Status == not_ready)
    {
        return;
    }

    if (GSM_com_state.Status_of_readSMS == busy) // если есть непрочитанные SMS
    {
    	recSMS();
    	return;
    }

    if (GSM_com_state.Status_of_mailing == busy) // если есть неразосланные SMS
    {
        sendSMS();
        return;
    }

    // проверяем цифровые входы
    Dig_Signals_Check();

    // проверяем входы АЦП
    ;

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

// функция поиска в строке текста SMS телефонного номера абонета и его запись во флеш (телефонную книгу)
void Phone_Num_Save()
{
	// SMS с текстом: "telN:telnumber", где N - порядковый номер абонента (1,2,3,...), telnumber - его телефон
	int abonent_num;
	int start_pos_num; // первый символ телефонного номера абонента
	uint8_t phone_num[16];

	abonent_num = atoi(state_of_sim800_num1.rec_SMS_data + (sizeof(SAVE_TEL_CMD)-1)) - 1;

	if ((abonent_num < 0) || (abonent_num >= NUM_OF_ABONENTS)) // если пользователь ввел некорректный номер абонента
	{
		return;
	}

	start_pos_num = strchr(state_of_sim800_num1.rec_SMS_data, ':') + 1; // сразу после двоеточия идет номер телефона абонента
	memcpy(phone_num, start_pos_num, 16); // Функция FLASH_Write_Phone_Num и FLASH_Write_Msg_String - работают довольно медленно,
	// а в это время может прийдти новая sms, по-этому копируем сохраняемый текст
	FLASH_Write_Phone_Num(abonent_num, phone_num, strlen(phone_num) + 1);
    return;
}

// функция поиска в строке текста SMS тревожного сообщения выдаваемого при периодической активности на цифровом входе (наличии меандра)
// и его запись во флеш
Alarm_Text1_Save()
{
	// SMS с текстом: "vhod text1 N:text", где N - порядковый номер входа (1,2,3,...), text - текст подлежащий сохранению
	int msg_num;
	int start_pos_msg; // первый символ записываемого сообщения
	uint8_t message[16];

	msg_num = atoi(state_of_sim800_num1.rec_SMS_data + (sizeof(SAVE_ALARM_T1_CMD)-1)) - 1;

	if ((msg_num < 0) || (msg_num >= NUM_OF_INPUT_SIGNAL)) // если пользователь ввел некорректный номер входа
	{
		return;
	}

	start_pos_msg = strchr(state_of_sim800_num1.rec_SMS_data, ':') + 1; // сразу после двоеточия идет текст сохраняемого сообщения
	memcpy(message, start_pos_msg, strlen(start_pos_msg) + 1);

	FLASH_Write_Msg_String(msg_num, 1, message, strlen(message) + 1);
    return;
}

// функция поиска в строке текста SMS тревожного сообщения выдаваемого при непрерывной активности на цифровом входе (длительный постоянный уровень)
// и его запись во флеш
Alarm_Text2_Save()
{
	// SMS с текстом: "vhod text2 N:text", где N - порядковый номер входа (1,2,3,...), text - текст подлежащий сохранению
	int msg_num;
	int start_pos_msg; // первый символ записываемого сообщения
	uint8_t message[16];

	msg_num = atoi(state_of_sim800_num1.rec_SMS_data + (sizeof(SAVE_ALARM_T2_CMD)-1)) - 1;

	if ((msg_num < 0) || (msg_num >= NUM_OF_INPUT_SIGNAL)) // если пользователь ввел некорректный номер входа
	{
		return;
	}

	start_pos_msg = strchr(state_of_sim800_num1.rec_SMS_data, ':') + 1; // сразу после двоеточия идет текст сохраняемого сообщения
	memcpy(message, start_pos_msg, strlen(start_pos_msg) + 1);

	FLASH_Write_Msg_String(msg_num, 0, message, strlen(message) + 1);
    return;
}

// функция поиска в строке текста SMS информации о том что считать активным состоянием на входе сообщения выдаваемого при непрерывной активности на цифровом входе (длительный постоянный уровень)
// и его запись во флеш
Alarm_State_Save()
{
	// SMS с текстом: "akt sost vhod:N1.S:N3-N5.S:...:NM.S", где Ni - порядковый номер входа (1,2,3,...), S - состояние, которое будет считаться активным: 1 - +питания, 0 - gnd
	// пользователь может задать номера входов через дефис (через диапазон), в этом случае признак состояния, который следует после точки, будет применен ко всем этим входам
	// если же пользователь ввел только один номер и следом точку и потом признак, то start_input_num будет равно stop_input_num
	uint8_t start_input_num; // начальный номер входа
	uint8_t stop_input_num;  // конечный номер входа
	uint8_t current_str_pos; // текущий проверяемый символ сообщения
	uint8_t alarm_state;     // сохраняемой конфигурация активного состояния
	uint8_t message[16];
	uint8_t i = 0;


	current_str_pos = state_of_sim800_num1.rec_SMS_data;

	do
	{
		current_str_pos = strchr(current_str_pos, ':');

		if (current_str_pos == NULL) return; // если пользователь вообще не ввел номеров входов выходим

		current_str_pos++; // сразу после двоеточия идет номер входа

		start_input_num = atoi(current_str_pos) - 1;

		// проверим следующие символы на наличие дефиса (число максимально может быть двузначным, так что следующий символ и сразу за ним)
		if ((state_of_sim800_num1.rec_SMS_data[current_str_pos + 1] == '-') || (state_of_sim800_num1.rec_SMS_data[current_str_pos + 2] == '-'))
		{
			current_str_pos = strchr(current_str_pos, '-') + 1; // сразу после тире идет финальный номер входа в диапазоне номеров входов
			stop_input_num = atoi(current_str_pos) - 1;
		}
		else
		{
			stop_input_num = start_input_num;
		}

		if (!(start_input_num < 0) || (start_input_num >= NUM_OF_INPUT_SIGNAL)) continue; // если пользователь ввел некорректный номер входа
		if ( !(stop_input_num < 0) || (stop_input_num >= NUM_OF_INPUT_SIGNAL))  continue;
		if (stop_input_num < start_input_num) stop_input_num = start_input_num; // если пользователь ввел некорректный диапазон входов

		current_str_pos = strchr(current_str_pos, '.') + 1; // сразу после точки идет признак активного состояния входа (1 - + питания / 0 -gnd)
		alarm_state = atoi(current_str_pos) - 1;

		if ((alarm_state != 0) && (alarm_state != 1)) continue;// если пользователь ввел некорректное активное состояние входа

		for (i = start_input_num; i <= stop_input_num; i++)
		{
			reg74hc165_current_state_num1.arr_res[i].config.bf.alarm_state = alarm_state;
		}
		save_config_74HC165(&reg74hc165_current_state_num1); // сохраняем конфигурацию во флеш
	}
	while (current_str_pos);

    return;
}

// Функция парсинга приходящих SMS - сообщений
void SMS_parse(void)
{
    uint8_t start;

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

	if (stristr(state_of_sim800_num1.rec_SMS_data, SAVE_TEL_CMD))
	// если пользователь хочет ввести новый номер целевого абонента
    {
		// SMS с текстом: "telN:telnumber", где N - порядковый номер абонента (1,2,3,...), telnumber - его телефон
		Phone_Num_Save();
        return;
    }
	else if (stristr(state_of_sim800_num1.rec_SMS_data, SAVE_ALARM_T1_CMD))
	// если пользователь хочет поменять текст тревожного сообщения выдаваемого при периодической активности на цифровом входе (наличии меандра)
    {
		// SMS с текстом: "vhod text1 N:text", где N - порядковый номер входа (1,2,3,...), text - текст подлежащий сохранению
		Alarm_Text1_Save();
        return;
    }
	else if (stristr(state_of_sim800_num1.rec_SMS_data, SAVE_ALARM_T2_CMD))
	// если пользователь хочет поменять текст тревожного сообщения выдаваемого при непрерывной активности на цифровом входе (длительный постоянный уровень)
    {
		// SMS с текстом: "vhod text2 N:text", где N - порядковый номер входа (1,2,3,...), text - текст подлежащий сохранению
		Alarm_Text2_Save();
        return;
    }
	else if (stristr(state_of_sim800_num1.rec_SMS_data, SAVE_ACT_STATE_CMD))
	// если пользователь хочет поменять уровень активного состояний на входе/входах (0 или +питания) которое будет приводить к отсылке тревожного SMS - сообщения
    {
		// SMS с текстом: "akt sost vhod:N1.S:N3-N5.S:...:NM.S", где Ni - порядковый номер входа (1,2,3,...), S - состояние, которое будет считаться активным: 1 - +питания, 0 - gnd
		Alarm_State_Save();
        return;
    }
    else
    {
        return; // а это все остальные тексты SMS сообщений
    }
}
