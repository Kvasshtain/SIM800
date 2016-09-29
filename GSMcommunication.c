// Функции для передачи и приема данных по интерфейсу GSM
#include <stdio.h>
#include "stm32f10x_gpio.h"

#include "flash.h"
#include "REG74HC165.h"
#include "SIM800.h"
#include "GSMcommunication.h"

// Структура описывает текущее состояние комуникационного GSM интерфейса
struct GSM_communication_state{
    uint8_t Status_of_mailing;                 // статус рассылки: занят (busy)- рассылка сообщения идет,
    //                  свободен (free) - рассылка сообщения закончена,
    uint8_t status_mes_send;                   // флаг статуса отправки SMS может принимать SMS_send_stop = 0 или SMS_send_start = 1
    uint8_t current_abonent;                   // тикущий номер абонета
    uint8_t max_num_of_abonent;                // максимальное число абонентов для рассылки
    uint8_t SMS_text[MAX_SIZE_STRING_8];       // текущий текс SMS для отправки
    uint8_t phone_num[MAX_SIZE_STR_PHONE_8];   // номер телефона текущего абонента
};

struct GSM_communication_state GSM_com_state; // структура хранящая текущее состояние коммуникационного GSM итерфеса

// функция инициализации коммуникационного интерфейса
void GSM_Com_Init(struct sim800_current_state * GSMmod)
{
    GSM_com_state.SMS_text[0] = '\0';
    uint8_t status_mes_send = SMS_send_stop;
    GSM_com_state.phone_num[0] = '\0';
    GSM_com_state.Status_of_mailing = free;
    GSM_com_state.current_abonent = 0;
    GSM_com_state.max_num_of_abonent = NUM_OF_ABONENTS;
}

// Функция отправки SMS
// Функция пытается его отправить используя соответсвующую низкоуровневую функцию
void sendSMS(void)
{
    do
    {
        FLASH_Read_Phone_Num(GSM_com_state.current_abonent, GSM_com_state.phone_num, MAX_SIZE_STR_PHONE_8);
        GSM_com_state.current_abonent++;
        if (GSM_com_state.current_abonent == GSM_com_state.max_num_of_abonent) // если пробежались по всем ячейкам флеш памяти и телефонных номеров больше нет
        {
            GSM_com_state.current_abonent = 0;
            GSM_com_state.Status_of_mailing = free;
            return;
        }
    }
    while (GSM_com_state.phone_num[0] == 0xFF);
    GSM_com_state.current_abonent--;

    if ((state_of_sim800_num1.communication_stage == proc_completed)     // если GSM-модуль не занят обработкой предыдущего запроса и
            && (GSM_com_state.status_mes_send == SMS_send_stop))         // мы еще не начали отсылать SMS
    {
        GSM_com_state.status_mes_send = SMS_send_start; // отправили запрос на отправку SMS
        sim800_ATplusCMGS_request(&state_of_sim800_num1, GSM_com_state.phone_num, GSM_com_state.SMS_text);

        return;
    }

    if (state_of_sim800_num1.communication_stage == proc_completed) // если GSM-модуль не занят обработкой предыдущего запроса
    {
        GPIOA->ODR ^= GPIO_Pin_0;
        GSM_com_state.status_mes_send = SMS_send_stop;
        if (state_of_sim800_num1.result_of_last_execution == OK)
        {
            GSM_com_state.current_abonent++; // следующий абонент
        }
        return;
    }

    return;
}

// Функция обработки принятых SMS
uint8_t recSMS(void)
{
    static uint8_t recSMSstatus; // флаг ожидания прихода запрошенного SMS-сообщения
    if ((recSMSstatus == free) && (state_of_sim800_num1.communication_stage == proc_completed)) // если SMS еще не читалось и модуль завершил обработку всех предидущих запросов
    {
        recSMSstatus = busy;
        sim800_ATplusCMGR_request(&state_of_sim800_num1, 1, 0); // чтение SMS под номером 1 !!!!!!!!!!!!!!!!!!!!!
    }
    else if (state_of_sim800_num1.communication_stage == proc_completed)
    {

        // Обработа принятого SMS
        //!!!!!!!!!!GSM_com_state.sim800module->rec_SMS_data;
        if (stristr(state_of_sim800_num1.rec_SMS_data,"BLA-BLA-BLA")) // Пришло СМС сообщение с текстом BLA-BLA-BLA
        {
            // !!!!! вызываем обработчик
        }
        else if (stristr(state_of_sim800_num1.rec_SMS_data,"helo-helo-helo")) //
        {
            // !!!!! вызываем обработчик
        }
        else
        {
            return; // на все остальные SMS-сообщения не обращаем внимание
        }
        recSMSstatus = free;
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

    GSM_counter++; // вызыв алгоритма обработчика происходит изредка для подавления дребезга контактов и
    if (GSM_counter < 300) // еще не пришла очередь ни чего не делаем
    {
        return;
    }
    GSM_counter = 0; // сбрасываем счетчик задержки вызова

    if (state_of_sim800_num1.Status == not_ready) // если обслуживающий нас модуль не готов ни чего не делаем
    {
        return;
    }

    //	if ((GSM_com_state.Status_of_ == free) && (GSM_com_state.sim800module->num_of_sms != 0)) // если есть непрочитанное SMS
    //	{
    //		recSMS();
    //		return;
    //	}

    if (GSM_com_state.Status_of_mailing == busy) // если случилось что-то, что требует рассылки SMS-сообщений или GSM-модуль занят обработкой предидущего запроса
    {

        sendSMS();
        return;
    }

    if (GSM_com_state.Status_of_mailing == free) // если рассылка предидущего SMS-сообщения и обработка запроса на чтение SMS-сообщения закончены
    {
        if ((reg74hc165_current_state_num1.arr_res[cur_dig_input].status.cur_log_state == 1) &&  // если на одном из входов случилась активное состояние
                (reg74hc165_current_state_num1.arr_res[cur_dig_input].status.already_sent == 0))     // и об этом еще не разослано SMS-сообщение
        {
            GSM_com_state.Status_of_mailing = busy; // начинаем рассылку
            reg74hc165_current_state_num1.arr_res[cur_dig_input].status.already_sent = 1;   // помечаем соответсвующий выход как отправленный на рассылку (что бы SMS разослалось один раз)
            FLASH_Read_Msg_String(cur_dig_input, GSM_com_state.SMS_text, MAX_SIZE_STRING_8);

        }
        else if (reg74hc165_current_state_num1.arr_res[cur_dig_input].status.cur_log_state == 0) // если на входе нет активного состояния
        {
            reg74hc165_current_state_num1.arr_res[cur_dig_input].status.already_sent = 0; // сбрасываем признак разосланного сообщения в ноль
        }
        cur_dig_input++;
        if (cur_dig_input == NUM_OF_INPUT)
        {
            cur_dig_input = 0;
        }
        //проверяем входы АЦП
    }

}
