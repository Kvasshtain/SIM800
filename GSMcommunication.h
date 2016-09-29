// Функции для передачи и приема данных по интерфейсу GSM

#ifndef __GSM_COM_H_
#define __GSM_COM_H_

#define busy 1
#define free 0

#define SMS_send_start  1 // признак начала отправки SMS
#define SMS_send_stop   0 // признак конца отправки SMS(когда GSM модуль ответит, что SMS точно ушла)

#define NUM_OF_ABONENTS 5 // NUM_OF_ABONENTS должно быть меньше MAX_NUM_OF_ABONENTS = 32

void GSM_Com_Init(struct sim800_current_state * current_state); // функция инициализации коммуникационного интерфейса

void GSM_Communication_routine(void); // главная коммуникационная функция GSM

#endif
