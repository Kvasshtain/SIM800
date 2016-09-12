#ifndef __SIM800_H_
#define __SIM800_H_

#define busy 1
#define free 0

// Коды ошибок
#define OK   0
#define fail 1 // общая ошибка
#define ATfail 2 // ошибка команды AT
#define PIN_CODE_REQ_EXPECfail 3 // таймаут ожидания приглашения ввести PIN-код
#define ATplusCPINfail 4 // ошибка команды AT+CPIN
#define ATplusCMGFfail 5 // ошибка команды AT+CMGF
#define ATplusCMGDfail 6 // ошибка команды AT+CMGD
#define CALL_SMS_EXPECfail 7 // таймаут ожидания готовности предачи СМС и Звонков

#define yes  1
#define no   0

#define text_mode 1
#define code_mode 0

#define ready 1
#define not_ready 0

#define SIM_SEL_PIN GPIO_Pin_5 // выбор порта и ножки управления мультиплексором SIM карт (если таковой имеется)
#define SIM_SEL_PORT GPIOA
#define select_sim1 GPIOA->ODR |= GPIO_Pin_5 // выбор конкретной подключенной SIM карты (если их две и они подключенны через специальный мультиплексор)
#define select_sim2 GPIOA->ODR &= ~GPIO_Pin_5

#define PWR_KEY_PIN_1 GPIO_Pin_1
#define PWR_KEY_PORT_1 GPIOA

#define PWR_KEY_DOWN GPIOA->ODR |= GPIO_Pin_1 // на линии стоит транзистор как того рекомендует даташит, так что низкий уровень это единичка на пине управления транзистором
#define PWR_KEY_UP GPIOA->ODR &= ~GPIO_Pin_1

#define exit_and_wait 0

#define NORMAL_VOLT 0
#define LOW_VOLTAGE 1

#define CURRENT_CMD_SIZE 256     // размер буфера под передаваемую SIM800 команду
#define REC_BUF_SIZE 256         // размер буфера под принимаемые от SIM800 данные (для случая получения больших объемов данных требуется увеличить это число)
#define DATA_BUF_SIZE 256        // размер буфера под передаваемые в SIM800 данные (для случая отправки больших объемов данных требуется увеличить это число)
#define SEND_SMS_DATA_SIZE 256   // размер буфера под отправляемое СМС сообщение
#define REC_SMS_DATA_SIZE 256    // размер буфера под принимаемое СМС сообщение
#define USSD_DATA_SIZE 256       // размер буфера под данные последнего USSD запроса
#define PHONE_NUM_SIZE 16        // размер буфера под телефонный номер сделал с запасом (достаточно 11 символов, но выравнил на степень двойки)
#define NUM_OF_SUBBUF 2          // число приемных под буферов (для двойной или даже тройной буферизации принимаемых данных)
#define REQ_TIMEOUT      0x100000// таймаут запроса - это когда запрос отправлен но получение ответа слишком затянулось (защита от зависания при работе в блокирующимся режиме)
#define LONG_REQ_TIMEOUT 0x500000// то-же что и предидущее но для случая когда ответ действительно может затянуться (для случая очень долгого ожидания необходимо использовать таймер или вложенные циклы)

// USSD запросы разных операторов
#define Beeline_balance_request   "#102#"
#define MTS_balance_request       "#100#"
#define MegaPhone_balance_request "#100#"
#define Tele2_balance_request     "#105#"
#define Yota_balance_request      "#100#"

// стадии процесса общения с модулем SIM800
enum com_stage {
    req_sent,         //запрос отправлен, но ответ еще не получен
    resp_rec,         //ответ получен, но еще не обработан
    proc_completed    //обработка ответа на запрос завершена
};

//// Структура представляющая из себя определенную AT-команду
//struct request {
//	char current_cmd[CURRENT_CMD_SIZE]; // Сама команда
//	void (*response_handler)(uint8_t * responce, uint8_t * result_of_last_execution, enum com_stage communication_stage, void (*unex_resp_handler)(uint8_t * responce), uint8_t num_of_sms;);
//	// указатель на обработчик ответа для данной команды
//	// должен принимать:1) указатель на буфер где лежит сформированный в прерывании ответ
//	//                  2) указатель на то куда сложить результат обработки ответа
//	//                  3) указатель на ячейку памяти где лежит флаг текущего состояние процесса обработки запроса. Это надо что-бы обработчик положил туда proc_completed - обработка ответа на запрос завершена
//  //                  4) указатель на обработчик неожиданных ответов, если данная функция обнаружила, что ответ адресован не ей
//};

