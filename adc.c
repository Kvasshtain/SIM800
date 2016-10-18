#include "stm32f10x.h"
#include "adc.h"
#include "stm32f10x_gpio.h"

struct ADC_current_state ADC_current_state_num1; // АЦП может быть несколько

// инициализация процесса оцифровки сигналов на входах АЦП
void ADC_init_routine(struct ADC_current_state *current_state)
{
    uint8_t i;
    current_state->current_channel = FIRST_CHANNEL;
    for(i = 0; i < NUM_OF_ADC_CHANNEL; i++)
    {
        current_state->result[i].value = 0;
        current_state->result[i].analog_ch_status.is_normal = 0;
        current_state->result[i].analog_ch_status.decr_th_already_sent = 1; //что-бы не рассылалось сообщение об отсутствии напряжений при первом запуске
        current_state->result[i].analog_ch_status.incr_th_already_sent = 0;
        current_state->temperature = 0;
        current_state->state = ADC_NOT_READY;
    }

    // назначаем пороги
    current_state->result[0].decrease_thteshold = DECREASE_THRESHOLD_MAIN_VOLT;
    current_state->result[0].increase_thteshold = INCREASE_THRESHOLD_MAIN_VOLT;
    current_state->result[1].decrease_thteshold = DECREASE_THRESHOLD_EXT_BAT;
    current_state->result[1].increase_thteshold = INCREASE_THRESHOLD_EXT_BAT;
    current_state->result[2].decrease_thteshold = DECREASE_THRESHOLD_BACKUP;
    current_state->result[2].increase_thteshold = INCREASE_THRESHOLD_BACKUP;
    current_state->result[3].decrease_thteshold = DECREASE_THRESHOLD_BAT;
    current_state->result[3].increase_thteshold = INCREASE_THRESHOLD_BAT;

    // запускаем первое преобразование
    //ADC1->SQR3 = current_state->current_channel;
    //ADC1->CR2 |= ADC_CR2_SWSTART;
    return;
}

// функция запуска преобразования на одном из входов АЦП (может вызываться например из прерывания системного таймера)
void ADC_conversion_start(struct ADC_current_state *current_state)
{ 
    static uint16_t ADC_counter; // счетчик задержки работы функции запуска АЦП
    ADC_counter++;
    if (ADC_counter < 100)
    {
        return;
    }
    ADC_counter = 0;

    ADC1->SQR3 = current_state->current_channel;
    ADC1->CR2 |= ADC_CR2_SWSTART;
    return;
}

// функция проверки напряжения на всех внешних и внутренних источниках питания
void PWR_check(struct ADC_current_state *current_state)
{
    uint8_t i;
    for (i = 0; i < NUM_OF_ADC_CHANNEL; i++)
    {
        if ((current_state->result[i].value < current_state->result[i].decrease_thteshold) && // если напряжение упало ниже порога
                (current_state->result[i].analog_ch_status.is_normal))                        // и до этого напряжение было нормальным
        {
            current_state->result[i].analog_ch_status.is_normal = 0;
            current_state->result[i].analog_ch_status.incr_th_already_sent = 0;
        }

        if ((current_state->result[i].value > current_state->result[i].increase_thteshold) && // если напряжение выросло выше порога
                (!current_state->result[i].analog_ch_status.is_normal))                       // и до этого напряжение было понижанным
        {
            current_state->result[i].analog_ch_status.is_normal = 1;
            current_state->result[i].analog_ch_status.decr_th_already_sent = 0;
        }
    }
}

// Функция вызывается из обработчика прерывания АЦП
// принимает значение полученное в ходе преобразования
void ADC_processing(struct ADC_current_state *current_state, uint16_t value)
{
    switch (current_state->current_channel)
    {
    case FIRST_CHANNEL:
    {
        current_state->result[0].value = value;
        break;
    }
    case FIRST_CHANNEL+1:
    {
        current_state->result[1].value = value;
        break;
    }
    case FIRST_CHANNEL+2:
    {
        current_state->result[2].value = value;
        break;
    }
    case LAST_CHANNEL:
    {
        current_state->result[3].value = value;
        break;
    }
    default:
        break;
    }
    ++current_state->current_channel;
    if (current_state->current_channel > LAST_CHANNEL)
    {
        current_state->current_channel = FIRST_CHANNEL;
        PWR_check(current_state);
    }
}
