#include <stdio.h>
#include "phisic.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"

// Отправка данных в UART на котором сидит SIM800
PutsToSim800(char * cmd)
{
	send_str_uart2rn(cmd); // выводим данные в UART с добавлением завершающих символов \n\r
}

// Функция отправки команды в модуль Sim800
void Sim800_WriteCmd (const char *cmd)
{
    PutsToSim800(cmd); // Отправка данных в UART на котором сидит SIM800
}

// Функция отправки команды в модуль Sim800 с проверкой ответа (блокирующийся вариант)
void Sim800_WriteCmd_confirm (const char *cmd)
{
    PutsToSim800(cmd); // Отправка данных в UART на котором сидит SIM800
    return Sim900_CompareStr("\r\nOK"); /// Функция чтения данных с порта UART с парсингом ответа
}

// Функция инициализации модуля SIM800
uint8_t sim800_init(uint8_t init_data)
{

}

// Функция отправки SMS с модуля SIM800
uint8_t sim800_sendSMS(uint8_t* text_buf, uint8_t length)
{

}

// Функция включения модуля SIM800
void sim800_PWRKEY_on(void)
{
	// Подаем на порт PWRKEY лог. 0
	GPIO_WriteBit (GPIO_SIM800_PWRKEY, SIM800_PWRKEY, Bit_RESET);
	// Задержка в мс., необходимая для включения
	DelayMs(2000);

	// Подаем на порт PWRKEY лог. 1
	GPIO_WriteBit (GPIO_SIM800_PWRKEY, SIM800_PWRKEY, Bit_SET);
	// Ожидаем включение/выключение SIM800
	DelayMs(4000);

}
