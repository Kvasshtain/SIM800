// драйвер SIM800
// частично совместим со тарыми семействами SIM900 и SIM300
#include <stdio.h>
#include "phisic.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "SIM800.h"

struct sim800_current_state state_of_sim800_num1; // модулей может быть несколько

uint8_t * stristr (const uint8_t * str1, const uint8_t * str2) // работает только с латинскими буквами
{
    uint8_t *cp = (uint8_t *) str1;
    uint8_t *s1, *s2;

    if ( !*str2 )
        return (uint8_t *)str1;

    while (*cp)
    {
        s1 = cp;
        s2 = (uint8_t *) str2;

        while ( *s1 && *s2 && !( tolower(*s1) - tolower(*s2) ) )
                s1++, s2++;

        if (!*s2)
                return cp;

        cp++;
    }

    return 0;
}

// Функция включения первого модуля SIM800
void sim800_1_PWRKEY_on(void)
{
    volatile int i;
    // Низкий уровень на PowerKey в течении 3-4 секунд (включаем SIM800)
    PWR_KEY_DOWN;
    for(i=0;i<0x500000;i++);
    PWR_KEY_UP;
    for(i=0;i<0x500000;i++);
    //	// Подаем на порт PWRKEY лог. 0
    //    GPIO_WriteBit (GPIO_SIM800_PWRKEY, SIM800_PWRKEY, Bit_RESET);
    //    // Задержка в мс., необходимая для включения
    //    for (i = 0; i<5000; i++);
    //
    //    // Подаем на порт PWRKEY лог. 1
    //    GPIO_WriteBit (GPIO_SIM800_PWRKEY, SIM800_PWRKEY, Bit_SET);
    //    // Ожидаем включение/выключение SIM800
    //    for (i = 0; i<5000; i++);
}

// Функция отправки данных в UART на котором сидит Sim800
inline void Sim800_WriteCmd(struct sim800_current_state *current_state)
{


    current_state->send_uart_function(current_state->current_cmd);
}

// Функция отправки СМС в UART на котором сидит Sim800
inline void Sim800_WriteSMS(struct sim800_current_state *current_state)
{
    current_state->send_uart_function(current_state->send_SMS_data);
    current_state->send_uart_function("\032"); // завершаем смс сообщение отправкой ctrl+Z
}

// Функция отправки запросов в SIM800
// функция отправки запросов во-первых должна записать в структуру sim800_current_state для своего модуля SIM800 запрос который собирается отправить
// затем записать в эту же структуру указатель на обработчик ответа
// затем записать в эту же структуру признак, что запрос отправлен (resp_rec)
// и только потом отпраить запрос в UART на котором сидит SIM800
// Принимает: указатель на структуру содержащую текущее состояние конкретного модуля SIM800(их может быть несколько)
// Возвращает статус занят (попробовать отправить запрос позже), или запрос отправлен ждем ответа
int8_t sim800_request(struct sim800_current_state *current_state)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    current_state->communication_stage = resp_rec; // Выставляем флаг занятости
    Sim800_WriteCmd(current_state);



    return exit_and_wait;                         // Возвращаемся в вызвавший код сигнал ждать ответа и заниматься другими делами, запрос отправлен
}

//**********************************************************************
//**********************************************************************
//ФУНКЦИИ ОТПРАВКИ ЗАПРОСОВ И ОБРАБОТЧИКИ ОТВЕТОВ (ПОПАРНО ЗАПРОС/ОТВЕТ)

