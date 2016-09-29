#include <stdio.h>

#ifndef PHISIC_H_INCLUDED
#define PHISIC_H_INCLUDED

#define GPIO_SIM800_PWRKEY     GPIOA
#define SIM800_PWRKEY          GPIO_Pin_1
#define SIZE_BUF_UART1         32
#define SIZE_BUF_UART2         32

#define USR_LED_OFF GPIOA->ODR |= GPIO_Pin_0
#define USR_LED_ON  GPIOA->ODR &= ~GPIO_Pin_0

#define F_CPU 		8000000UL
#define F_TICK 		1000              // требуемая частота срабатывания прерывания системного таймера в герцах (например 1000 - 1КГц)
#define TIMER_TICK  F_CPU/F_TICK-1

//extern uint8_t rec_buf_usart1[SIZE_BUF_UART1];  // буфер для принимаемых данных UART1
//extern int8_t rec_buf_last_usart1; // индекс последнего необработанного символа в буфере UART1
//extern uint8_t rec_buf_usart1_overflow; //флаг переполнения приемного буфера

//extern uint8_t rec_buf_usart2[SIZE_BUF_UART2];  // буфер для принимаемых данных UART2
//extern uint8_t rec_buf_last_usart2; // индекс последнего необработанного символа в буфере UART2
//extern uint8_t rec_buf_usart2_overflow; //флаг переполнения приемного буфера

void SetupClock(void);  // Функция настройки тактирования
void SetupUSART1(void); // Функция настройки UART1
void SetupUSART2(void); // Функция настройки UART2
void send_str_uart1(char * string); // отправка данных в UART1
void send_str_uart2(char * string); // отправка данных в UART2
void InitADC(void);     // Функция настройки АЦП
void SetupGPIO(void);   // функция инициализации пользовательских GPIO (например управляющих светодиодом)

#endif
