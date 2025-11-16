#include "uart.h"

static struct uart_struct sys_uart;
@near static uint8_t uart_rx_buffer[UART_RX_BUFF_SZ];

#define BaudRate_Mantissa    ((uint32_t)CORE_FCPU / ((uint32_t)UART_BAUDRATE << 4))
#define BaudRate_Mantissa100 (((uint32_t)CORE_FCPU * 100) / ((uint32_t)UART_BAUDRATE << 4))
	
void UART_Init(void (*rcv_evt)(uint8_t, uint8_t*))
{
	struct io_struct cur_io;
	AssignIO(&cur_io, PORT_UART_TX, PIN_UART_TX, TRUE, GPIO_MODE_OUT_PP_HIGH_FAST);
	AssignIO(&cur_io, PORT_UART_RX, PIN_UART_RX, TRUE, GPIO_MODE_IN_PU_NO_IT);
	
	//UART1_DeInit();
	/*UART1_Init(UART_BAUDRATE, UART1_WORDLENGTH_8D,
		UART1_STOPBITS_1, UART1_PARITY_NO,
		UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE
	);
	UART1_ITConfig(UART1_IT_RXNE_OR, ENABLE);
	UART1_ITConfig(UART1_IT_TXE, ENABLE);*/
  UART1->BRR2 = ( ((uint8_t)((BaudRate_Mantissa >> 4)) & (uint8_t)0xF0) | ((uint8_t)((uint8_t)(((BaudRate_Mantissa100 - (BaudRate_Mantissa * 100)) << 4) / 100) & (uint8_t)0x0F)) ); 
  UART1->BRR1 = (uint8_t)BaudRate_Mantissa;           
	UART1->CR2 = (uint8_t)(UART1_CR2_TEN | UART1_CR2_REN | 0b10100000);
	
	UART_ResetBufferRX();
	UART_ResetBufferTX();
	sys_uart.rx_event = rcv_evt;
}


void UART_HandlerRX(void)
{
	uint8_t rx_data;
	
	/*rx_data = UART1_ReceiveData8();
	UART1_ClearITPendingBit(UART1_IT_RXNE);*/
	UART1->SR = (uint8_t)~(UART1_SR_RXNE);
	rx_data = (uint8_t)UART1->DR;
	
	//disable RX handler while TX
	if (!UART_TXidle())
		return;
		
	if (sys_uart.rx_msg_size == 0) {
		if (rx_data > UART_RX_BUFF_SZ) {
			UART_ResetBufferRX();
		} else {
			sys_uart.rx_msg_size = (uint8_t)(rx_data + 2); //add checksum bytes
			sys_uart.rx_buffer_pos = 0;
		}
	} 
	
	else {
		uart_rx_buffer[sys_uart.rx_buffer_pos] = rx_data;
		++sys_uart.rx_buffer_pos;
		if (sys_uart.rx_buffer_pos >= UART_RX_BUFF_SZ)
			UART_ResetBufferRX();
		else if (sys_uart.rx_buffer_pos >= sys_uart.rx_msg_size)
			UART_ProcessRX();
	}
}


void UART_HandlerTX(void)
{
	uint8_t tx_data;
	
	if (sys_uart.tx_msg_size == 0) {
		/*UART1_ITConfig(UART1_IT_TXE, DISABLE);*/
		UART1->CR2 &= (uint8_t)(0b01111111);
		return;
	}
		
	if ((sys_uart.tx_buffer_pos >= sys_uart.tx_msg_size) || (sys_uart.tx_buffer_pos >= UART_TX_BUFF_SZ)) {
		UART_ResetBufferTX();
	} else {
		if (!sys_uart.tx_start) {
			tx_data = (uint8_t)(sys_uart.tx_msg_size - 2);
			sys_uart.tx_start = TRUE;
		} else {
			tx_data = sys_uart.tx_buffer[sys_uart.tx_buffer_pos];
			++sys_uart.tx_buffer_pos;
		}
		/*UART1_SendData8(tx_data);*/
		UART1->DR = tx_data;
	}
}


void UART_ResetBufferRX(void)
{
	sys_uart.rx_buffer_pos = 0;
	sys_uart.rx_msg_size = 0;
	sys_uart.rx_time = 0;
}


void UART_ResetBufferTX(void)
{
	sys_uart.tx_buffer_pos = 0;
	sys_uart.tx_msg_size = 0;
	sys_uart.tx_start = FALSE;
	sys_uart.tx_time = 0;
}


void UART_Send(uint8_t msg_sz, uint8_t *msg_buff)
{
	uint8_t i;
	uint16_t crc;
	
	assert_param(IS_UART_TX_BUFF_SZ_OK(msg_sz));

	if (sys_uart.tx_msg_size > 0)
		return;
		
	UART_ResetBufferTX();
	sys_uart.tx_msg_size = msg_sz; //first byte
	
	crc = 0;
	CRC16(&crc, msg_sz); //compute crc from all data
	for (i = 0; i < msg_sz; ++i) { //fill buffer with packet data
		sys_uart.tx_buffer[i] = msg_buff[i];
		CRC16(&crc, msg_buff[i]);
	}
	//add checksum to the end of buffer
	sys_uart.tx_buffer[msg_sz] = BYTE_H(crc);
	sys_uart.tx_buffer[msg_sz+1] = BYTE_L(crc);
	sys_uart.tx_msg_size += 2;
	
	/*UART1_ITConfig(UART1_IT_TXE, ENABLE);*/
	UART1->CR2 |= (uint8_t)(0b10000000);
}


void UART_ProcessRX(void)
{
	uint8_t i;
	uint16_t crc1, crc2;
		
	if (sys_uart.rx_msg_size > 2) {
		sys_uart.rx_msg_size -= 2;
		crc1 = 0;
		CRC16(&crc1, sys_uart.rx_msg_size);
		for (i = 0; i < sys_uart.rx_msg_size; ++i)
			CRC16(&crc1, uart_rx_buffer[i]);

		crc2 = WORD_HL(uart_rx_buffer[sys_uart.rx_msg_size], uart_rx_buffer[sys_uart.rx_msg_size+1]);
		if (crc1 == crc2)
			(*sys_uart.rx_event)(sys_uart.rx_msg_size, uart_rx_buffer);
		else
			nop();
	}
	
	UART_ResetBufferRX();
}


UART_ERROR_TypeDef UART_Update(void)
{
	if ( (sys_uart.rx_msg_size > 0) && ((sys_uart.rx_time++) > UART_MAX_RXTX_TIME) ) {
		UART_ResetBufferRX();
		return UART_ERR_RX_RESET;
	} else if ( (sys_uart.tx_msg_size > 0) && ((sys_uart.tx_time++) > UART_MAX_RXTX_TIME) ) {
		UART_ResetBufferTX();
		return UART_ERR_TX_RESET;
	} else {
		return UART_ERR_NONE;
	}
}


bool UART_RXidle(void)
{	
	return (bool)((sys_uart.rx_msg_size == 0) ? TRUE: FALSE);
}


bool UART_TXidle(void)
{
	return (bool)((sys_uart.tx_msg_size == 0) ? TRUE : FALSE);
}