// Функция отправки запроса на автонастройку baudrate модуля SIM800 (команда "AT")
uint8_t sim800_AT_request(struct sim800_current_state * current_state)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT\r", 4);
    current_state->response_handler = sim800_AT_responce_handler;
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT"
void sim800_AT_responce_handler(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT",2)==0) // Пришло ЭХО?
    {
        int j; GPIOA->ODR &= ~GPIO_Pin_0; //for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"OK",2)==0)
    {
//        if (strncasecmp(current_state->responce,"OK",2)==0)
//        {
//        	int j; GPIOA->ODR &= ~GPIO_Pin_0;// for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
//        }
    	current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else
    {
        current_state->unex_resp_handler(current_state);
        return;
    }
}

//Функция разблокировки SIM-карты "AT+CPIN=pin-code"
// Принимает: 1) указатель на структуру состояния данного модуля SIM800
//            2) пин-код
uint8_t sim800_ATplusCPIN_request(struct sim800_current_state * current_state, uint16_t PIN_code)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT+CPIN=", 9);

    uint8_t string_of_PIN[5]; // PIN код как правило 4 цифры
    itoa(PIN_code, string_of_PIN, 10);

    strncat(current_state->current_cmd, string_of_PIN, 4);
    //    if (mode == text_mode)
    //    {
    //    	strncat(current_state->current_cmd, "1", 1);
    //    }
    //    else
    //    {
    //    	strncat(current_state->current_cmd, "0", 1);
    //	}

    strncat(current_state->current_cmd, "\r", 2);
    current_state->response_handler = sim800_ATplusCPIN_responce_handler;
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT+CPIN=pin-code"
void sim800_ATplusCPIN_responce_handler(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT+CPIN=",8)==0) // Пришло ЭХО?
    {
        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"OK",2)==0)
    {
        current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else
    {
        current_state->unex_resp_handler(current_state); // если пришел не ответ на нашу команду, а что-то еще, вызываем обработчик внезапных сообщений
        return;
    }
}

// Функция переключения SIM800 в текстовый режим (команда "AT+CMGF=1 или 0 1-включить, 0-выключить")
// Принимает: 1) указатель на структуру состояния данного модуля SIM800
//            2) режим ввода команд text_mode = 1 - включить текстовый режим, code_mode = 0 - выключить текстовый режим
// Возвращает статус выполнения
uint8_t sim800_ATplusCMGF_request(struct sim800_current_state * current_state, uint8_t mode)
{
	if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT+CMGF=", 9);

    uint8_t string_of_mode[2];
    itoa(mode, string_of_mode, 10);

    strncat(current_state->current_cmd, string_of_mode, 1);
    //    if (mode == text_mode)
    //    {
    //    	strncat(current_state->current_cmd, "1", 1);
    //    }
    //    else
    //    {
    //    	strncat(current_state->current_cmd, "0", 1);
    //	}

    strncat(current_state->current_cmd, "\r", 2);
    current_state->response_handler = sim800_ATplusCMGF_responce_handler;
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT+CMGF=0/1"
void sim800_ATplusCMGF_responce_handler(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT+CMGF=",8)==0) // Пришло ЭХО?
    {
        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"OK",2)==0)
    {
        current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else
    {
        current_state->unex_resp_handler(current_state); // если пришел не ответ на нашу команду, а что-то еще, вызываем обработчик внезапных сообщений
        return;
    }
}

// Функция отправки запроса оператора SIM-карты SIM800 (команда "AT+CSPN?")
// Принимает: 1) указатель на структуру состояния данного модуля SIM800
// Возвращает статус выполнения
uint8_t sim800_ATplusCSPNquestion_request(struct sim800_current_state * current_state)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT+CSPN?\r", 10);
    current_state->response_handler = sim800_ATplusCSPNquestion_responce_handler;
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT+CSPN?"
void sim800_ATplusCSPNquestion_responce_handler(struct sim800_current_state * current_state)
{
	if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT+CSPN?",8)==0) // Пришло ЭХО?
    {

        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"+CSPN:",6)==0) // Пришло сообщение о операторе SIM-карты?
    {
    	if      (stristr(&current_state->rec_buf[current_state->current_read_buf][6],"Beeline"))   {current_state->current_mobile_operator = Beeline;}
    	else if (stristr(&current_state->rec_buf[current_state->current_read_buf][6],"MTS"))       {current_state->current_mobile_operator = MTS;}
    	else if (stristr(&current_state->rec_buf[current_state->current_read_buf][6],"MegaPhone")) {current_state->current_mobile_operator = MegaPhone;}
    	else if (stristr(&current_state->rec_buf[current_state->current_read_buf][6],"Tele2"))     {current_state->current_mobile_operator = Tele2;}
    	else if (stristr(&current_state->rec_buf[current_state->current_read_buf][6],"Yota"))      {current_state->current_mobile_operator = Yota;}
    	else                                                                                       {current_state->current_mobile_operator = 0;}

        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"\r\nOK",4)==0)
    {
        current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else
    {
        current_state->unex_resp_handler(current_state);
        return;
    }
}

// Функция отправки СМС SIM800 (команда "AT+CMGS=«ХХХХХХХХХХХ»")
// Принимает: 1) указатель на структуру состояния данного модуля SIM800
//            2) указатель на строку содержащую телефонный номер абонента
//            3) указатель на строку содержащую текстовое сообщение
// Возвращает статус выполнения
uint8_t sim800_ATplusCMGS_request(struct sim800_current_state * current_state, uint8_t * phone_number, uint8_t * SMS_data)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT+CMGS=", 9);
    strncat(current_state->current_cmd, "\"", 2);
    strncat(current_state->current_cmd, phone_number, 14); // 14 - выбрано с запасом на всякий пожарный (обычный телефонный номер максимум 11 символв)
    strncat(current_state->current_cmd, "\"", 2); // добавим ковычки в конец и начало телефонного номера как того требует стандарт AT-команды
    strncat(current_state->current_cmd, "\r", 2);
    current_state->response_handler = sim800_ATplusCMGS_responce_handler;

    memcpy(current_state->send_SMS_data, SMS_data, SEND_SMS_DATA_SIZE);
    strncat(current_state->send_SMS_data, 0x1A, 1); // В конце СМС должно завершаться символом Ctrl+Z (признак отправки)
    strncat(current_state->send_SMS_data, "\0", 1);
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT+CMGS=«ХХХХХХХХХХХ»"
void sim800_ATplusCMGS_responce_handler(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT+CMGS=",8)==0) // Пришло ЭХО?
    {
        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else if (strchr(&current_state->rec_buf[current_state->current_read_buf][0], '>')) // Пришло приглашение ввести текст СМС сообщения
    {
    	//int j; GPIOA->ODR &= ~GPIO_Pin_0; //for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
    	Sim800_WriteSMS(current_state); // само СМС-сообщение лежит внутри current_state и функция Sim800_WriteSMS извлечет его от туда
        return;
    }
    else if (stristr(&current_state->rec_buf[current_state->current_read_buf][0],"+CMGS:"))
    {
    	//int j; GPIOA->ODR &= ~GPIO_Pin_0; //for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
    	//Обработка служебного сообщения об отправки SMS вида:
        //+CMGS: XXX (XXX - некий условный номер отправленного SMS)
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"\r\nOK",4)==0)
    {
    	//int j; GPIOA->ODR &= ~GPIO_Pin_0; //for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
    	current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else
    {
        current_state->unex_resp_handler(current_state);
        return;
    }
}

// Функция удаления СМС SIM800 (команда "AT+CMGD=1,0" 1, — номер сообщения 0, — режим удаления)
// Принимает: 1) указатель на структуру состояния данного модуля SIM800
//            2) номер сообщения
//            3) режим удаления
// Возвращает статус выполнения
uint8_t sim800_ATplusCMGD_request(struct sim800_current_state * current_state, uint8_t num_of_message, uint8_t mode_of_delete)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT+CMGD=", 9);

    uint8_t buf_string[3]; // в обычную SIM-карту помещается не более 10 - 20 СМС сообщений
    itoa(num_of_message, buf_string, 10);
    strncat(current_state->current_cmd, buf_string, 3);

    strncat(current_state->current_cmd, ",", 2); // добавим запятую как того требует формат отправки команды

    // режимов может быть всего 5: 0 — удаление указанного сообщений. Работает по умолчанию, можно использовать просто AT+CMGD=2
    //                             1 — удаление только всех прочитанных сообщений
    //                             2 — удаление прочитанных и отправленных сообщений
    //                             3 — удаление всех прочитанных, отправленных и не отправленных сообщений
    //                             4 — удаление всех сообщений

    itoa(mode_of_delete, buf_string, 10);
    strncat(current_state->current_cmd, buf_string, 2);

    strncat(current_state->current_cmd, "\r", 2);
    current_state->response_handler = sim800_ATplusCMGD_responce_handler;
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT+CMGD="
void sim800_ATplusCMGD_responce_handler(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT+CMGD=",8)==0) // Пришло ЭХО?
    {

        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"OK",2)==0)
    {
        current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else
    {
        current_state->unex_resp_handler(current_state);
        return;
    }
}

// Функция отправки запроса регистрации в сети SIM800 (команда "AT+CREG?")
// Принимает: 1) указатель на структуру состояния данного модуля SIM800
// Возвращает статус выполнения
uint8_t sim800_ATplusCREGquestion_request(struct sim800_current_state * current_state)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT+CREG?\r", 10);
    current_state->response_handler = sim800_ATplusCREGquestion_responce_handler;
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT+CREG?"
void sim800_ATplusCREGquestion_responce_handler(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT+CREG?",8)==0) // Пришло ЭХО?
    {

        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"+CREG:",6)==0) // Пришло сообщение о текущем состоянии регистрации в сети?
    {
        current_state->current_registration_state = atoi(&current_state->rec_buf[current_state->current_read_buf][9]); //+CREG: 0,1 финальная 1 (9-ый символ) - это и есть текущая регистрация
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"\r\nOK",4)==0)
    {
        current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else
    {
        current_state->unex_resp_handler(current_state);
        return;
    }
}

// Функция отправки запроса на чтения СМС SIM800 (команда "AT+CMGR=1,0" 1, — номер смс
//                                                                      0, — обычный режим или 1, — не изменять статус.)
// Принимает: 1) указатель на структуру состояния данного модуля SIM800
//            2) номер сообщения
//            3) режим чтения
// Возвращает статус выполнения
uint8_t sim800_ATplusCMGR_request(struct sim800_current_state * current_state, uint8_t num_of_message, uint8_t mode_of_read)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT+CMGR=", 9);

    uint8_t buf_string[3]; // в обычную SIM-карту помещается не более 10 - 20 СМС сообщений
    itoa(num_of_message, buf_string, 10);
    strncat(current_state->current_cmd, buf_string, 3);

    strncat(current_state->current_cmd, ",", 2); // добавим запятую как того требует формат отправки команды

    // режимов может быть всего 2: 0 — обычный режим
    //                             1 — не изменять статус

    itoa(mode_of_read, buf_string, 10);
    strncat(current_state->current_cmd, buf_string, 2);

    strncat(current_state->current_cmd, "\r", 2);
    current_state->response_handler = sim800_ATplusCMGR_responce_handler_st1;
    return sim800_request(current_state);
}

// Пришлось обработку ответа разбить на стадии, т.к. ответ большой и его парсинг не успевал до прихода финального OK
// Обработчик ответа команды "AT+CMGR= - чтение SMS стадия 1 - прием ЭХО
void sim800_ATplusCMGR_responce_handler_st1(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT+CMGR=",8)==0) // Пришло ЭХО?
    {
        current_state->response_handler = sim800_ATplusCMGR_responce_handler_st2;
        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else
    {
        current_state->unex_resp_handler(current_state);
        return;
    }
}

// Обработчик ответа команды "AT+CMGR= - чтение SMS стадия 2 - прием служебных данных о SMS
void sim800_ATplusCMGR_responce_handler_st2(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"+CMGR:",6)==0)
    {
        //GPIOA->ODR &= ~GPIO_Pin_0; // ОТЛАДКА!!!
        current_state->response_handler = sim800_ATplusCMGR_responce_handler_st3;
        // ПОКА НИ ЧЕГО НЕ ДЕЛАЕМ, но из строки вида
        // +CMGR: "REC UNREAD","+7XXXXXXXXXX","","16/09/06,14:17:35+12" можно извлечь много полезной информации
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"OK",2)==0) // если приходит сразу OK без текста сообщения, значит запрашиваемая ячейка пуста
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0) // Точнее будет +CMS ERROR: 517 (SIM-карта не готова)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else
    {
        current_state->unex_resp_handler(current_state);
        return;
    }
}

