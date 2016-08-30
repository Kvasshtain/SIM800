#ifndef __SIM800_H_
#define __SIM800_H_

#define busy 1
#define free 0

#define OK   0
#define fail 1

#define yes  1
#define no   0

#define exit_and_wait 0

#define CURRENT_CMD_SIZE 16 // размер буфера под передаваемую SIM800 команду
#define REC_BUF_SIZE 256    // размер буфера под принимаемые от SIM800 данные (для случая получения больших объемов данных требуется увеличить это число)

// стадии процесса общения с модулем SIM800
enum com_stage {
	req_sent,         //запрос отправлен, но ответ еще не получен
	resp_rec,         //ответ получен, но еще не обработан
	proc_completed    //обработка ответа на запрос завершена
};

// ГЛОБАЛЬНАЯ СТРУКТУРА СТАТУСА ПРОЦЕССА ОБМЕНА ДАННЫМИ С SIM800
// Структура описывает текущее состояние процесса выдачи запросов и получения ответов от конкретного модуля SIM800 (их может быть несколько)
typedef struct {
    char current_cmd[CURRENT_CMD_SIZE];       // буффер для текущей обрабатываемой SIM800 команды (необходим для проверки ЭХО)
    com_stage communication_stage;            // текущее состояние процесса обработки запроса
    uint8_t current_pos;                      // текущая позиция последнего принятого символа в приемном буфере
    //uint8_t is_responce;                      // флаг для того, что бы (если включен режим ЭХО у SIM800) отличать внезапные сообщения от ответов на заданные запросы
    uint8_t rec_buf[REC_BUF_SIZE];            // приемный буфер, куда будет складываться ответ от SIM800 (используетя обработчиком прерывания)
    uint8_t responce[REC_BUF_SIZE];           // буфер с сформированным ответом от SIM800 для обработки ответа на посланный запрос или внезапного сообщения
    //uint8_t unexp_responce[REC_BUF_SIZE];     // буфер с сформированным ответом от SIM800 для обработки внезапного сообщения
    void (*response_handler)(void);           // указатель на обработчик ответа
    void (*send_uart_function)(char *);       // указатель на функцию отправки данных в конкретный UART на котором сидит конкретный модуль SIM800
    uint8_t result_of_last_execution;         // результат выполнения последней команды 0 - OK, 1 - fail
    uint8_t num_of_sms;                       // число пришедших СМС хранящихся в SIM-карте
} sim800_current_state;

extern sim800_current_state sim800_1_current_state; // модулей может быть несколько

void sim800_PWRKEY_on(void); // Функция включения модуля SIM800
//!!!!!!inline void Sim800_WriteCmd(const char *cmd); // Функция отправки данных в UART на котором сидит Sim800
void sim800_AT_request(sim800_current_state * current_module); // Функция отправки запроса на автонастройку baudrate модуля SIM800
void sim800_AT_responce_handler(uint8_t * responce, uint8_t result, uint8_t is_busy); // Обработчик ответа команды "AT"
uint8_t sim800_init(sim800_current_state * current_module, int32_t * init_data); // Функция инициализации одного из модулей SIM800
int8_t sim800_request(sim800_current_state * current_state); // функция отправки запросов в SIM800
void process_echo(uint8_t is_responce, uint8_t current_pos, sim800_current_state *current_state); // Обработка ЭХО
void process_cmd(uint8_t is_responce, uint8_t current_pos, sim800_current_state *current_state);  // Обработка ответа на команду
void sim800_response_handler(sim800_current_state * current_state, uint8_t data); // функция вызываемая из обработчика прерывания по приему символов от SIM800
void unexpec_message_parse(uint8_t * unexp_responce); //функция парсинга внезапных сообщений от SIM800 (например пришла SMS)
//uint8_t sim800_sendSMS(uint8_t* text_buf, uint8_t length);   // Функция отправки SMS с модуля SIM800

#endif