// ГЛОБАЛЬНАЯ СТРУКТУРА СТАТУСА ПРОЦЕССА ОБМЕНА ДАННЫМИ С SIM800
// Структура описывает текущее состояние процесса выдачи запросов и получения ответов от конкретного модуля SIM800 (их может быть несколько)
struct sim800_current_state{
    uint8_t current_SIM_card;                       // номер активной SIM-карты
	uint8_t is_pin_req;                             // PIN-код запрошен?
    uint8_t is_pin_accept;                          // PIN-код принят?
	enum com_stage communication_stage;             // текущее состояние процесса обработки запроса
    uint8_t current_pos;                            // текущая позиция последнего принятого символа в приемном буфере
    // двойная буферизация (возможно сделать даже тройную если нужно будет обрабатывать очень большие ответы)
    uint8_t current_read_buf;                       // текущий подбуфер для чтения принятых данных в обработчиках принятых ответов
    uint8_t current_write_buf;                      // текущий подбуфер для записи принятых данных в прерывании
    uint8_t rec_buf[NUM_OF_SUBBUF][REC_BUF_SIZE];   // приемный буфер (состоит из нескольких подбуферов), куда будет складываться ответ от SIM800 (используетя обработчиком прерывания)
    uint8_t responce[REC_BUF_SIZE];                 // буфер с сформированным ответом от SIM800
    //struct request *current_req;                  // указатель на текущую обрабатываемую команду
    void (*send_uart_function)(char *);             // указатель на функцию отправки данных в конкретный UART на котором сидит конкретный модуль SIM800
    uint8_t result_of_last_execution;               // результат выполнения последней команды 0 - OK, 1 - fail
    uint8_t num_of_sms;                             // число пришедших СМС хранящихся в SIM-карте
    void (*unex_resp_handler)(struct sim800_current_state *current_state); // указатель на обработчик неожиданного ответа
    char current_cmd[CURRENT_CMD_SIZE];             // отправленная команда
    void (*response_handler)(struct sim800_current_state *current_state); // указатель на обработчик ответа для текущей обрабатываемой команды
    uint8_t send_phone_number[PHONE_NUM_SIZE];      // текущий номер телефона для отправляемых СМС сообщений
    uint8_t rec_phone_number[PHONE_NUM_SIZE];       // текущий номер телефона принятого СМС сообщения
    uint8_t send_SMS_data[SEND_SMS_DATA_SIZE];      // буфер для передаваемых СМС сообщений
    uint8_t rec_SMS_data[REC_SMS_DATA_SIZE];        // буфер для принимаемых СМС сообщений
    uint8_t last_USSD_data[USSD_DATA_SIZE];         // буфер данных полученных на последний USSD запрос
    void (*PWR_KEY_handler)(void);                  // указатель на функцию включения конкретного модуля SIM800
    uint8_t is_Call_Ready;                          // Флаг готовности обрабатывать звонки и звонить может быть ready или not_ready
    uint8_t is_SMS_Ready;                           // Флаг готовности обрабатывать звонки и звонить может быть ready или not_ready
    uint8_t power_voltage_status;                   // Статус напряжения питания может быть LOW_VOLTAGE и NORMAL_VOLT
    void (* SIM1_select_handler)(void);             // указатели на функции переключения SIM-карт
    void (* SIM2_select_handler)(void);
};

extern struct sim800_current_state state_of_sim800_num1; // модулей может быть несколько

void sim800_PWRKEY_on(void); // Функция включения модуля SIM800
//!!!!!!inline void Sim800_WriteCmd(const char *cmd); // Функция отправки данных в UART на котором сидит Sim800
//void sim800_AT_request(struct sim800_current_state *current_state); // Функция отправки запроса на автонастройку baudrate модуля SIM800