// Обработчик ответа команды "AT+CMGR= - чтение SMS стадия 3 - прием самого текста SMS
void sim800_ATplusCMGR_responce_handler_st3(struct sim800_current_state * current_state)
{
    //			if (stristr(current_state->responce,"REC"))
    //			{
    //				GPIOA->ODR &= ~GPIO_Pin_0; // ОТЛАДКА!!!
    //			}
    current_state->response_handler = sim800_ATplusCMGR_responce_handler_st4;
    memcpy(current_state->rec_SMS_data, &current_state->rec_buf[current_state->current_read_buf][0], strlen(&current_state->rec_buf[current_state->current_read_buf][0])); // копируем принятое SMS сообщение
    return;
}

// Обработчик ответа команды "AT+CMGR= - чтение SMS стадия 4 - обработка сообщения OK
void sim800_ATplusCMGR_responce_handler_st4(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"\r\nOK",4)==0)
    {
        //int j; GPIOA->ODR &= ~GPIO_Pin_0; //for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
        current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
    else
    {
        current_state->unex_resp_handler(current_state);
        return;
    }
}

// Функция отправки USSD запроса SIM800 (команда "AT+CUSD=1,"XXXXX", где XXXXX например #102#)
// Принимает: 1) указатель на структуру состояния данного модуля SIM800
//            2) Строку USSD запроса
uint8_t sim800_ATplusCUSD_request(struct sim800_current_state * current_state, uint8_t * USSD_req)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT+CUSD=1,\"", 12); //=1 — выполнить запрос, ответ вернуть в терминал. Остальные режимы не нужны - 0 — выполнить запрос, полученный ответ проигнорировать, 2 — отменить операцию

    strncat(current_state->current_cmd, USSD_req, 6);
    //    if (mode == text_mode)
    //    {
    //    	strncat(current_state->current_cmd, "1", 1);
    //    }
    //    else
    //    {
    //    	strncat(current_state->current_cmd, "0", 1);
    //	}
    strncat(current_state->current_cmd, "\"", 2); // дописываем завершающие ковычки
    strncat(current_state->current_cmd, "\r", 2);
    current_state->response_handler = sim800_ATplusCUSD_responce_handler;
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT+CUSD=1,"XXXXX"
void sim800_ATplusCUSD_responce_handler(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT+CUSD=1,\"",11)==0) // Пришло ЭХО?
    {
        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"OK",2)==0)
    {
        current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (stristr(&current_state->rec_buf[current_state->current_read_buf][0],"+CUSD:"))
    {
    	memcpy(current_state->last_USSD_data, &current_state->rec_buf[current_state->current_read_buf][0], strlen(&current_state->rec_buf[current_state->current_read_buf][0])); // копируем ответ на USSD запрос
    	return;
    }
    else
    {
        current_state->unex_resp_handler(current_state); // если пришел не ответ на нашу команду, а что-то еще, вызываем обработчик внезапных сообщений
        return;
    }
}

