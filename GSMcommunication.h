// Функции для передачи и приема данных по интерфейсу GSM

#ifndef __REG74HC165_H_
#define __REG74HC165_H_

uint8_t sim800_sendSMS(uint8_t* text_buf, uint8_t length); // Функция отправки SMS
uint8_t sim800_recSMS(uint8_t* text_buf, uint8_t length); // Функция обработки принятых SMS

#endif
