//Includes ------------------------------------------------------------------*/

#include "main.h"
#include "phisic.h"
#include "SIM800.h"
#include "flash.h"
#include "REG74HC165.h"
#include "GSMcommunication.h"

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
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
	volatile int i,j;
//	uint8_t temp[32];

	Sys_Init(); // первоначальная инициализация системы

	GSM_Com_Init(&state_of_sim800_num1); // инициализация коммуникационного интерфейса GSM

	FLASH_Write_Default_String(); // запись в последнии страницы флеш памяти дефолтных строк текстовых сообщений SMS если это еще не сделано

	FLASH_Write_Msg_String(11, 0, "Hello0", strlen("Hello0")+1); // запись во флеш тестового SMS сообщения
	FLASH_Write_Msg_String(11, 1, "Hello1", strlen("Hello1")+1); // запись во флеш тестового SMS сообщения

	FLASH_Write_Phone_Num(0, "89649955199", strlen("89649955199")); // запись телефонного номера в телефонную книгу
	FLASH_Write_Phone_Num(1, "89061536606", strlen("89061536606")); // запись телефонного номера в телефонную книгу
	FLASH_Write_Phone_Num(2, "89878145441", strlen("89878145441")); // запись телефонного номера в телефонную книгу

	// инициализация первого SIM800
	sim800_init(&state_of_sim800_num1, send_str_uart2, 2, 6239); // Первый SIM800 сидит на UART2
	// запуск системного таймера надо производить только после настройки SIM800

//    if ((state_of_sim800_num1.communication_stage == proc_completed))
//    {
//    	GPIOA->ODR &= ~GPIO_Pin_0;
//    }

//	while (state_of_sim800_num1.communication_stage != proc_completed) // ждем пока не ответит OK
//	{
//	}
//	sim800_ATplusCMGS_request(&state_of_sim800_num1, "+79649955199", "TEST1"); // отправка SMS
//	while (state_of_sim800_num1.communication_stage != proc_completed) // ждем пока не ответит OK
//	{
//	}
//	sim800_ATplusCMGS_request(&state_of_sim800_num1, "+79649955199", "TEST2"); // отправка SMS
//	while (state_of_sim800_num1.communication_stage != proc_completed) // ждем пока не ответит OK
//	{
//	}
//	sim800_ATplusCMGS_request(&state_of_sim800_num1, "+79649955199", "TEST3"); // отправка SMS

	Init_SysTick(); // разрешаем работу системного таймера



//!!!!!!!!!!!!!!!!!! Разкоменть
//	FLASH_Write_Default_Config(); // запись в пятую с конца страницу дефолтной конфигурации цифровых входов

//	uint8_t test_byte = 3;
//	uint8_t temp_byte;
//	FLASH_Write_Byte(start_DATA_Page_59, 1, test_byte); // пример записи одного байта в произвольную страницу флеш памяти по произвольному смещению
//	FLASH_Read_Byte(start_DATA_Page_59, 1, &temp_byte); // пример чтения одного байта из произвольний страницы флеш памяти по произвольному смещению
//	FLASH_Write_Config_Byte(3, test_byte); // пример записи одного байта в конфигурационную страницу флеш памяти по произвольному смещению
//	FLASH_Read_Config_Byte(3, &temp_byte); // пример чтения одного байта из конфигурационной страницы флеш памяти по произвольному смещению
//	if (temp_byte == 3)
//	{
//		GPIOA->ODR &= ~GPIO_Pin_0; //for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
//	}

//	FLASH_Write_String(start_DATA_Page_60, 0, "888888", strlen("888888"));
//	FLASH_Write_Phone_Num(0, "89649955199", strlen("89649955199")); // запись телефонного номера в телефонную книгу
//	FLASH_Write_Phone_Num(0, "89198364844", strlen("89198364844"));
//	FLASH_Write_Phone_Num(2, "89658894144", strlen("89658894144"));
//
//	FLASH_Read_Phone_Num(1, temp, 32); // чтение телефонного номера из телефонной книги
//
//	if (strstr(temp,"89198364844")) // проверка функции чтения из флеш FLASH_Read_String
//	{
//		GPIOA->ODR &= ~GPIO_Pin_0; //for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
//	}

//	FLASH_Write_Msg_String(start_DATA_Page_61, 1, "Hello", strlen("Hello")+1); // запись во флеш тестового SMS сообщения
//
//	FLASH_Read_Msg_String(start_DATA_Page_61, 1, temp, 32);
//
//	if (strstr(temp,"Hello")) // проверка функции чтения из флеш FLASH_Read_String
//	{
//		GPIOA->ODR &= ~GPIO_Pin_0; //for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
//	}




//	while (state_of_sim800_num1.communication_stage != proc_completed) // ждем пока не ответит OK
//	{
//
//	}
//  sim800_ATplusCMGS_request(&state_of_sim800_num1, "+79649955199", "TEST TEST TEST"); // отправка SMS
//	while (state_of_sim800_num1.communication_stage != proc_completed) // ждем пока не ответит OK
//	{
//
//	}
//	sim800_ATplusCMGS_request(&state_of_sim800_num1, "+79649955199", "TEST TEST TEST"); // отправка SMS


