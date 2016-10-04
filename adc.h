#ifndef __ADC_h_
#define __ADC_H_

#define NUM_OF_ADC_CHANNEL 4

#define FIRST_CHANNEL      6 // первый канал подлежащий оцефровке
#define LAST_CHANNEL       9 // последний канал подлежащий оцефровке

#define ADC_NOT_READY      0 // не готово, когда результаты всех каналов АЦП в нулях (состояние при первом пуске системы или неисправности)
#define ADC_READY          1 // готов, когда АЦП начало преобразования и в массиве result лежат валидные данные оцифровки
#define ADC_BUSY           2 // занят, резервное состояние например для процесса перекалибровки

// пороги при снижении и увеличении проверямых напряжений при которых будут рассылаться соответствующие сообщения
// Для внешней батареи
#define DECREASE_THRESHOLD_EXT_BAT         2600 // порог уменьшения (меньший)
#define INCREASE_THRESHOLD_EXT_BAT         3072 // порог увеличения (больший)
// Для сетевого напряжения
#define DECREASE_THRESHOLD_MAIN_VOLT       2600
#define INCREASE_THRESHOLD_MAIN_VOLT       3072
// Для внешнего резерва
#define DECREASE_THRESHOLD_BACKUP          2600
#define INCREASE_THRESHOLD_BACKUP          3072
// Для внутренней батареи
#define DECREASE_THRESHOLD_BAT             2600
#define INCREASE_THRESHOLD_BAT             3072

// БИТОВОЕ ПОЛЕ СТАТУСА ОДНОГО АНАЛОГОВОГО КАНАЛА
struct analog_ch_status_bf
{
    unsigned decr_th_already_sent : 1; // статус отправки сообщения (напрмер SMS) о уменьшении напряжения на аналоговом канале ниже порога снижения
    //                                    если напряжение на канале выше порога, то тут 0,
    //                                    если напряжение упало ниже порога то до отправки SMS здесь 0, после отправки SMS здесь 1 (что бы SMS не отправлялись без остановки)
    unsigned incr_th_already_sent : 1; // статус отправки сообщения (напрмер SMS) о увеличении напряжения на аналоговом канале выше порога увеличения
    //                                    если напряжение на канале ниже порога, то тут 0,
    //                                    если напряжение выросло выше порога то до отправки SMS здесь 0, после отправки SMS здесь 1 (что бы SMS не отправлялись без остановки)
    unsigned is_normal            : 1; // признак что напряжение нормально, устанавливается в 1 при превышении выше порогоа увеличения (большего) и в 0 при принижении порога на уменьшение (меньшего)
    unsigned reserved             : 5; // зарезервированы
};

// структура описывающая текущее состояние одного аналогового канала
struct channel_state
{
    struct analog_ch_status_bf analog_ch_status;
    uint16_t                   value;                        // оцифрованное значение входного напряжение
    uint16_t                   decrease_thteshold;           // порог уменьшения (меньший)
    uint16_t                   increase_thteshold;           // порог увеличения (больший)
};

// Структура описывает текущее состояние подсистемы АЦП
struct ADC_current_state
{
    uint8_t              current_channel;            // текущий считываемый канал АЦП
    struct channel_state result[NUM_OF_ADC_CHANNEL]; // массив результатов оцифровки входных сигналов каналов АЦП (содержит так-же пороги, статус канала, и строки сообщений)
    uint16_t             temperature;                // результат оцифровки канала температуры внутри процессора
    uint8_t              state;                      // состояние системы может принимать три значения ADC_NOT_READY/ADC_READY/ADC_BUSY
};

extern struct ADC_current_state ADC_current_state_num1; // подсистем для работы с АЦП может быть несколько

void ADC_init_routine(struct ADC_current_state *current_state); // инициализация процесса оцифровки сигналов на входах АЦП

void ADC_conversion_start(struct ADC_current_state *current_state); // функция запуска преобразования на одном из входов АЦП

void PWR_check(struct ADC_current_state *current_state); // функция проверки напряжения на всех внешних и внутренних источниках питания

void ADC_processing(struct ADC_current_state *current_state, uint16_t value); // Функция вызывается из обработчика прерывания АЦП принимает значение полученное в ходе преобразования

#endif
