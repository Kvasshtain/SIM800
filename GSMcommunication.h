// Функции для передачи и приема данных по интерфейсу GSM

#ifndef __GSM_COM_H_
#define __GSM_COM_H_

#include "SIM800.h"
#include "flash.h"

#define busy 1
#define free 0

// общение с GSM модулем осуществляется в режиме вопрос/ответ и между запросом и ответом может пройти некоторое время
// причем запрос на выполнение операции может завершиться неудачей и потребуется перезапрос, поэтому ввобятся флаги
// для определения начала и конца конкретного запроса

#define SMS_send_start  1 // признак начала отправки SMS
#define SMS_send_stop   0 // признак конца отправки SMS(когда GSM модуль ответит, что SMS точно ушла)

#define SMS_rec_start   1 // признак начала чтения SMS
#define SMS_rec_stop    0 // признак конца чтения SMS(когда GSM модуль пришлет читаемую SMS)

#define SMS_del_start   1 // признак начала удаления SMS
#define SMS_del_stop    0 // признак конца удаления SMS


#define MAX_NUM_OF_FAIL 3 // максимальное число повторных попыток при неудачном исходе запроса в GSM модуль

#define NUM_OF_ABONENTS 6 // NUM_OF_ABONENTS должно быть меньше MAX_NUM_OF_ABONENTS = 32

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
    uint8_t send_SMS_text[SEND_SMS_DATA_SIZE]; // текущий текс SMS для отправки
    //uint8_t rec_SMS_text[REC_SMS_DATA_SIZE];   // текущий текс принятого SMS //!!! Данный буфер пока не нужен, вполне хватит приемного буфера драйвера SIM800 (или другого модуля)
    uint8_t phone_num[MAX_SIZE_STR_PHONE_8];   // номер телефона текущего абонента
    volatile uint8_t number_of_failures;                // счетчик неудачных попыток (используется для защиты от зацикливания)
};

extern struct GSM_communication_state GSM_com_state; // структура хранящая текущее состояние коммуникационного GSM итерфеса

void GSM_Com_Init(struct sim800_current_state * current_state); // функция инициализации коммуникационного интерфейса

void SMS_parse(void); // Функция парсинга приходящих SMS - сообщений

void sendSMS(void); // Функция отправки SMS

void recSMS(void); // Функция обработки принятых SMS

void Dig_Signals_Check(void); // Функция проверки состояния цифровых входов и рассылки сообщений

void Analog_Signals_Check(void); // функция проверки состояния аналоговых входов и рассылки сообщений

void GSM_Communication_routine(void); // главная коммуникационная функция GSM

void SMS_parse(void); // Функция парсинга приходящих SMS - сообщений

#endif