// Функция регистрации модуля SIM800 в GPRS сети (команда "AT+CGATT=1")
// Принимает: 1) указатель на структуру состояния данного модуля SIM800
uint8_t sim800_ATplusCGATTequal1_request(struct sim800_current_state * current_state)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT+CGATT=1", 11);
    strncat(current_state->current_cmd, "\r", 2);
    current_state->response_handler = sim800_ATplusCGATTequal1_responce_handler;
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT+CGATT=1"
void sim800_ATplusCGATTequal1_responce_handler(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT+CGATT=1",10)==0) // Пришло ЭХО?
    {
        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"OK",2)==0)
    {
        current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else
    {
        current_state->unex_resp_handler(current_state); // если пришел не ответ на нашу команду, а что-то еще, вызываем обработчик внезапных сообщений
        return;
    }
}

// Функция переключения модуля SIM800 в режим приема GPRS даненых вручную (команда "AT+CIPRXGET=1")
// Принимает: 1) указатель на структуру состояния данного модуля SIM800
uint8_t sim800_ATplusCIPRXGETequal1_request(struct sim800_current_state * current_state)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT+CIPRXGET=1", 14);
    strncat(current_state->current_cmd, "\r", 2);
    current_state->response_handler = sim800_ATplusCIPRXGETequal1_responce_handler;
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT+CIPRXGET=1"
void sim800_ATplusCIPRXGETequal1_responce_handler(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT+CIPRXGET=1",13)==0) // Пришло ЭХО?
    {
        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"OK",2)==0)
    {
        current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else
    {
        current_state->unex_resp_handler(current_state); // если пришел не ответ на нашу команду, а что-то еще, вызываем обработчик внезапных сообщений
        return;
    }
}

