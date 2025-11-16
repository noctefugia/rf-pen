#include "nrf24.h"

extern struct spiss_struct sys_spis;
struct nrf24_struct sys_nrf24 = {0};


void NRF24_Init(void)
{
	AssignIO(&sys_nrf24.io_ce_rxtx, PORT_NRF24_CE_RXTX, PIN_NRF24_CE_RXTX, TRUE, GPIO_MODE_OUT_PP_LOW_FAST);
	SPIS_HighCS();
}


void NRF24_Config(uint8_t channel, uint8_t uid)
{
	uint8_t i, rf_addr[NRF24_CUR_ADDR_LEN*2];
	
	// Set RF channel
	SPIS_ConfigRegister(NRF24_RF_CH, channel);
	
	// Set length of incoming payload 
	SPIS_ConfigRegister(NRF24_RX_PW_P0, 0x00); // Auto-ACK pipe ...
	SPIS_ConfigRegister(NRF24_RX_PW_P1, NRF24_PAYLOAD_LEN); // Data payload pipe
	SPIS_ConfigRegister(NRF24_RX_PW_P2, 0x00); // Pipe not used 
	SPIS_ConfigRegister(NRF24_RX_PW_P3, 0x00); // Pipe not used 
	SPIS_ConfigRegister(NRF24_RX_PW_P4, 0x00); // Pipe not used 
	SPIS_ConfigRegister(NRF24_RX_PW_P5, 0x00); // Pipe not used 
	
	// 1 Mbps, TX gain: 0dbm
	SPIS_ConfigRegister(NRF24_RF_SETUP, (0<<NRF24_RF_DR)|((0x03)<<NRF24_RF_PWR));

	// CRC enable, 2 byte CRC length
	SPIS_ConfigRegister(NRF24_CONFIG, NRF24_CUR_CONFIG);

	// Auto Acknowledgment
	SPIS_ConfigRegister(NRF24_EN_AA, (1<<NRF24_ENAA_P0)|(1<<NRF24_ENAA_P1)|(0<<NRF24_ENAA_P2)|(0<<NRF24_ENAA_P3)|(0<<NRF24_ENAA_P4)|(0<<NRF24_ENAA_P5));

	// Enable RX addresses
	SPIS_ConfigRegister(NRF24_EN_RXADDR,(1<<NRF24_ERX_P0)|(1<<NRF24_ERX_P1)|(0<<NRF24_ERX_P2)|(0<<NRF24_ERX_P3)|(0<<NRF24_ERX_P4)|(0<<NRF24_ERX_P5));

	// Auto retransmit delay: 1000 us and Up to 15 retransmit trials
	SPIS_ConfigRegister(NRF24_SETUP_RETR, (0x04<<NRF24_ARD)|(0x0F<<NRF24_ARC));

	// Dynamic length configurations: No dynamic length
	SPIS_ConfigRegister(NRF24_DYNPD, (0<<NRF24_DPL_P0)|(0<<NRF24_DPL_P1)|(0<<NRF24_DPL_P2)|(0<<NRF24_DPL_P3)|(0<<NRF24_DPL_P4)|(0<<NRF24_DPL_P5));

	// Start listening
	NRF24_PowerUpRx();

	//set RX & TX addr
	for (i = 0; i < NRF24_CUR_ADDR_LEN*2; ++i)
		rf_addr[i] = ((i >= NRF24_CUR_ADDR_LEN) ? ~uid : uid);
	SPIS_WriteRegister(NRF24_RX_ADDR_P0, &rf_addr[0], NRF24_CUR_ADDR_LEN);
	SPIS_WriteRegister(NRF24_TX_ADDR, &rf_addr[0], NRF24_CUR_ADDR_LEN);
	NRF24_LowCE();
	SPIS_WriteRegister(NRF24_RX_ADDR_P1, &rf_addr[NRF24_CUR_ADDR_LEN], NRF24_CUR_ADDR_LEN);
	NRF24_HighCE();
}


void NRF24_HighCE(void)
{
	WriteIO(&sys_nrf24.io_ce_rxtx, IO_High);
}


void NRF24_LowCE(void)
{
	WriteIO(&sys_nrf24.io_ce_rxtx, IO_Low);
}


void NRF24_PowerUpRx(void)
{     
	SPIS_LowCS();
	SPIS_Transfer(NRF24_FLUSH_RX);
	SPIS_HighCS();
	
	SPIS_ConfigRegister(NRF24_STATUS, (1<<NRF24_RX_DR)|(1<<NRF24_TX_DS)|(1<<NRF24_MAX_RT)); 
	
	NRF24_LowCE();
	SPIS_ConfigRegister(NRF24_CONFIG, NRF24_CUR_CONFIG|((1<<NRF24_PWR_UP)|(1<<NRF24_PRIM_RX)));    
	NRF24_HighCE();
}


