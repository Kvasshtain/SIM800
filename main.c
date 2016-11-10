//Includes ------------------------------------------------------------------*/

#include "main.h"
#include "phisic.h"
#include "SIM800.h"
#include "flash.h"
#include "REG74HC165.h"
#include "adc.h"
#include "GSMcommunication.h"

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_flash.h"
#include <stdio.h>

//**********************************************************************************************************
void Sys_Init(void) // функция первоначальной инициализации системы
{
    SetupClock();
    SetupGPIO();
    SetupUSART1();
    SetupUSART2();
    InitADC();
    FLASH_Unlock();      // Разблокируем запись во FLASH программ
}

int main(void)
{
    Sys_Init(); // первоначальная инициализация системы

    GSM_Com_Init(&state_of_sim800_num1); // инициализация коммуникационного интерфейса GSM

    FLASH_Write_Default_String(); // запись в последнии страницы флеш памяти дефолтных строк текстовых сообщений SMS если это еще не сделано

    FLASH_Write_Default_Config(); // запись дефолтной конфигурации входов

    // инициализация каскада регистров 74HC165
    init_74HC165(&reg74hc165_current_state_num1);

    ADC_init_routine(&ADC_current_state_num1);

    uint8_t current_SIM = 1;

    // инициализация SIM800
    while (sim800_init(&state_of_sim800_num1, send_str_uart2, current_SIM, 0)) // Первый SIM800 сидит на UART2
    {
    	if (current_SIM == 1)
    	{
    	    current_SIM = 2; // Пробуем завестись на одной из двух SIM-карт
    	}
    	else
    	{
		    current_SIM = 1;
		}
    };

    // запуск системного таймера надо производить только после настройки SIM800
    Init_SysTick(); // разрешаем работу системного таймера

    while(1)
    {
        WriteDataInFlash(); // вызываем функцию записи во флешь новых данных, если таковые есть
    }
}
