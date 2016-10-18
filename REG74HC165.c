// Драйвер регистров 74HC165 с параллельным входом и последовательным выходом включенных каскадно
#include <stdio.h>
#include "phisic.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "REG74HC165.h"
#include "flash.h"

struct reg74hc165_current_state reg74hc165_current_state_num1; // регистровых каскадов может быть несколько

// функция сохраниеия конфигурации во флеш
void save_config_74HC165(struct reg74hc165_current_state * current_state)
{
    uint8_t i;
    uint8_t tmpbuf[NUM_OF_INPUT];
    for(i = 0; i < NUM_OF_INPUT; i++)
    {
        tmpbuf[i] = current_state->arr_res[i].config.i8;
    }
    FLASH_Write_Config_Page(tmpbuf, NUM_OF_INPUT);
}

// функция чтения конфигурации из флеш
void load_config_74HC165(struct reg74hc165_current_state * current_state)
{
    uint8_t i;
    for(i = 0; i < NUM_OF_INPUT; i++)
    {
        current_state->arr_res[i].config.i8 = FLASH_Read_Config_Byte(i);
    }
}

// Функция инициализации опроса регистра/регистров 74HC165
// вычитывает сохраненые во флеш данные конфигурации входов и применяет их к текущей конфигурации
void init_74HC165(struct reg74hc165_current_state * current_state)
{
    uint8_t i;

    current_state->current_bit = 0;
    current_state->stage = pl_low;

    load_config_74HC165(current_state);

    for(i = 0; i < NUM_OF_INPUT; i++)
    {
        current_state->arr_res[i].pause_duration = 0;
        current_state->arr_res[i].pulse_duration = 0;

        current_state->arr_res[i].status.bf.const_already_sent = 0;
        current_state->arr_res[i].status.bf.cur_log_state = 0;
        current_state->arr_res[i].status.bf.is_const_sig = 0;
        current_state->arr_res[i].status.bf.is_meander = 0;
        current_state->arr_res[i].status.bf.meandr_already_sent = 0;
        current_state->arr_res[i].status.bf.cur_phis_state = ~current_state->arr_res[i].config.bf.alarm_state; // что-бы при старте не были выданы ложные срабатывания
    }
}

// Определяет: импульс - это меандр
void meander_recognition(struct reg74hc165_current_state * current_state, int input_num)
{
    if ((current_state->arr_res[input_num].pulse_duration > NOICE_DURATION) /*&& (current_state->arr_res[input_num].pulse_duration < THRESHOLD_DURATION)*/)
    {
        current_state->arr_res[input_num].status.bf.is_meander = 1;
    }
}

// Определяет: импульс - это постоянный уровень, который держится более 2 секунды
void const_sig_recognition(struct reg74hc165_current_state * current_state, int input_num)
{
    if (current_state->arr_res[input_num].pulse_duration >= THRESHOLD_DURATION)
    {
        current_state->arr_res[input_num].status.bf.is_const_sig = 1;
    }
}

// Определяет: проподание сигнала
//             или импульс - это постоянный уровень, который держится более 1 секунды
void vanishing_recognition(struct reg74hc165_current_state * current_state, int input_num)
{
    if ((current_state->arr_res[input_num].pause_duration > THRESHOLD_DURATION))
    {
        current_state->arr_res[input_num].status.bf.const_already_sent = 0;
        current_state->arr_res[input_num].status.bf.meandr_already_sent = 0;
        current_state->arr_res[input_num].status.bf.is_const_sig = 0;
        current_state->arr_res[input_num].status.bf.is_meander = 0;
    }
}

// Функция обработыает логическое состояния цифровых входов
// а так же обеспечивает фильтрацию импульсов (например коротких выбросов)
void pulse_processing(struct reg74hc165_current_state * current_state)
{
    uint8_t i;
    for (i = 0; i < NUM_OF_INPUT; i++)
    {
        if (current_state->arr_res[i].status.bf.cur_log_state)
        {
            current_state->arr_res[i].pulse_duration ++;
            current_state->arr_res[i].pause_duration = 0;
            const_sig_recognition(current_state, i);
        }

        if (!current_state->arr_res[i].status.bf.cur_log_state)
        {
            current_state->arr_res[i].pause_duration ++;
            meander_recognition(current_state, i);
            current_state->arr_res[i].pulse_duration = 0;
            vanishing_recognition(current_state, i);
        }
    }
}

// Функция вызываемая из обработчика прерывания таймера производящая побитное чтение данных с выхода 74hc165
void load_data74HC165(struct reg74hc165_current_state * current_state)
{
    pulse_processing(current_state);

    if (current_state->stage == pl_low)
    {
        PL_PIN_DOWN;
        CP_PIN_UP;
        current_state->stage = cp_low;
        return;
    }

    if (current_state->stage == cp_low)
    {
        PL_PIN_UP;
        CP_PIN_UP;
        current_state->stage = cp_high;
        return;
    }

    if (current_state->stage == cp_high)
    {
        PL_PIN_UP;
        CP_PIN_DOWN;
        current_state->stage = qh_ready;
        return;
    }

    if (current_state->stage == qh_ready)
    {
        PL_PIN_UP;
        CP_PIN_DOWN;

        if (QH_PIN_STATE) // Вычитываем текущий бит
        {
            current_state->arr_res[NUM_OF_INPUT - 1 - current_state->current_bit].status.bf.cur_phis_state = 1;
        }
        else
        {
            current_state->arr_res[NUM_OF_INPUT - 1 - current_state->current_bit].status.bf.cur_phis_state = 0;
        }

        // устанавливаем логическое состояние в зависимости от физического и что считать активным
        if (current_state->arr_res[NUM_OF_INPUT - 1 - current_state->current_bit].config.bf.alarm_state)
        {
            current_state->arr_res[NUM_OF_INPUT - 1 - current_state->current_bit].status.bf.cur_log_state =
                    current_state->arr_res[NUM_OF_INPUT - 1 - current_state->current_bit].status.bf.cur_phis_state;
        }
        else
        {
            current_state->arr_res[NUM_OF_INPUT - 1 - current_state->current_bit].status.bf.cur_log_state =
                    ~current_state->arr_res[NUM_OF_INPUT - 1 - current_state->current_bit].status.bf.cur_phis_state;
        }

        current_state->current_bit++;

        if (current_state->current_bit == (NUM_OF_INPUT))
        {
            current_state->stage = pl_low;
            current_state->current_bit = 0;
            return;
        }

        current_state->stage = cp_low;
        return;
    }

    return;
}
