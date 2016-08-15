//Includes ------------------------------------------------------------------*/

#include "main.h"
#include "phisic.h"

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include <stdio.h>

//**********************************************************************************************************
void Sys_Init(void) // функция первоначальной инициализации системы
{
	SetupClock();
	SetupUSART1();
	SetupUSART2();
	InitADC();
}

int main(void)
{
	Sys_Init(); // первоначальная инициализация системы

    while(1)
    {
    }
}
