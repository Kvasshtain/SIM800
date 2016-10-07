/* Includes ------------------------------------------------------------------*/
#include "phisic.h"

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_adc.h"
#include <stdio.h>
#include <string.h>

#include "misc.h"

#include "SIM800.h"
#include "REG74HC165.h"
#include "GSMcommunication.h"
#include "adc.h"
#include "flash.h"

//uint8_t rec_buf_usart1[SIZE_BUF_UART1];  // буфер для принимаемых данных UART1
//int8_t rec_buf_last_usart1; // индекс последнего необработанного символа в буфере UART1
//uint8_t rec_buf_usart1_overflow; //флаг переполнения приемного буфера

//uint8_t rec_buf_usart2[SIZE_BUF_UART2];  // буфер для принимаемых данных UART2
//uint8_t rec_buf_last_usart2; // индекс последнего необработанного символа в буфере UART2
//uint8_t rec_buf_usart2_overflow; //флаг переполнения приемного буфера

/***************************************************************************//**
 Настройка тактирования
 ******************************************************************************/
void SetupClock(void)
{
    // Настраиваем тактирование АЦП
    RCC_ADCCLKConfig(RCC_PCLK2_Div2);
    /* Enable USART1, USART2 and GPIOA, GPIOB, GPIOC and ADC1 clock */
    RCC_APB2PeriphClockCmd (RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_ADC1 , ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

}

/***************************************************************************//**
 Настраиваем входы и выходы
 ******************************************************************************/
void SetupGPIO(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    /** Configure pins as GPIO
    PC13    ------> GPIO_Output Сигнал PL для дискретных входов
    PC14    ------> GPIO_Output Сигнал CP для дискретных входов
    PC15    ------> GPIO_Input  Сигнал QH от  дискретных входов
    */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //вход с подтяжкой к питанию
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /** Configure pins as GPIO
    PA0     ------> GPIO_Output Сигнал пользовательский светодиод (LED)
    PA15    ------> GPIO_Input  Сигнал Conf0 от DIP-переключателя
    */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //вход с подтяжкой к питанию
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USR_LED_OFF;// тушим пользовательский светодиод в начале работы

    /** Configure pins as GPIO
    PB3 - PB9, PB12 - PB15 ------> GPIO_Input Конфигурация задаваемая DIP-переключателями
    */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //вход с подтяжкой к питанию
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/***************************************************************************//**
 * @brief Init USART1 на нем сидит RS485
 ******************************************************************************/
void SetupUSART1(void)
{

    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    //    memset(rec_buf_usart1, 0, SIZE_BUF_UART1); // обнуляем буфер для принимаемых данных UART1
    //  	rec_buf_last_usart1 = -1;                   // индекс последнего необработанного символа принятого от UART1 устанавливаем в -1


    /** Configure pins as GPIO
    PA8    ------> GPIO_Output направление передачи RS485
    */

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Configure USART1 Rx (PA10) as input floating                         */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Configure USART2 Tx (PA9) as alternate function push-pull            */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* USART1 configured as follow:
          - BaudRate = 115200 baud
          - Word Length = 8 Bits
          - One Stop Bit
          - No parity
          - Hardware flow control disabled (RTS and CTS signals)
          - Receive and transmit enabled
          - USART Clock disabled
          - USART CPOL: Clock is active low
          - USART CPHA: Data is captured on the middle
          - USART LastBit: The clock pulse of the last data bit is not output to
                           the SCLK pin
    */
    USART_InitStructure.USART_BaudRate            = 115200;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);

    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART1, USART_IT_TC, ENABLE);   // разрешаем прерывание по окончанию передачи (нужно для переключения режима работы rs485)

    /*
    DMA_DeInit(DMA1_Channel4);
    DMA_InitStructure.DMA_BufferSize = 1;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_InitStructure.DMA_MemoryBaseAddr = TransmitBuff[0];
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (ulong)&USART1->DR;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_Init(DMA1_Channel4, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel4, DISABLE);
    */
}

/***************************************************************************//**
 * @brief Init USART2 на нем сидит SIM800
 ******************************************************************************/
void SetupUSART2(void)
{

    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    //    memset(rec_buf_usart1, 0, SIZE_BUF_UART1); // обнуляем буфер для принимаемых данных UART2
    //    rec_buf_last_usart1 = -1;                   // индекс последнего необработанного символа принятого от UART1 устанавливаем в -1


    /** Configure pins as GPIO
    PA1    ------> GPIO_Output Сигнал PowerKEY для SIM800
    PA4    ------> GPIO_Input  Сигнал UART1_RI от  SIM800 (звонок или пришла SMS)
    PA5    ------> GPIO_Output Сигнал SEL (Выбор SIM-карты)
    */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //вход с подтяжкой к питанию
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Configure USART2 Rx (PA3) as input floating                         */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Configure USART2 Tx (PA2) as alternate function push-pull            */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* USART2 configured as follow:
          - BaudRate = 115200 baud
          - Word Length = 8 Bits
          - One Stop Bit
          - No parity
          - Hardware flow control disabled (RTS and CTS signals)
          - Receive and transmit enabled
          - USART Clock disabled
          - USART CPOL: Clock is active low
          - USART CPHA: Data is captured on the middle
          - USART LastBit: The clock pulse of the last data bit is not output to
                           the SCLK pin
    */
    USART_InitStructure.USART_BaudRate            = 115200;

    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    //USART2->BRR=0x280; //BaudRate 115200

    USART_Cmd(USART2, ENABLE);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);




    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable the USARTx Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);



}