// Функция настройки колличества подключений модуля SIM800 в режим приема GPRS (команда "AT+CIPMUX=x", где x = 0 - одно соединение или 1 - много соединений)
// Принимает: 1) указатель на структуру состояния данного модуля SIM800
//            2) признак колличества соединений single_connection или multiple_connection
uint8_t sim800_ATplusCIPMUX_request(struct sim800_current_state * current_state, uint8_t mode)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT+CIPMUX=", 11);
    uint8_t string_of_mode[2];
    itoa(mode, string_of_mode, 10);

    strncat(current_state->current_cmd, string_of_mode, 1);

    strncat(current_state->current_cmd, "\r", 2);
    current_state->response_handler = sim800_ATplusCIPMUX_responce_handler;
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT+CIPMUX=x"
void sim800_ATplusCIPMUX_responce_handler(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT+CIPMUX=",10)==0) // Пришло ЭХО?
    {
        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"OK",2)==0)
    {
        current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else
    {
        current_state->unex_resp_handler(current_state); // если пришел не ответ на нашу команду, а что-то еще, вызываем обработчик внезапных сообщений
        return;
    }
}

// Функция настройки точки доступа SIM800 в режим приема GPRS (команда "AT+CSTT="xxxxxxxxx"", где xxxxxxxxx - это APN — Access Point Name
// Принимает: 1) указатель на структуру состояния данного модуля SIM800
uint8_t sim800_ATplusCSTT_request(struct sim800_current_state * current_state)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT+CSTT=", 9);

    // теперь надо определить какой мобильный оператор на нашей SIM-карте и в зависимости от этого сформировать правильной APN запрос
    switch (current_state->current_mobile_operator) {
		case Beeline:
		{
			strncat(current_state->current_cmd, "\"internet.beeline.ru\"", 22);
			break;
		}
		case MTS:
		{
			strncat(current_state->current_cmd, "\"internet.mts.ru\"", 18);
			break;
		}
		case MegaPhone:
		{
			strncat(current_state->current_cmd, "\"internet\"", 11);
			break;
		}
		case Tele2:
		{
			strncat(current_state->current_cmd, "\"internet\"", 11);
			break;
		}
		case Yota:
		{
			strncat(current_state->current_cmd, "\"yota.ru\"", 10);
			break;
		}
		default:
		{
			//int j; GPIOA->ODR &= ~GPIO_Pin_0; //for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
			return ATplusCSTTfail;
			break;
		}
	}

    strncat(current_state->current_cmd, "\r", 2);
    current_state->response_handler = sim800_ATplusCSTT_responce_handler;
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT+CSTT="xxxxxxxxx""
void sim800_ATplusCSTT_responce_handler(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT+CSTT=",8)==0) // Пришло ЭХО?
    {
        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"OK",2)==0)
    {
        current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else
    {
        current_state->unex_resp_handler(current_state); // если пришел не ответ на нашу команду, а что-то еще, вызываем обработчик внезапных сообщений
        return;
    }
}

// Функция установки беспроводного GPRS соединения модуля SIM800(команда "AT+CIICR")
// Принимает: 1) указатель на структуру состояния данного модуля SIM800
uint8_t sim800_ATplusCIICR_request(struct sim800_current_state * current_state)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT+CIICR", 9);
    strncat(current_state->current_cmd, "\r", 2);
    current_state->response_handler = sim800_ATplusCIICR_responce_handler;
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT+CIICR"
void sim800_ATplusCIICR_responce_handler(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT+CIICR",8)==0) // Пришло ЭХО?
    {
        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"OK",2)==0)
    {
        current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else
    {
        current_state->unex_resp_handler(current_state); // если пришел не ответ на нашу команду, а что-то еще, вызываем обработчик внезапных сообщений
        return;
    }
}