void select_sim_card1(void); // Функция выбора SIM-карты 1 (вынес в функции, т.к. модулей SIM800 может быть несколько и у каждого могут быть свои мультиплесоры выбора SIM-карт)
void select_sim_card2(void); // Функция выбора SIM-карты 2
uint8_t sim800_init(struct sim800_current_state * current_state, void (*send_uart_function)(char *), uint8_t cur_SIM_card, uint16_t pin_code); // Функция инициализации одного из модулей SIM800
int8_t sim800_request(struct sim800_current_state *current_state); // функция отправки запросов в SIM800
void process_echo(uint8_t is_responce, uint8_t current_pos, struct sim800_current_state *current_state); // Обработка ЭХО
void process_cmd(uint8_t is_responce, uint8_t current_pos, struct sim800_current_state *current_state);  // Обработка ответа на команду
void call_handler(struct sim800_current_state * current_state); // Вспомогательная функция копирования содержимого приемного буфера в буфер принятого ответа и вызова соответствующего обработчика принятого ответа
void sim800_response_handler(struct sim800_current_state *current_state, uint8_t data); // функция вызываемая из обработчика прерывания по приему символов от SIM800


uint8_t sim800_AT_request(struct sim800_current_state * current_state); // Функция отправки запроса на автонастройку baudrate модуля SIM800 (команда "AT")
void sim800_AT_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT"

uint8_t sim800_ATplusCPIN_request(struct sim800_current_state * current_state, uint16_t PIN_code); //Функция разблокировки SIM-карты "AT+CPIN=pin-code"
void sim800_ATplusCPIN_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CPIN=pin-code"

uint8_t sim800_ATplusCMGF_request(struct sim800_current_state * current_state, uint8_t mode); // Функция переключения SIM800 в текстовый режим (команда "AT+CMGF=1 или 0 1-включить, 0-выключить")
void sim800_ATplusCMGF_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGF=0/1"

uint8_t sim800_ATplusCMGS_request(struct sim800_current_state * current_state, uint8_t * phone_number, uint8_t * SMS_data); // Функция отправки СМС SIM800 (команда "AT+CMGS=«ХХХХХХХХХХХ»")
void sim800_ATplusCMGS_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGS=«ХХХХХХХХХХХ»"

uint8_t sim800_ATplusCMGD_request(struct sim800_current_state * current_state, uint8_t num_of_message, uint8_t mode_of_delete); // // Функция удаления СМС SIM800 (команда "AT+CMGD=1,0" 1, — номер сообщения 0, — режим удаления)
void sim800_ATplusCMGD_responce_handler(struct sim800_current_state * current_state); // // Обработчик ответа команды "AT+CMGD=

uint8_t sim800_ATplusCMGR_request(struct sim800_current_state * current_state, uint8_t num_of_message, uint8_t mode_of_read); // Функция отправки запроса на чтения СМС SIM800 (команда "AT+CMGR=1,0"
// Пришлось обработку ответа разбить на стадии, т.к. ответ большой и его парсинг не успевал до прихода финального OK
void sim800_ATplusCMGR_responce_handler_st1(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGR= - чтение SMS стадия 1 - прием ЭХО
void sim800_ATplusCMGR_responce_handler_st2(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGR= - чтение SMS стадия 2 - прием служебных данных о SMS
void sim800_ATplusCMGR_responce_handler_st3(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGR= - чтение SMS стадия 3 - прием самого текста SMS
void sim800_ATplusCMGR_responce_handler_st4(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGR= - чтение SMS стадия 4 - обработка сообщения OK
//void sim800_ATplusCMGR_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGR= - чтение SMS

uint8_t sim800_ATplusCUSD_request(struct sim800_current_state * current_state, uint8_t * USSD_req); // Функция отправки USSD запроса SIM800 (команда "AT+CUSD=1,"XXXXX", где XXXXX например #102#)
void sim800_ATplusCUSD_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CUSD=1,"XXXXX"

void unexpec_message_parse(struct sim800_current_state *current_state); //функция парсинга внезапных сообщений от SIM800 (например пришла SMS)
//uint8_t sim800_sendSMS(uint8_t* text_buf, uint8_t length);   // Функция отправки SMS с модуля SIM800

#endif
