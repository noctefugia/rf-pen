#ifndef __UART_H
#define __UART_H

#include "utils.h" 

#define UART_BAUDRATE 115200
#define UART_MAX_RXTX_TIME TICKS8(1000)
#define UART_RX_PACKET_MAX_SIZE ((uint8_t)140)
#define UART_TX_PACKET_MAX_SIZE ((uint8_t)10)

//+2 checksum bytes
#define UART_RX_BUFF_SZ ((uint8_t)(UART_RX_PACKET_MAX_SIZE+2))
#define UART_TX_BUFF_SZ ((uint8_t)(UART_TX_PACKET_MAX_SIZE+2))

#define IS_UART_TX_BUFF_SZ_OK(msg_sz) \
	((msg_sz) <= ((uint8_t)(UART_TX_PACKET_MAX_SIZE))) 

typedef enum {
	UART_ERR_NONE,
	UART_ERR_RX_RESET,
	UART_ERR_TX_RESET,
	UART_ERR_INVALID //add new states above this line
} UART_ERROR_TypeDef;

struct uart_struct {
	bool tx_start : 1;
	uint8_t tx_buffer[UART_TX_BUFF_SZ];
	uint8_t rx_buffer_pos, tx_buffer_pos, rx_msg_size, tx_msg_size, rx_time, tx_time;
	void (*rx_event)(uint8_t, uint8_t*);
};

void UART_Init(void (*rcv_evt)(uint8_t, uint8_t*));
void UART_HandlerRX(void);
void UART_HandlerTX(void);
static void UART_ResetBufferRX(void);
static void UART_ResetBufferTX(void);
void UART_Send(uint8_t msg_sz, uint8_t *msg_buff);
static void UART_ProcessRX(void);
bool UART_RXidle(void);
bool UART_TXidle(void);
UART_ERROR_TypeDef UART_Update(void);

#endif /* __UART_H */