// Функция запроса IP адреса беспроводного GPRS соединения модуля SIM800(команда "AT+CIFSR")
// Принимает: 1) указатель на структуру состояния данного модуля SIM800
uint8_t sim800_ATplusCIFSR_request(struct sim800_current_state * current_state)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT+CIFSR", 9);
    strncat(current_state->current_cmd, "\r", 2);
    current_state->response_handler = sim800_ATplusCIICR_responce_handler;
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT+CIFSR"
void sim800_ATplusCIFSR_responce_handler(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT+CIFSR",8)==0) // Пришло ЭХО?
    {
        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else // модуль просто выдает свой IP-адрес без сообщения "OK"
    {
    	memcpy(current_state->IP_address_string, &current_state->rec_buf[current_state->current_read_buf][0], strlen(&current_state->rec_buf[current_state->current_read_buf][0])); // сохраняем текущий IP-адрес
    	current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
}

// Функция устанавливки сервера DNS беспроводного GPRS соединения модуля SIM800(команда "AT+CDNSCFG="X.X.X.X","X.X.X.X"")
// "X.X.X.X","X.X.X.X" - это первичный и вторичный DNS сервер
// Принимает: 1) указатель на структуру состояния данного модуля SIM800
//            2) строку содержащую первичный DNS сервер
//            3) строку содержащую вторичный DNS сервер
uint8_t sim800_ATplusCDNSCFG_request(struct sim800_current_state * current_state, uint8_t * primary_DNS_server, uint8_t * secondary_DNS_server)
{
    if(current_state->communication_stage != proc_completed) // защита от слишком частых запросов
    {
        return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
    }
    memcpy(current_state->current_cmd, "AT+CDNSCFG=", 12);
    strncat(current_state->current_cmd, "\"", 2);
    strncat(current_state->current_cmd, primary_DNS_server, strlen(primary_DNS_server)+1);
    strncat(current_state->current_cmd, "\",\"", 4);
    strncat(current_state->current_cmd, secondary_DNS_server, strlen(secondary_DNS_server)+1);
    strncat(current_state->current_cmd, "\r", 2);
    current_state->response_handler = sim800_ATplusCDNSCFG_responce_handler;
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT+CDNSCFG"
void sim800_ATplusCDNSCFG_responce_handler(struct sim800_current_state * current_state)
{
    if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"AT+CDNSCFG",8)==0) // Пришло ЭХО?
    {
        return; // ни чего не делаем (хотя потом можно ставить некий флаг)
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"OK",2)==0)
    {
        current_state->result_of_last_execution = OK;
        current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
        current_state->communication_stage = proc_completed;
        return;
    }
    else if (strncasecmp(&current_state->rec_buf[current_state->current_read_buf][0],"ERROR",5)==0)
    {
        current_state->result_of_last_execution = fail;
        current_state->response_handler = NULL;
        current_state->communication_stage = proc_completed;
        return;
    }
    else
    {
        current_state->unex_resp_handler(current_state); // если пришел не ответ на нашу команду, а что-то еще, вызываем обработчик внезапных сообщений
        return;
    }
}

//**********************************************************************
//**********************************************************************

void select_sim_card1(void) // Функция выбора SIM-карты 1 (вынес в функции, т.к. модулей SIM800 может быть несколько и у каждого могут быть свои мультиплексоры выбора SIM-карт)
{
	select_sim1; // Функция выбора SIM-карты 1
}

void select_sim_card2(void) // Функция выбора SIM-карты 2
{
	select_sim2; // Функция выбора SIM-карты 2
}