void NRF24_PowerUpTx(void)
{
	SPIS_ConfigRegister(NRF24_STATUS, (1<<NRF24_RX_DR)|(1<<NRF24_TX_DS)|(1<<NRF24_MAX_RT)); 

	SPIS_ConfigRegister(NRF24_CONFIG, NRF24_CUR_CONFIG|((1<<NRF24_PWR_UP)|(0<<NRF24_PRIM_RX)));
}


void NRF24_PowerDown(void)
{
	NRF24_LowCE();
	SPIS_ConfigRegister(NRF24_CONFIG, NRF24_CUR_CONFIG);
}


/* Checks if data is available for reading */
/* Returns 1 if data is ready ... */
uint8_t NRF24_DataReady(void) 
{
	// See note in getData() function - just checking RX_DR isn't good enough
	uint8_t status = NRF24_GetStatus();
	
	// We can short circuit on RX_DR, but if it's not set, we still need
	// to check the FIFO for any pending packets
	if ( status & (1 << NRF24_RX_DR) ) 
		return 1;
	
	return ( !NRF24_RxFifoEmpty() );
}


/* Checks if receive FIFO is empty or not */
uint8_t NRF24_RxFifoEmpty(void)
{
	uint8_t fifoStatus;
	
	SPIS_ReadRegister(NRF24_FIFO_STATUS, &fifoStatus, 1);
	
	return (fifoStatus & (1 << NRF24_RX_EMPTY));
}


/* Returns the length of data waiting in the RX fifo */
uint8_t NRF24_PayloadLengthDynamic(void)
{
	uint8_t status;
	
	SPIS_LowCS();
	SPIS_Transfer(NRF24_R_RX_PL_WID);
	status = SPIS_Transfer(0x00);
	SPIS_HighCS();
	
	return status;
}


/* Reads payload bytes into data array */
void NRF24_GetData(uint8_t* data) 
{
	/* Pull down chip select */
	SPIS_LowCS();                              
	
	/* Send cmd to read rx payload */
	SPIS_Transfer(NRF24_R_RX_PAYLOAD);
	
	/* Read payload */
	SPIS_TransferSync(data, data, NRF24_PAYLOAD_LEN);
	
	/* Pull up chip select */
	SPIS_HighCS();
	
	/* Reset status register */
	SPIS_ConfigRegister(NRF24_STATUS, (1<<NRF24_RX_DR));   
}


/* Returns the number of retransmissions occured for the last message */
uint8_t NRF24_RetransmissionCount(void)
{
	uint8_t rv;
	
	SPIS_ReadRegister(NRF24_OBSERVE_TX, &rv, 1);
	rv = rv & 0x0F;
	
	return rv;
}


// Sends a data package to the default address. Be sure to send the correct
// amount of bytes as configured as payload on the receiver.
void NRF24_Send(uint8_t* value) 
{    
	/* Go to Standby-I first */
	NRF24_LowCE();
	 
	/* Set to transmitter mode , Power up if needed */
	NRF24_PowerUpTx();
	
	/* Do we really need to flush TX fifo each time ? */
	#if 1
		/* Pull down chip select */
		SPIS_LowCS();         
		/* Write cmd to flush transmit FIFO */
		SPIS_Transfer(NRF24_FLUSH_TX);     
		/* Pull up chip select */
		SPIS_HighCS();                    
	#endif 
	
	/* Pull down chip select */
	SPIS_LowCS();
	
	/* Write cmd to write payload */
	SPIS_Transfer(NRF24_W_TX_PAYLOAD);
	
	/* Write payload */
	SPIS_TransmitSync(value, NRF24_PAYLOAD_LEN);   
	
	/* Pull up chip select */
	SPIS_HighCS();
	
	/* Start the transmission */
	NRF24_HighCE();  
}


uint8_t NRF24_IsSending(void)
{
	uint8_t status;
	
	/* read the current status */
	status = NRF24_GetStatus();
							
	/* if sending successful (TX_DS) or max retries exceded (MAX_RT). */
	if ( status & ((1 << NRF24_TX_DS) | (1 << NRF24_MAX_RT)) )    
		return 0; /* false */
	
	return 1; /* true */
}

uint8_t NRF24_GetStatus(void)
{
	uint8_t rv;
	
	SPIS_LowCS();
	rv = SPIS_Transfer(NRF24_NOP);
	SPIS_HighCS();
	
	return rv;
}


NRF24_LMS_TypeDef NRF24_LastMessageStatus(void)
{
	uint8_t rv;
	
	rv = NRF24_GetStatus();
	
	/* Transmission went OK */
	if((rv & ((1 << NRF24_TX_DS))))
		return NRF24_LMS_TRANSMISSON_OK;
		
	/* Maximum retransmission count is reached */
	/* Last message probably went missing ... */
	else if((rv & ((1 << NRF24_MAX_RT))))
		return NRF24_LMS_MESSAGE_LOST; 
		
	/* Probably still sending ... */
	else
		return NRF24_LMS_UNDEFINED;
}