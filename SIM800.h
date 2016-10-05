// драйвер SIM800
// частично совместим со тарыми семействами SIM900 и SIM300
#ifndef __SIM800_H_
#define __SIM800_H_

#define busy 1
#define free 0

// Коды ошибок
#define OK   0
#define fail 2                   // общая ошибка
#define ATfail 3                 // ошибка команды AT
#define PIN_CODE_REQ_EXPECfail 4 // таймаут ожидания приглашения ввести PIN-код
#define ATplusCPINfail 5         // ошибка команды AT+CPIN
#define ATplusCMGFfail 6         // ошибка команды AT+CMGF
#define ATplusCMGDfail 7         // ошибка команды AT+CMGD
#define CALL_SMS_EXPECfail 8     // таймаут ожидания готовности предачи СМС и Звонков
#define ATplusCREGfail 9         // ошибка команды AT+CREG
#define ATplusCSPNfail 10        // ошибка команды AT+CSPN
#define ATplusCSTTfail 11        // ошибка отправки запроса APN (например в списке доступных мобильных операторов не нашлось заданного)

#define yes  1
#define no   0

#define text_mode 1
#define code_mode 0

#define ready 1
#define not_ready 0

#define single_connection 0
#define multiple_connection 1

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

#define CURRENT_CMD_SIZE 128     // размер буфера под передаваемую SIM800 команду
#define REC_BUF_SIZE 128         // размер буфера под принимаемые от SIM800 данные (для случая получения больших объемов данных требуется увеличить это число)
#define DATA_BUF_SIZE 128        // размер буфера под передаваемые в SIM800 данные (для случая отправки больших объемов данных требуется увеличить это число)
#define SEND_SMS_DATA_SIZE 128   // размер буфера под отправляемое СМС сообщение
#define REC_SMS_DATA_SIZE 128    // размер буфера под принимаемое СМС сообщение
#define USSD_DATA_SIZE 128       // размер буфера под данные последнего USSD запроса
#define PHONE_NUM_SIZE 16        // размер буфера под телефонный номер сделал с запасом (достаточно 11 символов, но выравнил на степень двойки)
#define NUM_OF_SUBBUF 3          // число приемных под буферов (для двойной или даже тройной буферизации принимаемых данных)
#define REQ_TIMEOUT      0x100000// таймаут запроса - это когда запрос отправлен но получение ответа слишком затянулось (защита от зависания при работе в блокирующимся режиме)
#define LONG_REQ_TIMEOUT 0x500000// то-же что и предидущее но для случая когда ответ действительно может затянуться (для случая очень долгого ожидания необходимо использовать таймер или вложенные циклы)
#define LL_REQ_TIMEOUT  0x1500000// то-же что и предидущее но для случая когда ответ действительно может затянуться (для случая очень долгого ожидания необходимо использовать таймер или вложенные циклы)

// USSD запросы разных операторов
#define Beeline_balance_request   "#102#"
#define MTS_balance_request       "#100#"
#define MegaPhone_balance_request "#100#"
#define Tele2_balance_request     "#105#"
#define Yota_balance_request      "#100#"

// Идентификаторы мобилных операторов
enum operators {
    Beeline = 1,     //1
    MTS,             //2
    MegaPhone,       //3
    Tele2,           //4
    Yota             //5
} mobile_operators;

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
    uint8_t current_read_buf;                       // номер текущего подбуфера для чтения принятых данных в обработчиках принятых ответов
    uint8_t current_write_buf;                      // номер текущего подбуфер для записи принятых данных в прерывании
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
    uint8_t current_registration_state;             // Текущее состояние регистрации в сети: 0 — не зарегистрирован, поиск сети не ведется,
    //                                                                                       1 — зарегистрирован в своей домашней сети,
    //                                                                                       2 — не зарегистрирован, идет поиск сети,
    //                                                                                       3 — регистрация отклонена,
    //                                                                                       4 — модуль сам не знает что происходит
    //                                                                                       5 — зарегистрирован в роуминге.
    void (* SIM1_select_handler)(void);             // указатели на функции переключения SIM-карт
    void (* SIM2_select_handler)(void);
    enum operators mobile_operator_SIM1;            // идентификатор мобильного оператора SIM-карты 1
    enum operators mobile_operator_SIM2;            // идентификатор мобильного оператора SIM-карты 2
    enum operators current_mobile_operator;         // в зависимости от того, какая SIM-карта активна здесь будет лежать либо mobile_operator_SIM1, либо mobile_operator_SIM2
    uint8_t IP_address_string[64];                  // ip-адрес IPv4 например 123.675.874.234 - максимум 16 символов, в IPv6 (на всякий случай) - максимум 40 символов, округлил до 64
    uint8_t Status;                                 // Статус: модуль готов к работе (ready), или не готов к работе (not_ready)
    uint8_t num_of_fail;                            // Число неудачно выполненых запросов (закончевшихся ERROR)
    uint8_t mode_of_delete;                         // режим удаления SMS - сообщений в команде AT+CMGD
};

extern struct sim800_current_state state_of_sim800_num1; // модулей может быть несколько

uint8_t * stristr (const uint8_t * str1, const uint8_t * str2);

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

uint8_t sim800_ATplusCSPNquestion_request(struct sim800_current_state * current_state); // Функция отправки запроса оператора SIM-карты SIM800 (команда "AT+CSPN?")
void sim800_ATplusCSPNquestion_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CSPN?"