//	sim800_AT_request(&state_of_sim800_num1); // пробуем отправить тестовую команду, заодно настроив скорость передачи по UART
//
//	for(i=0;i<0x1000000;i++);
//	{
//	    for(j=0;j<0x500000;j++);
//	}
//
//    sim800_ATplusCPIN_request(&state_of_sim800_num1, 6239); // вводим PIN-код
//
//	for(i=0;i<0x1000000;i++);
//	{
//	    for(j=0;j<0x500000;j++);
//	}
//    sim800_ATplusCGATTequal1_request(&state_of_sim800_num1); // регистрируемся в GPRS сети
//	for(i=0;i<0x1000000;i++);
//	{
//	    for(j=0;j<0x500000;j++);
//	}
//    sim800_ATplusCIPRXGETequal1_request(&state_of_sim800_num1); // переключаемся в режим ручного приема данных GPRS
//	for(i=0;i<0x1000000;i++);
//	{
//	    for(j=0;j<0x500000;j++);
//	}
//	sim800_ATplusCIPMUX_request(&state_of_sim800_num1, single_connection); // переключаемся в режим одиночного подключения по GPRS
//	for(i=0;i<0x1000000;i++);
//	{
//	    for(j=0;j<0x500000;j++);
//	}
//	if (state_of_sim800_num1.mobile_operator_SIM2 == MTS) // Проверка текущего оператора на SIM-карте
//	{
//		GPIOA->ODR &= ~GPIO_Pin_0; //for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!;
//	}
//	if (state_of_sim800_num1.current_SIM_card == 2) // проверка текущей SIM-карты
//	{
//		GPIOA->ODR &= ~GPIO_Pin_0; //for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!;
//	}
//    sim800_ATplusCSTT_request(&state_of_sim800_num1);
//	for(i=0;i<0x1000000;i++);
//	{
//	    for(j=0;j<0x500000;j++);
//	}
//    sim800_ATplusCIICR_request(&state_of_sim800_num1);
//	for(i=0;i<0x1000000;i++);
//	{
//	    for(j=0;j<0x500000;j++);
//	}
//    sim800_ATplusCIFSR_request(&state_of_sim800_num1);

//	for(i=0;i<0x1000000;i++);
//	{
//	    for(j=0;j<0x500000;j++);
//	}
//	sim800_ATplusCSPNquestion_request(&state_of_sim800_num1); // запрос оператора SIM-карты
//
//    sim800_ATplusCREGquestion_request(&state_of_sim800_num1); // отправка запроса о регистрации в сети
//
//        	for(i=0;i<0x1000000;i++);
//        	{
//        	    for(j=0;j<0x500000;j++);
//        	}
//
//        	if (state_of_sim800_num1.current_registration_state == 1) // проверка ответа на запрос о регистрации в сети
//        	{
//        		int j; GPIOA->ODR &= ~GPIO_Pin_0; //for(j=0;j<0x50000;j++); GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
//        	}


//
//        sim800_ATplusCMGD_request(&state_of_sim800_num1, 1, 4); // удаление всех SMS
//
//    	for(i=0;i<0x1000000;i++);
//    	{
//    	    for(j=0;j<0x500000;j++);
//    	}
//

//    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//    sim800_ATplusCMGS_request(&state_of_sim800_num1, "+79649955199", "TEST!"); // отправка SMS
//    for(i=0;i<0x2000000;i++);
//   	{
//        for(j=0;j<0x500000;j++);
//   	}



//
//    sim800_ATplusCMGR_request(&state_of_sim800_num1, 1, 0); // чтение SMS под номером 1
//
//    for(i=0;i<0x1000000;i++);
//    {
//        for(j=0;j<0x500000;j++);
//    }
//
//    sim800_ATplusCMGS_request(&state_of_sim800_num1, "+79649955199", state_of_sim800_num1.rec_SMS_data); // отправка ответного SMS
//    //sim800_ATplusCMGS_request(&state_of_sim800_num1, "+79198364844", state_of_sim800_num1.rec_SMS_data); // отправка ответного SMS
//
//    //    if (strstr(state_of_sim800_num1.rec_SMS_data,"KAS"))
//    //    {
//    //    	GPIOA->ODR &= ~GPIO_Pin_0; // ОТЛАДКА!!!
//    //    }
//
//    for(i=0;i<0x1000000;i++);
//    {
//        for(j=0;j<0x500000;j++);
//    }
//
//    sim800_ATplusCUSD_request(&state_of_sim800_num1, MTS_balance_request); // проверка баланса SIM карты


	while(1)
    {

//    	if (reg74hc165_current_state_num1.arr_res[12].status.cur_phis_state == 0) // Пример чтения физического состояния входов (если соответствующий вход в нуле то загорается светодиод)
//        {
//    		GPIOA->ODR &= ~GPIO_Pin_0; // ОТЛАДКА!!!
//        }
//    	else
//    	{
//    		GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
//    	};

//    	if (reg74hc165_current_state_num1.arr_res[12].status.cur_log_state == 0) // Пример чтения логического состояния входов (если соответствующий вход в нуле то загорается светодиод)
//        {
//    		GPIOA->ODR &= ~GPIO_Pin_0; // ОТЛАДКА!!!
//        }
//    	else
//    	{
//    		GPIOA->ODR |= GPIO_Pin_0; // ОТЛАДКА!!!
//    	};

    }
}
