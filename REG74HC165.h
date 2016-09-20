// Драйвер регистров 74HC165 с параллельным входом и последовательным выходом включенных каскадно
#ifndef __REG74HC165_H_
#define __REG74HC165_H_

#define   NUM_OF_74HC165 3 // число каскадированных регистров

#define   REG74HC165_PORT GPIOC        // порт на котором сидят регистры 74hc165

#define   CP_PIN          GPIO_Pin_14  // строб - сдвиг данных
#define   CP_PIN_DOWN GPIOC->ODR |=  GPIO_Pin_14
#define   CP_PIN_UP GPIOC->ODR   &= ~GPIO_Pin_14
#define   CP_PIN_STATE  GPIOC->ODR & GPIO_Pin_14

#define   PL_PIN          GPIO_Pin_13  //защёлкивание входов
#define   PL_PIN_DOWN GPIOC->ODR |=  GPIO_Pin_13
#define   PL_PIN_UP GPIOC->ODR   &= ~GPIO_Pin_13
#define   PL_PIN_STATE  GPIOC->ODR & GPIO_Pin_13

#define   QH_PIN          GPIO_Pin_15  //вход данных
#define   QH_PIN_DOWN GPIOC->ODR |=  GPIO_Pin_15
#define   QH_PIN_UP GPIOC->ODR   &= ~GPIO_Pin_15
#define   QH_PIN_STATE  GPIOC->ODR & GPIO_Pin_15

#define   NUM_BIT         (8*NUM_OF_74HC165) //количество входов-бит ...1 регистр 8 входов

// ГЛОБАЛЬНАЯ СТРУКТУРА СТАТУСА ПРОЦЕССА ЧТЕНИЯ ДАННЫХ ИЗ РЕГИСТРОВ 74HC165
// Структура описывает текущее состояние процесса побитного чтения данных из нескольких каскадированных параллельно последовательных регистров 74hc165
struct reg74hc165_current_state{
	uint8_t current_bit;                       // текущий считываемый бит
	uint32_t input_data;                       // считываемое значение
	uint32_t result;                           // результат
};

extern struct reg74hc165_current_state reg74hc165_current_state_num1; // регистровых каскадов может быть несколько

void load_data74HC165(struct reg74hc165_current_state * current_state); // Функция вызываемая из обработчика прерывания таймера производящая побитное чтение данных с выхода 74hc165

#endif
