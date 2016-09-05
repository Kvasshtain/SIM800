#include <stdio.h>
#include "phisic.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "SIM800.h"



struct sim800_current_state state_of_sim800_num1; // модулей может быть несколько

//const struct request AT_req = {"AT", sim800_AT_responce_handler}; // команда AT проверка связи
//const struct request ATplusCMGS_req = {"AT+CMGS=", sim800_ATplusCMGS_responce_handler}; // команда AT+CMGS=«ХХХХХХХХХХХ» - отправка СМС

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
//	if(current_state->communication_stage != proc_completed)
//    {
//    	return busy;                             // Запрос отправить не удалось, т.к. предидущий запрос еще не получен или не обработан
//    }
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
	memcpy(current_state->current_cmd, "AT\r", 4);
    current_state->response_handler = sim800_AT_responce_handler;
	return sim800_request(current_state);
}

// Обработчик ответа команды "AT"
void sim800_AT_responce_handler(struct sim800_current_state * current_state)
{
	if (strstr(current_state->responce,"AT")) // Пришло ЭХО?
	{
		//int j; GPIOA->ODR &= ~GPIO_Pin_0; for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
		return; // ни чего не делаем (хотя потом можно ставить некий флаг)
	}
	else if (strstr(current_state->responce,"OK"))
	{
		current_state->result_of_last_execution = OK;
		current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
		current_state->communication_stage = proc_completed;
		return;
	}
	else if (strstr(current_state->responce,"ERROR"))
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

// Функция переключения SIM800 в текстовый режим (команда "AT+CMGF=1 или 0 1-включить, 0-выключить")
uint8_t sim800_ATplusCMGF_request(struct sim800_current_state * current_state, uint8_t * mode)
{
    memcpy(current_state->current_cmd, "AT+CMGF=", 9);
    if (mode == text_mode)
    {
    	strncat(current_state->current_cmd, "1", 1);
    }
    else
    {
    	strncat(current_state->current_cmd, "0", 1);
	}

    strncat(current_state->current_cmd, "\r", 2);
    current_state->response_handler = sim800_ATplusCMGF_responce_handler;
    return sim800_request(current_state);
}

// Обработчик ответа команды "AT+CMGF=0/1"
void sim800_ATplusCMGF_responce_handler(struct sim800_current_state * current_state)
{
	if (strstr(current_state->responce,"AT+CMGF=")) // Пришло ЭХО?
	{
		return; // ни чего не делаем (хотя потом можно ставить некий флаг)
	}
	else if (strstr(current_state->responce,"OK"))
	{
		//int j; GPIOA->ODR &= ~GPIO_Pin_0; for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
		current_state->result_of_last_execution = OK;
		current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
		current_state->communication_stage = proc_completed;
		return;
	}
	else if (strstr(current_state->responce,"ERROR"))
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
uint8_t sim800_ATplusCMGS_request(struct sim800_current_state * current_state, uint8_t * phone_number, uint8_t * SMS_data)
{
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
	if (strstr(current_state->responce,"AT+CMGS=")) // Пришло ЭХО?
	{

		return; // ни чего не делаем (хотя потом можно ставить некий флаг)
	}
	else if (strchr(current_state->responce, '>')) // Пришло приглашение ввести текст СМС сообщения
	{
		//current_state->send_uart_function("HELLO!!!");

		//Sim800_WriteSMS(current_state); // само СМС-сообщение лежит внутри current_state и функция Sim800_WriteSMS извлечет его от туда
		Sim800_WriteSMS(current_state); //ОТЛАДКА!!!!
		return;
	}
	else if (strstr(current_state->responce,"OK"))
	{
		current_state->result_of_last_execution = OK;
		current_state->response_handler = NULL; // сбрасываем указатель на обработчик в NULL (ответ обработан)
		current_state->communication_stage = proc_completed;
		return;
	}
	else if (strstr(current_state->responce,"ERROR"))
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

//**********************************************************************
//**********************************************************************

// Функция инициализации одного из модулей SIM800
// Принимает: 1) указатель на конкретную структуру описывающую состояние конкретного модуля
//            2) указатель на функцию отправки данных в конкретный UART на котором сидит данный модуль
uint8_t sim800_init(struct sim800_current_state * current_state, void (*send_uart_function)(char *))
{
	// Инициализируем структуру статуса для одного конкретного модуля
	current_state->communication_stage = proc_completed;
	current_state->current_pos = 0;
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
	current_state->PWR_KEY_handler = sim800_1_PWRKEY_on;    // указатель на функцию включения конкретного модуля SIM800
	current_state->PWR_KEY_handler();                       // включаем модуль
	// пробуем отправить тестовую команду, заодно настроив скорость передачи по UART
	sim800_AT_request(&state_of_sim800_num1);








	//ТУТ НАДО ВСЕ ПРОИНИЦИАЛИЗИРОВАТЬ И СТЕРЕТЬ ВСЕ СТАРЫЕ СМС-ки
//	sim800_AT_request(current_module);
//	while (current_module->communication_stage);
//    if(current_module->result_of_last_execution)
//    	return 1; //неудача
//    return 0;
}

//// Обработка ЭХО
//void process_echo(uint8_t is_responce, uint8_t current_pos, struct sim800_current_state *current_state)
//{
//    //теперь надо проверить, а это точно ответ на наш запрос (совподает ли отправленная команда с полученным ЭХО)
//    if (strstr(current_state->rec_buf ,current_state->current_req->current_cmd))
//    {
//        // если да, то спокойно переводим текущую позицию приемного буфера в начало, тем самым выкинув ЭХО из последующих проверок
//        //(новые данные будут записываться поверх ЭХО), что бы не делать поиск \r\r\n повторно в функции парсинга ответа
//       current_pos = 0;
//        // необходимо еще выставить флаг, что все дальнейшие получаемые данные до следующей последовательности символов \r\n - это точно
//        // ответ на выданный запрос, а не внезапное сообщение например о пришедшем SMS
//        is_responce = yes;
//    } // если же нет, то возможно это еще какой-то полезный ответ
//    else // вставляем тогда вместо первого \r символ конца строки \0 (в конце было \r\r\n станет \0\r\n)
//    {
//        current_state->rec_buf[current_pos - 3] = '\0';
//
//
//        current_pos = 0;
//    }
//}
//
//// Обработка ответа на команду
//void process_cmd(uint8_t is_responce, uint8_t current_pos, struct sim800_current_state *current_state)
//{
//    if( !strstr(&current_state->rec_buf[current_pos - 2], "\r\n") )
//        return; //битая команда
//
//    // для этого вставляем вместо первого \r символ конца строки \0 (в конце было \r\n станет \0\n)
//    current_state->rec_buf[current_pos - 2] = '\0';
//    if (is_responce == yes) // если до этого уже пришло ЭХО отправленной команды, то это ответ на нее
//    {
//        memcpy(current_state->responce, current_state->rec_buf, current_pos - 1);
//        //!!!!!!!!!
//        is_responce = no;
//    }
//    else // если же это внезапное сообщение, например о приходе SMS
//    {
//
//
//    }
//}

// функция вызываемая из обработчика прерывания по приему символов от SIM800
// которя формирует полный ответа и отправляет его на дальнейшие разбор и обработку
// Принимает: указатель на структуру содержащую текущее состояние конкретного модуля SIM800(их может быть несколько) и принятый по UART символ
// ни чего не возвращает, а в коде складывает ответ в приемный буфер и в случае приема всей команды вызывает соответсвующий обработчик ответа
void sim800_response_handler(struct sim800_current_state * current_state, uint8_t data)
{
    current_state->rec_buf[current_state->current_pos] = data;
    current_state->current_pos++;
    current_state->rec_buf[current_state->current_pos] = '\0'; // заполняем следующую за последним принятым символом позицию нуль-терминатором для корректной работы функций наподобие strstr, memcpy и прочих
    if (current_state->current_pos > REC_BUF_SIZE - 1)
    {
        // В случае переполнения леквидируем полученный ответ
    	current_state->current_pos = 0;
        return;
    }

    //надо отследить приглашение к вводу SMS-ки (символ '>')
    if (strchr(&current_state->rec_buf[current_state->current_pos - 1], '>'))
    {
      	//int j; GPIOA->ODR &= ~GPIO_Pin_0; //for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
       	memcpy(current_state->responce, current_state->rec_buf, current_state->current_pos); // копируем содержимое приемного буфера в буфер принятого ответа (т.к. содержимое приемного буфера продолжает дополнятся при приеме)
       	if (current_state->response_handler != NULL)
       	{
       	    current_state->communication_stage = resp_rec; // ответ получен
       	    current_state->response_handler(current_state);
       	}
       	else // если по указателю на обработчик ответа лежит NULL, значит запрос не отправлялся и это неожиданное сообщение от SIM800 (например приход СМС)
       	{
       		current_state->unex_resp_handler(current_state);
       	}
       	current_state->current_pos = 0;
       	return;
    }

    if (current_state->current_pos <= 3)
        return;

    // теперь надо проверить приемный буфер на наличие заветных \r\n
    if( strstr(&current_state->rec_buf[current_state->current_pos - 2], "\r\n") )
    {
    	memcpy(current_state->responce, current_state->rec_buf, current_state->current_pos); // копируем содержимое приемного буфера в буфер принятого ответа (т.к. содержимое приемного буфера продолжает дополнятся при приеме)
    	if (current_state->response_handler != NULL)
    	{
    		current_state->communication_stage = resp_rec; // ответ получен
    		current_state->response_handler(current_state);
    	}
    	else // если по указателю на обработчик ответа лежит NULL, значит запрос не отправлялся и это неожиданное сообщение от SIM800 (например приход СМС)
    	{
    		current_state->unex_resp_handler(current_state);
		}
    	current_state->current_pos = 0;
    	return;
    }

}

//функция парсинга внезапных сообщений от SIM800 (например пришла SMS)
void unexpec_message_parse(struct sim800_current_state *current_state)
{
	if (strstr(current_state->responce,"+CMGS:")) // СМС отправляется
	{
	    return; // ни чего не делаем (хотя потом можно ставить некий флаг)
	}
	else if (strstr(current_state->responce,"+CMTI:")) // Пришло СМС сообщение (нпример "+CMTI: "SM",12")
	{
		// !!!!!!!!! ТУТ НАДО СДЕЛАТЬ ОБРАБОТКУ ПРИНЯТОГО СМС СООБЩЕНИЯ ИЛИ ХОТЯБЫ ИНКРЕМЕНТИРОВАТЬ СЧЕТЧИК ПРИНЯТЫХ СООБЩЕНИЙ
		return;
	}
	else
	{
		return; // а это все остальные внезапные сообщения
	}
}

// Функция отправки SMS с модуля SIM800
//uint8_t sim800_sendSMS(uint8_t* text_buf, uint8_t length)
//{
//
//}