// Функция инициализации одного из модулей SIM800
// Принимает: 1) указатель на конкретную структуру описывающую состояние конкретного модуля
//            2) указатель на функцию отправки данных в конкретный UART на котором сидит данный модуль
//            3) номер текущей подключенной SIM-карты для случая наличия микросхемы переключения SIM-карт
//            4) пин код для соответствующей SIM-карты
uint8_t sim800_init(struct sim800_current_state * current_state, void (*send_uart_function)(char *), uint8_t cur_SIM_card, uint16_t pin_code)
{
    // Инициализируем структуру статуса для одного конкретного модуля
    current_state->is_pin_req = no;
    current_state->is_pin_accept = no;
	current_state->communication_stage = proc_completed; // в начале работы сбрасываем счетчик стадий в ноль
    current_state->current_pos = 0;
    current_state->current_read_buf = 0;
    current_state->current_write_buf = 0;
    memset(current_state->rec_buf,'\0',REC_BUF_SIZE);
    memset(current_state->responce,'\0',REC_BUF_SIZE);
    memset(current_state->current_cmd,'\0',CURRENT_CMD_SIZE);
    //current_state->current_req = NULL;
    current_state->send_uart_function = send_uart_function; // привязка конкретного модуля к конкретному UART-у
    current_state->result_of_last_execution = OK;
    current_state->num_of_sms = 0;
    current_state->unex_resp_handler = unexpec_message_parse;
    current_state->response_handler = NULL;
    memset(current_state->send_phone_number,'\0',PHONE_NUM_SIZE);
    memset(current_state->rec_phone_number,'\0',PHONE_NUM_SIZE);
    memset(current_state->send_SMS_data,'\0',SEND_SMS_DATA_SIZE);
    memset(current_state->rec_SMS_data,'\0',REC_SMS_DATA_SIZE);
    memset(current_state->last_USSD_data,'\0',USSD_DATA_SIZE);
    current_state->is_Call_Ready = not_ready;
    current_state->is_SMS_Ready = not_ready;
    current_state->power_voltage_status = NORMAL_VOLT;
    current_state->current_registration_state = 0;
    current_state->SIM1_select_handler = select_sim_card1;
    current_state->SIM2_select_handler = select_sim_card2;

    current_state->PWR_KEY_handler = sim800_1_PWRKEY_on;    // указатель на функцию включения конкретного модуля SIM800
    current_state->PWR_KEY_handler();                       // включаем модуль

    // выбираем активную SIM-карту
    if (cur_SIM_card == 1)
    {
        current_state->current_SIM_card = 1;
        current_state->SIM1_select_handler();
    }
    else if (cur_SIM_card == 2)
    {
    	current_state->current_SIM_card = 2;
    	current_state->SIM2_select_handler();
    }
    else
    {
	    return fail; // подключенно может быть не более 2-х SIM карт
	}

    uint32_t count = 0; // счетчик на случай проблем с обменом с SIM800

    //----------------------
    sim800_AT_request(&state_of_sim800_num1); // пробуем отправить тестовую команду, заодно настроив скорость передачи по UART
    while (current_state->communication_stage != proc_completed) // ждем пока не ответит OK
    {
        count ++;
        if (count == REQ_TIMEOUT)
        {
        	return ATfail;
        }
    };
    count = 0;
    if (current_state->result_of_last_execution == fail)
    {
    	return ATfail;
    };
    //----------------------

    //----------------------
    while (current_state->is_pin_req == no) // ждем запроса ввести PIN-код
    {
    	count ++;
        if (count == LONG_REQ_TIMEOUT)
        {
        	return PIN_CODE_REQ_EXPECfail;
        }
        if (current_state->is_pin_accept == yes) // эта проверка на случай если PIN-код вводить не надо
        {
            break;
        }
    }
    count = 0;
    if (current_state->is_pin_accept == no)
    {
    	sim800_ATplusCPIN_request(&state_of_sim800_num1, pin_code);
    }
    while (current_state->communication_stage != proc_completed)
    {
        count ++;
        if (count == REQ_TIMEOUT)
        {
        	return ATplusCPINfail;
        }
    }
    count = 0;
    if (current_state->result_of_last_execution == fail)
    {
    	return ATplusCPINfail;
    };
    //----------------------

    //----------------------
    while ((current_state->is_Call_Ready != ready) || (current_state->is_SMS_Ready != ready)) // ждем выдачи сообщений Call Ready и SMS Ready
    {
    	count ++;
        if (count == LL_REQ_TIMEOUT)
        {
        	return CALL_SMS_EXPECfail;
        }
    }
    count = 0;
    //----------------------

    //----------------------
    sim800_ATplusCMGF_request(&state_of_sim800_num1, text_mode); // переключение в текстовый режим ввода SMS
    while (current_state->communication_stage != proc_completed) // ждем пока не ответит OK
    {
        count ++;
        if (count == REQ_TIMEOUT)
        {
        	return ATplusCMGFfail;
        }
    };
    count = 0;
    if (current_state->result_of_last_execution == fail)
    {
    	return ATplusCMGFfail;
    };
    //----------------------

    //----------------------
    sim800_ATplusCMGD_request(&state_of_sim800_num1, 1, 4); // удаление всех SMS
    while (current_state->communication_stage != proc_completed) // ждем пока не ответит OK
    {
        count ++;
        if (count == REQ_TIMEOUT)
        {
        	return ATplusCMGDfail;
        }
    };
    count = 0;
    if (current_state->result_of_last_execution == fail)
    {
    	return ATplusCMGDfail;
    };
    //----------------------

    //----------------------
    sim800_ATplusCREGquestion_request(&state_of_sim800_num1); // запрос регистрации в сети
    while (current_state->communication_stage != proc_completed) // ждем пока не ответит OK
    {
        count ++;
        if (count == REQ_TIMEOUT)
        {
        	return ATplusCREGfail;
        }
    };
    count = 0;
    if (current_state->current_registration_state != 1)
    {
    	return ATplusCREGfail;
    };
    //----------------------

    //----------------------
    sim800_ATplusCSPNquestion_request(&state_of_sim800_num1); // запрос оператора SIM-карты
    while (current_state->communication_stage != proc_completed) // ждем пока не ответит OK
    {
        count ++;
        if (count == LONG_REQ_TIMEOUT)
        {
        	return ATplusCSPNfail;
        }
    };
    count = 0;
    if (current_state->result_of_last_execution == fail)
    {
    	return ATplusCSPNfail;
    };
    if (cur_SIM_card == 1) // текущий оператор копируется в оператор соответствующей SIM-карты (служебная информация)
    {
    	current_state->mobile_operator_SIM1 = current_state->current_mobile_operator;
    }
    else if (cur_SIM_card == 2)
    {
    	current_state->mobile_operator_SIM2 = current_state->current_mobile_operator;
    }

    //----------------------

    return OK;
}

// Функция инициализации GPRS одного из модулей SIM800
// Принимает: 1) указатель на конкретную структуру описывающую состояние конкретного модуля
// Должна вызываться после sim800_init если требуется GPRS
uint8_t sim800_GPRS_init(struct sim800_current_state * current_state, void (*send_uart_function)(char *), uint8_t cur_SIM_card, uint16_t pin_code)
{

}

// Вспомогательная функция копирования содержимого приемного буфера в буфер принятого ответа
// и вызова соответствующего обработчика принятого ответа
void call_handler(struct sim800_current_state * current_state)
{

    if (current_state->response_handler != NULL)
    {
        current_state->communication_stage = resp_rec; // ответ получен
        current_state->response_handler(current_state);
    }
    else // если по указателю на обработчик ответа лежит NULL, значит запрос не отправлялся и это неожиданное сообщение от SIM800 (например приход СМС)
    {
        current_state->unex_resp_handler(current_state);
    }
    return;
}

