#include "spi_soft.h"
struct spiss_struct sys_spis = {0};

void SPIS_Init(void)
{
	AssignIO(&sys_spis.io_miso, PORT_SPI_MISO, PIN_SPI_MISO, TRUE, GPIO_MODE_IN_FL_NO_IT);
	AssignIO(&sys_spis.io_mosi, PORT_SPI_MOSI, PIN_SPI_MOSI, TRUE, GPIO_MODE_OUT_PP_LOW_FAST);
	AssignIO(&sys_spis.io_sck, PORT_SPI_SCK, PIN_SPI_SCK, TRUE, GPIO_MODE_OUT_PP_LOW_FAST);
	AssignIO(&sys_spis.io_cs, PORT_SPI_CS, PIN_SPI_CS, TRUE, GPIO_MODE_OUT_PP_LOW_FAST);
}


/* software spi routine */
uint8_t SPIS_Transfer(uint8_t tx)
{
	uint8_t i, rx;
	bool pin_state;
		
	rx = 0;
	WriteIO(&sys_spis.io_sck, IO_Low);

	for(i=0; i<8; i++)
	{
		WriteIO(&sys_spis.io_mosi, (IO_MODE_TypeDef)((tx & (1<<(7-i))) ? IO_High : IO_Low));
      
		WriteIO(&sys_spis.io_sck, IO_High);
		
		rx = rx << 1;
		ReadIO(&sys_spis.io_miso, &pin_state);
		if (pin_state)
			rx |= 0x01;
              
		WriteIO(&sys_spis.io_sck, IO_Low);
	}

	return rx;
}


/* send and receive multiple bytes over SPI */
void SPIS_TransferSync(uint8_t* dataout, uint8_t* datain, uint8_t len)
{
	uint8_t i;
		
	for(i=0; i<len; i++)
		datain[i] = SPIS_Transfer(dataout[i]);
}


/* send multiple bytes over SPI */
void SPIS_TransmitSync(uint8_t* dataout, uint8_t len)
{
	uint8_t i;
		
	for(i=0; i<len; i++)
		SPIS_Transfer(dataout[i]);
}


/* Clocks only one byte into the given nrf24 register */
void SPIS_ConfigRegister(uint8_t reg, uint8_t value)
{
	//assert_param(IS_SPIS_DEVICE_ID_OK(id));
		
	SPIS_LowCS();
	SPIS_Transfer(SPIS_W_REGISTER | (SPIS_REGISTER_MASK & reg));
	SPIS_Transfer(value);
	SPIS_HighCS();
}


/* Read single register from nrf24 */
void SPIS_ReadRegister(uint8_t reg, uint8_t* value, uint8_t len)
{
	//assert_param(IS_SPIS_DEVICE_ID_OK(id));
		
	SPIS_LowCS();
	SPIS_Transfer(SPIS_R_REGISTER | (SPIS_REGISTER_MASK & reg));
	SPIS_TransferSync(value, value, len);
	SPIS_HighCS();
}


/* Write to a single register of nrf24 */
void SPIS_WriteRegister(uint8_t reg, uint8_t* value, uint8_t len) 
{
	//assert_param(IS_SPIS_DEVICE_ID_OK(id));
		
	SPIS_LowCS();
	SPIS_Transfer(SPIS_W_REGISTER | (SPIS_REGISTER_MASK & reg));
	SPIS_TransmitSync(value, len);
	SPIS_HighCS();
}


void SPIS_HighCS(void)
{
	WriteIO(&sys_spis.io_cs, IO_High);
}


void SPIS_LowCS(void)
{
	WriteIO(&sys_spis.io_cs, IO_Low);
}