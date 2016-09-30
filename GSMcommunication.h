// Функции для передачи и приема данных по интерфейсу GSM

#ifndef __GSM_COM_H_
#define __GSM_COM_H_

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



#define NUM_OF_ABONENTS 5 // NUM_OF_ABONENTS должно быть меньше MAX_NUM_OF_ABONENTS = 32

void GSM_Com_Init(struct sim800_current_state * current_state); // функция инициализации коммуникационного интерфейса

void sendSMS(void); // Функция отправки SMS

void recSMS(void); // Функция обработки принятых SMS

void Dig_Signals_Check(void); // Функция проверки состояния цифровых входов и рассылки сообщений

void GSM_Communication_routine(void); // главная коммуникационная функция GSM

#endif