// функция вызываемая из обработчика прерывания по приему символов от SIM800
// которя формирует полный ответа и отправляет его на дальнейшие разбор и обработку
// Принимает: указатель на структуру содержащую текущее состояние конкретного модуля SIM800(их может быть несколько) и принятый по UART символ
// ни чего не возвращает, а в коде складывает ответ в приемный буфер и в случае приема всей команды вызывает соответсвующий обработчик ответа
void sim800_response_handler(struct sim800_current_state * current_state, uint8_t data)
{
    current_state->rec_buf[current_state->current_write_buf][current_state->current_pos++] = data;
    current_state->rec_buf[current_state->current_write_buf][current_state->current_pos] = '\0'; // заполняем следующую за последним принятым символом позицию нуль-терминатором для корректной работы функций наподобие stristr, memcpy и прочих
    if (current_state->current_pos > REC_BUF_SIZE - 1)
    {
        // В случае переполнения леквидируем полученный ответ
        current_state->current_pos = 0;
        return;
    }

    //надо отследить приглашение к вводу SMS-ки (символ '>')
    if (strchr(&current_state->rec_buf[current_state->current_write_buf][current_state->current_pos - 1], '>'))
    {
    	current_state->current_read_buf = current_state->current_write_buf; // читаем из текущего заполненого буфера
    	if (++ current_state->current_write_buf == NUM_OF_SUBBUF) {current_state->current_write_buf = 0;}; // переключаемся на следующий буфер для записи принимаемых данных
        current_state->current_pos = 0;
        call_handler(current_state);
        return;
    }

    if (current_state->current_pos <= 3) // если принято меньше 3-х символов ответа то остальные проверки делать бессмысленно
        return;

    // надо проверить приемный буфер на наличие признака конца ответного сообщения (два последовательных символа "\r\n")
    if( stristr(&current_state->rec_buf[current_state->current_write_buf][current_state->current_pos - 2], "\r\n") )
    {
        //memcpy(current_state->responce, current_state->rec_buf, current_state->current_pos + 1); // копируем содержимое приемного буфера в буфер принятого ответа (т.к. содержимое приемного буфера продолжает дополнятся при приеме)
    	current_state->current_read_buf = current_state->current_write_buf; // читаем из текущего заполненого буфера
    	if (++ current_state->current_write_buf == NUM_OF_SUBBUF) {current_state->current_write_buf = 0;}; // переключаемся на следующий буфер для записи принимаемых данных
    	current_state->current_pos = 0;
        call_handler(current_state);
        return;
    }
}

//функция парсинга внезапных сообщений от SIM800 (например пришла SMS)
void unexpec_message_parse(struct sim800_current_state *current_state)
{
    if (stristr(&current_state->rec_buf[current_state->current_read_buf][0],"+CMTI:")) // Пришло СМС сообщение (нпример "+CMTI: "SM",12")
    {
        // !!!!!!!!! ТУТ НАДО СДЕЛАТЬ ОБРАБОТКУ ПРИНЯТОГО СМС СООБЩЕНИЯ ИЛИ ХОТЯБЫ ИНКРЕМЕНТИРОВАТЬ СЧЕТЧИК ПРИНЯТЫХ СООБЩЕНИЙ
        //int j; GPIOA->ODR &= ~GPIO_Pin_0; //for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
        return;
    }
    else if (stristr(&current_state->rec_buf[current_state->current_read_buf][0],"Call Ready")) //
    {
        current_state->is_Call_Ready = ready;
        return;
    }
    else if (stristr(&current_state->rec_buf[current_state->current_read_buf][0],"SMS Ready")) //
    {
        current_state->is_SMS_Ready = ready;
        int j; GPIOA->ODR &= ~GPIO_Pin_0; for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
        return;
    }
    else if (stristr(&current_state->rec_buf[current_state->current_read_buf][0],"RING")) // Нам звонят.
    {
        // Пока ни чего не делаем
        return;
    }
    else if (stristr(&current_state->rec_buf[current_state->current_read_buf][0],"NO CARRIER")) // Звонок окончен
    {
        // Пока ни чего не делаем
        return;
    }
    else if (stristr(&current_state->rec_buf[current_state->current_read_buf][0],"+CPIN: SIM PIN"))// - SIM-карта требует ввести пин-код
    {
    	current_state->is_pin_req = yes;
    }
    else if (stristr(&current_state->rec_buf[current_state->current_read_buf][0],"+CPIN: READY"))// - SIM-карта приняла пин-код или он вообще не требуется
    {
    	current_state->is_pin_req = no;
    	current_state->is_pin_accept = yes;
    }
    else if (stristr(&current_state->rec_buf[current_state->current_read_buf][0],"+CPIN: NOT INSERTED"))// - SIM-карта не вставлена
    {
        // !!!!! надо написать код обработки этого сообщения
    	return;
    }
    else if (stristr(&current_state->rec_buf[current_state->current_read_buf][0],"UNDER-VOLTAGE WARNNING"))
    {
		// При понижении питающего напряжения ниже 3,3 Вольт модуль начинает слать соответствующие предупреждения. Сообщения будут отсылаться каждые 5 секунд.
    	current_state->power_voltage_status = LOW_VOLTAGE;
	}
    else
    {
        return; // а это все остальные внезапные сообщения
    }
}
