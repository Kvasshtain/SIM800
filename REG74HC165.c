// Драйвер регистров 74HC165 с параллельным входом и последовательным выходом включенных каскадно
#include <stdio.h>
#include "phisic.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "REG74HC165.h"

struct reg74hc165_current_state reg74hc165_current_state_num1; // регистровых каскадов может быть несколько

// Функция вызываемая из обработчика прерывания таймера производящая побитное чтение данных с выхода 74hc165
void load_data74HC165(struct reg74hc165_current_state * current_state)
{
	if (CP_PIN_STATE == 0) // если на линии клока 0, то ставим 1 и ждем следующего раза
	{
		CP_PIN_UP;
		return;
	}
	CP_PIN_DOWN;
	current_state->input_data <<= 1; // сдвигаем данные влево
	QH_PIN_STATE ? (current_state->input_data |= 1) : (current_state->input_data |= 0);
	current_state->current_bit++;
	if (current_state->current_bit == NUM_BIT)
	{
		current_state->current_bit = 0;
		current_state->result = current_state->input_data;
	}
    return;
}
