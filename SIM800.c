#include <stdio.h>
#include "phisic.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "SIM800.h"

sim800_current_state sim800_1_current_state; // модулей может быть несколько

//sim800_1_current_state.send_uart_function = send_str_uart2rn; // данный конкретный модуль SIM800 сидит на порту UART2

// Функция включения модуля SIM800
void sim800_PWRKEY_on(void)
{
    // Подаем на порт PWRKEY лог. 0
    GPIO_WriteBit (GPIO_SIM800_PWRKEY, SIM800_PWRKEY, Bit_RESET);
    // Задержка в мс., необходимая для включения
    DelayMs(2000);

    // Подаем на порт PWRKEY лог. 1
    GPIO_WriteBit (GPIO_SIM800_PWRKEY, SIM800_PWRKEY, Bit_SET);
    // Ожидаем включение/выключение SIM800
    DelayMs(4000);
}

// Функция отправки данных в UART на котором сидит Sim800
inline void Sim800_WriteCmd(sim800_current_state * current_module)
{
	current_module->send_uart_function(current_module->current_cmd); //с добавлением завершающих символов \r\n
}

//ФУНКЦИИ ОТПРАВКИ ЗАПРОСОВ И ОБРАБОТЧИКИ ОТВЕТОВ (ПОПАРНО ЗАПРОС/ОТВЕТ)

// Функция отправки запроса на автонастройку baudrate модуля SIM800 (команда "AT")
void sim800_AT_request(sim800_current_state * current_module)
{
    memcpy(current_module->current_cmd, "AT", 2);
    current_module->response_handler = sim800_AT_responce_handler;
	sim800_request(current_module); // AT\r команда автонастройки скорости передачи данных SIM800 по UART
}

// Обработчик ответа команды "AT"
void sim800_AT_responce_handler(uint8_t * responce, uint8_t result, uint8_t is_busy)
{
	if (strstr(responce,"OK"))
	{
	    result = OK;
	}
	else
	{
		result = fail;
	}
	is_busy = free; // в конце обработки ответа на запрос выставляем флаг "свободно", для разрешения новых запросов
}

// Функция инициализации одного из модулей SIM800
uint8_t sim800_init(sim800_current_state * current_module, int32_t * init_data)
{
	sim800_AT_request(current_module);
	while (current_module->communication_stage);
    if(current_module->result_of_last_execution)
    	return 1; //неудача
    return 0;
}

// функция отправки запросов в SIM800
// Принимает: указатель на структуру содержащую текущее состояние конкретного модуля SIM800(их может быть несколько)
// Возвращает статус занят не занят
int8_t sim800_request(sim800_current_state * current_state)
{
    if(current_state->communication_stage)
	    return busy;

	current_state->communication_stage = busy;
	Sim800_WriteCmd(current_state->current_cmd);

	return exit_and_wait;                         // Возвращаемся в вызвавший код сигнал ждать ответа и заниматься другими делами
}

// Обработка ЭХО
void process_echo(uint8_t is_responce, uint8_t current_pos, sim800_current_state *current_state)
{
    //теперь надо проверить, а это точно ответ на наш запрос (совподает ли отправленная команда с полученным ЭХО)
    if (strstr(current_state->rec_buf ,current_state->current_cmd))
    {
        // если да, то спокойно переводим текущую позицию приемного буфера в начало, тем самым выкинув ЭХО из последующих проверок
        //(новые данные будут записываться поверх ЭХО), что бы не делать поиск \r\r\n повторно в функции парсинга ответа
       current_pos = 0;
        // необходимо еще выставить флаг, что все дальнейшие получаемые данные до следующей последовательности символов \r\n - это точно
        // ответ на выданный запрос, а не внезапное сообщение например о пришедшем SMS
        is_responce = yes;
    } // если же нет, то возможно это еще какой-то полезный ответ
    else // вставляем тогда вместо первого \r символ конца строки \0 (в конце было \r\r\n станет \0\r\n)
    {
        current_state->rec_buf[current_pos - 3] = '\0';
        memcpy(current_state->unexp_responce, current_state->rec_buf, current_pos - 2);
        unexpec_message_parse(current_state->unexp_responce); // вызываем функцию разбора внезапных сообщений
        current_pos = 0;
    }
}

// Обработка ответа на команду
void process_cmd(uint8_t is_responce, uint8_t current_pos, sim800_current_state *current_state)
{
    if( !strstr(&current_state->rec_buf[current_pos - 2], "\r\n") )
        return; //битая команда

    // для этого вставляем вместо первого \r символ конца строки \0 (в конце было \r\n станет \0\n)
    current_state->rec_buf[current_pos - 2] = '\0';
    if (is_responce == yes) // если до этого уже пришло ЭХО отправленной команды, то это ответ на нее
    {
        memcpy(current_state->responce, current_state->rec_buf, current_pos - 1);
        current_state->response_handler(current_state->responce, current_state->result_of_last_execution, current_state->communication_stage); // вызываем функцию обработки ответа, которая нам указана в структуре текущего состояния
        is_responce = no;
    }
    else // если же это внезапное сообщение, например о приходе SMS
    {
        memcpy (current_state->unexp_responce, current_state->rec_buf, current_pos - 1);
        unexpec_message_parse(current_state->unexp_responce); // вызываем функцию разбора внезапных сообщений
    }
}

// функция вызываемая из обработчика прерывания по приему символов от SIM800
// которя формирует полный ответа и отправляет его на дальнейшие разбор и обработку
// Принимает: указатель на структуру содержащую текущее состояние конкретного модуля SIM800(их может быть несколько) и принятый по UART символ
// ни чего не возвращает, а в коде складывает ответ в приемный буфер и в случае приема всей команды вызывает соответсвующий обработчик ответа
void sim800_response_handler(sim800_current_state * current_state, uint8_t data)
{
    //static uint8_t rec_buf[REC_BUF_SIZE]; // временный приемный буфер

    current_state->rec_buf[current_state->current_pos] = data;
    current_state->current_pos++;
    current_state->rec_buf[current_state->current_pos] = '\0';
    if (current_state->current_pos > REC_BUF_SIZE - 1)
    {
        // В случае переполнения леквидируем полученный ответ
    	current_state->current_pos = 0;
        return;
    }
    if (current_state->current_pos <= 3)
        return;

    // теперь надо проверить приемный буфер на наличие либо заветных \r\r\n (признак конца эхо отправленной команды
    // если включен соответствующий режим)
    // либо заветных \r\n (признака конца ответа полученного от модуля SIM800)
    if( strstr(&current_state->rec_buf[current_state->current_pos - 3], "\r\r\n") )
    {

    	process_echo(current_state->is_responce, current_state->current_pos, current_state);
    	return;
    }
    process_cmd(current_state->is_responce, current_state->current_pos, current_state);
}

//функция парсинга внезапных сообщений от SIM800 (например пришла SMS)
void unexpec_message_parse(uint8_t * unexp_responce)
{

}

// Функция отправки SMS с модуля SIM800
//uint8_t sim800_sendSMS(uint8_t* text_buf, uint8_t length)
//{
//
//}
