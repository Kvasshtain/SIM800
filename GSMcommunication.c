// Функции для передачи и приема данных по интерфейсу GSM
#include <stdio.h>
#include "stm32f10x_gpio.h"

#include "flash.h"
#include "REG74HC165.h"
#include "SIM800.h"
#include "GSMcommunication.h"

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
    uint8_t rec_SMS_text[REC_SMS_DATA_SIZE];   // текущий текс принятого SMS
    uint8_t phone_num[MAX_SIZE_STR_PHONE_8];   // номер телефона текущего абонента
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
    	//GPIOA->ODR &= ~GPIO_Pin_0; // ОТЛАДКА!!!;
    	GSM_com_state.current_read_SMS++; // следующий абонент
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

    	if ((reg74hc165_current_state_num1.arr_res[i].status.is_meander == 1) &&  // если на одном из входов появился меандр или одиночный импульс
            (reg74hc165_current_state_num1.arr_res[i].status.meandr_already_sent == 0)) // и об этом еще не разослано SMS-сообщение
	    {
			if (i < NUM_OF_INPUT_SIGNAL)  // обязательно проверяем есть ли у данного входного сигнала строка во флеш
   		    {
   		        FLASH_Read_Msg_String(i, 1, temp_str, MAX_SIZE_STRING_8);
   		    }
   		    if (strlen(GSM_com_state.send_SMS_text) + strlen(temp_str) + 5 < SEND_SMS_DATA_SIZE) // 5 - это 2 пробела в конце + 1 нулевой символ + 2-а прозапас
   		    {
       		    reg74hc165_current_state_num1.arr_res[i].status.meandr_already_sent = 1;   // помечаем соответсвующий выход как отправленный на рассылку
       		    // что бы SMS разослалось один раз
       		    strcat(GSM_com_state.send_SMS_text, temp_str);
       		    strncat(GSM_com_state.send_SMS_text, "  ", 3);
   		    }
   	    }

		if ((reg74hc165_current_state_num1.arr_res[i].status.is_const_sig == 1) &&  // если на одном из входов появился постоянный уровень
            (reg74hc165_current_state_num1.arr_res[i].status.const_already_sent == 0)) // и об этом еще не разослано SMS-сообщение
	        {
			if (i < NUM_OF_INPUT_SIGNAL)
		    {
		        FLASH_Read_Msg_String(i, 0, temp_str, MAX_SIZE_STRING_8);
		    }
		    if (strlen(GSM_com_state.send_SMS_text) + strlen(temp_str) + 5 < SEND_SMS_DATA_SIZE)
		    {
    		    reg74hc165_current_state_num1.arr_res[i].status.const_already_sent = 1;
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