//Функция отправки байта в UART1
void send_to_uart1(uint8_t data)
{
    while(!(USART1->SR & USART_SR_TC));
    USART1->DR=data;
}

//Функция отправки байта в UART2
void send_to_uart2(uint8_t data)
{
    while(!(USART2->SR & USART_SR_TC));
    USART2->DR=data;
}

//Функция отправки строки в UART1
void send_str_uart1(char * string)
{
    uint8_t i=0;
    while(string[i])
    {
        send_to_uart1(string[i]);
        i++;
    }
}

//Функция отправки строки в UART2
void send_str_uart2(char * string)
{
    uint8_t i=0;
    while(string[i])
    {
        send_to_uart2(string[i]);
        i++;
    }
}

// настройка АЦП
void  InitADC(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /** ADC1 GPIO Configuration
                          PA6	 ------> ADC1_IN6
                          PA7	 ------> ADC1_IN7
                          PB0	 ------> ADC1_IN8
                          PB1	 ------> ADC1_IN9
        */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    ADC1->CR1     =  0;
    ADC1->SQR1    =  0;

    ADC1->SMPR2 |= ADC_SMPR2_SMP6; // MAX cycles (239.5) for 6 channel PA6
    ADC1->SMPR2 |= ADC_SMPR2_SMP7; // MAX cycles (239.5) for 7 channel PA7
    ADC1->SMPR2 |= ADC_SMPR2_SMP8; // MAX cycles (239.5) for 8 channel PB0
    ADC1->SMPR2 |= ADC_SMPR2_SMP9; // MAX cycles (239.5) for 9 channel PB1
    RCC->CFGR   |= RCC_CFGR_ADCPRE;
    RCC->CFGR   |= RCC_CFGR_ADCPRE_DIV8;
    ADC1->CR2   |=  ADC_CR2_CAL;
    while (!(ADC1->CR2 & ADC_CR2_CAL)){};
    ADC1->CR2    =  ADC_CR2_EXTSEL;
    ADC1->CR2   |=  ADC_CR2_EXTTRIG;
    ADC1->CR2   |=  ADC_CR2_ADON;
    ADC_TempSensorVrefintCmd(ENABLE); //TSVREFE_bit (16 - канал встроенный датчик температуры внутри процессора)

    ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE); // разрешаем прерывание по окончаню преобразования

    // калибровка АЦП
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1)) { };
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1)) { };

    // Настройка группы приоритета прерываний
    //NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = ADC1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

// Настройка сиситемного таймера
void Init_SysTick()
{
    SysTick_Config(TIMER_TICK); // Вызываем стандартную функцию настройки системного таймера из core_cm3.h (в ней разрешается прерывание системного таймера)
}

// Прерывания от UART1
void USART1_IRQHandler(void)
{
    uint8_t tmp;

    //if (USART_GetITStatus(USART1, USART_FLAG_RXNE) == SET)
    if((USART1->SR & USART_SR_RXNE)!=0)
    {
        USART_ClearITPendingBit(USART1, USART_FLAG_RXNE);
        tmp = USART_ReceiveData(USART1);

    }

    //Transmission complete interrupt
    if(USART_GetITStatus(USART1, USART_IT_TC) != RESET) // когда все будет передано
    {
        USART_ClearITPendingBit(USART1, USART_IT_TC);
        //tx_end=1;
        GPIO_ResetBits(GPIOB,GPIO_Pin_11); // Переключаем RS485 на прием
    }
}

// Прерывания от UART2
void USART2_IRQHandler(void)
{
    //if (USART_GetITStatus(USART1, USART_FLAG_RXNE) == SET)
    if((USART2->SR & USART_SR_RXNE)!=0)
    {
        USART_ClearITPendingBit(USART2, USART_FLAG_RXNE);

        sim800_response_handler(&state_of_sim800_num1, USART_ReceiveData(USART2)); // вызываем функцию обработки получаемых от SIM800
        //данных, передаем первым параметром ссылку на состояние конкретного модуля SIM800 (их может быть несколько)
    }
}

void ADC1_IRQHandler(void) {
    if (ADC_GetITStatus(ADC1, ADC_IT_EOC))
    {

        ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
        // Вызываем обработку
        ADC_processing(&ADC_current_state_num1, ADC_GetConversionValue(ADC1));
    };
};


uint8_t sys_timer_stop; // флаг остановки вызовов внутри системного таймера
// Прерывание системного таймера
void SysTick_Handler(void)
{
	if (sys_timer_stop)
	{
		//GPIOA->ODR |= GPIO_Pin_0;
		return;
	}
	//GPIOA->ODR &= ~GPIO_Pin_0;
	load_data74HC165(&reg74hc165_current_state_num1); // вызываем функцию чтения входов
    GSM_Communication_routine(); // главная коммуникационная функция GSM
    ADC_conversion_start(&ADC_current_state_num1); // вызываем функцию чтения АЦП
}
