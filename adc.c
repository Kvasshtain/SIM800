#include "stm32f10x.h"
#include "adc.h"

uint16_t ReadADC(uint8_t numb)
{ 
    ADC1->SQR3 = numb;
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while(!(ADC1->SR & ADC_SR_EOC)){};
    return ADC1->DR;
}

//uint16_t ReadADC(uint8_t numb)
//{
//    ADC1->SQR3 = numb;
//    ADC1->CR2 |= ADC_CR2_SWSTART;
//    while(!(ADC1->SR & ADC_SR_EOC)){};
//    return ADC1->DR;
//}
