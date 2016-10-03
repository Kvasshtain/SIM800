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


#define MAX_NUM_OF_FAIL 5 // максимальное число повторных попыток при неудачном исходе запроса в GSM модуль

#define NUM_OF_ABONENTS 6 // NUM_OF_ABONENTS должно быть меньше MAX_NUM_OF_ABONENTS = 32

//#define SAVE_TEL_CMD_TXT = "tel";
//#define SAVE_TEL_CMD_LENG (sizeof(SAVE_TEL_CMD)-1)
//
//#define SAVE_ALARM_T1_CMD_TXT = "vhod text1 ";
//#define SAVE_ALARM_T1_CMD_LENG (sizeof(SAVE_ALARM_T1_CMD)-1)
//
//#define SAVE_ALARM_T2_CMD_TXT = "vhod text2 ";
//#define SAVE_ALARM_T2_CMD_LENG (sizeof(SAVE_ALARM_T2_CMD)-1)
//
//#define SAVE_ACT_STATE_CMD_TXT = "akt sost vhod";
//#define SAVE_ACT_STATE_CMD_LENG (sizeof(SAVE_ACT_STATE_CMD)-1)

void GSM_Com_Init(struct sim800_current_state * current_state); // функция инициализации коммуникационного интерфейса

void SMS_parse(void); // Функция парсинга приходящих SMS - сообщений

void sendSMS(void); // Функция отправки SMS

void recSMS(void); // Функция обработки принятых SMS

void Dig_Signals_Check(void); // Функция проверки состояния цифровых входов и рассылки сообщений

void GSM_Communication_routine(void); // главная коммуникационная функция GSM

void SMS_parse(void); // Функция парсинга приходящих SMS - сообщений

#endif
