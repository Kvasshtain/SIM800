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

extern uint8_t sys_timer_stop; // признак для остановки вызовов внутри обработчика системного таймера
// перменная требуется в случае выполнения критичесого (долгого и ресурсозатратного) кода, например записи во флеш конфигурационных пользовательских данных

void SetupClock(void);  // Функция настройки тактирования
void SetupUSART1(void); // Функция настройки UART1
void SetupUSART2(void); // Функция настройки UART2
void send_str_uart1(char * string); // отправка данных в UART1
void send_str_uart2(char * string); // отправка данных в UART2
void InitADC(void);     // Функция настройки АЦП
void SetupGPIO(void);   // функция инициализации пользовательских GPIO (например управляющих светодиодом)
void Init_SysTick(void); // Настройка сиситемного таймера

#endif