uint8_t sim800_ATplusCMGS_request(struct sim800_current_state * current_state, uint8_t * phone_number, uint8_t * SMS_data); // Функция отправки СМС SIM800 (команда "AT+CMGS=«ХХХХХХХХХХХ»")
// Пришлось обработку ответа разбить на стадии, т.к. ответ большой и его парсинг может не успеть до прихода финального OK
void sim800_ATplusCMGS_responce_handler_st1(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGS= - передачи SMS стадия 1 - прием ЭХО
void sim800_ATplusCMGS_responce_handler_st2(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGS= - передачи SMS стадия 2 - прием приглашения ввести текст SMS и ввод текста
void sim800_ATplusCMGS_responce_handler_st3(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGS= - передачи SMS стадия 3 - передачи SMS стадия 3 - прием эхо текста SMS или служебной информации
void sim800_ATplusCMGS_responce_handler_st4(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGS= - передачи SMS стадия 4 - прием служебной информации
void sim800_ATplusCMGS_responce_handler_st5(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGS= - передачи SMS стадия 4 - обработка сообщения OK

uint8_t sim800_ATplusCMGD_request(struct sim800_current_state * current_state, uint8_t num_of_message, uint8_t mode_of_delete); // // Функция удаления СМС SIM800 (команда "AT+CMGD=1,0" 1, — номер сообщения 0, — режим удаления)
void sim800_ATplusCMGD_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGD="

uint8_t sim800_ATplusCREGquestion_request(struct sim800_current_state * current_state); // Функция отправки запроса регистрации в сети SIM800 (команда "AT+CREG?")
void sim800_ATplusCREGquestion_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CREG?"

uint8_t sim800_ATplusCMGR_request(struct sim800_current_state * current_state, uint8_t num_of_message, uint8_t mode_of_read); // Функция отправки запроса на чтения СМС SIM800 (команда "AT+CMGR=1,0"
// Пришлось обработку ответа разбить на стадии, т.к. ответ большой и его парсинг может не успеть до прихода финального OK
void sim800_ATplusCMGR_responce_handler_st1(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGR= - чтение SMS стадия 1 - прием ЭХО
void sim800_ATplusCMGR_responce_handler_st2(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGR= - чтение SMS стадия 2 - прием служебных данных о SMS
void sim800_ATplusCMGR_responce_handler_st3(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGR= - чтение SMS стадия 3 - прием самого текста SMS
void sim800_ATplusCMGR_responce_handler_st4(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CMGR= - чтение SMS стадия 4 - обработка сообщения OK

uint8_t sim800_ATplusCUSD_request(struct sim800_current_state * current_state, uint8_t * USSD_req); // Функция отправки USSD запроса SIM800 (команда "AT+CUSD=1,"XXXXX", где XXXXX например #102#)
void sim800_ATplusCUSD_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CUSD=1,"XXXXX"

uint8_t sim800_ATplusCGATTequal1_request(struct sim800_current_state * current_state); // // Функция регистрации модуля SIM800 в GPRS сети (команда "AT+CGATT=1")
void sim800_ATplusCGATTequal1_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CGATT=1"

uint8_t sim800_ATplusCIPRXGETequal1_request(struct sim800_current_state * current_state); // Функция переключения модуля SIM800 в режим приема GPRS даненых вручную (команда "AT+CIPRXGET=1")
void sim800_ATplusCIPRXGETequal1_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CIPRXGET=1"

uint8_t sim800_ATplusCIPMUX_request(struct sim800_current_state * current_state, uint8_t mode); // Функция настройки колличества подключений модуля SIM800 в режим приема GPRS (команда "AT+CIPMUX=x", где x = 0 - одно соединение или 1 - много соединений)
void sim800_ATplusCIPMUX_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CIPMUX=x"

uint8_t sim800_ATplusCSTT_request(struct sim800_current_state * current_state); // Функция настройки точки доступа SIM800 в режим приема GPRS (команда "AT+CSTT="xxxxxxxxx"", где xxxxxxxxx - это APN — Access Point Name
void sim800_ATplusCSTT_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CSTT="xxxxxxxxx""

uint8_t sim800_ATplusCIICR_request(struct sim800_current_state * current_state); // Функция установки беспроводного GPRS соединения модуля SIM800(команда "AT+CIICR")
void sim800_ATplusCIICR_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CIICR"

uint8_t sim800_ATplusCIFSR_request(struct sim800_current_state * current_state); // Функция запроса IP адреса беспроводного GPRS соединения модуля SIM800(команда "AT+CIFSR")
void sim800_ATplusCIFSR_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CIFSR"

uint8_t sim800_ATplusCDNSCFG_request(struct sim800_current_state * current_state, uint8_t * primary_DNS_server, uint8_t * secondary_DNS_server); // Функция устанавливки сервера DNS беспроводного GPRS соединения модуля SIM800(команда "AT+CDNSCFG="X.X.X.X","X.X.X.X"")
void sim800_ATplusCDNSCFG_responce_handler(struct sim800_current_state * current_state); // Обработчик ответа команды "AT+CDNSCFG"

void unexpec_message_parse(struct sim800_current_state *current_state); //функция парсинга внезапных сообщений от SIM800 (например пришла SMS)
//uint8_t sim800_sendSMS(uint8_t* text_buf, uint8_t length);   // Функция отправки SMS с модуля SIM800

#endif
