#include "eeprom.h"


I2CS_STATE_TypeDef EEPROM_Write(uint16_t addr, uint8_t buff_sz, uint8_t *buff)
{
	uint8_t i;
	I2CS_STATE_TypeDef ack;
	
	assert_param(IS_EEPROM_BUFF_SIZE_OK(buff_sz));
	//assert_param(IS_EEPROM_ADDR_OK(addr, buff_sz));
	
	I2CS_Start();
	I2CS_WriteByte((uint8_t)(EEPROM_ID | EEPROM_I2C_WRITE_FLAG));
	
	I2CS_WriteByte(BYTE_H(addr));
	I2CS_WriteByte(BYTE_L(addr));
	ack = I2CSS_NO_ACK;
	for (i = 0; i < buff_sz; ++i) {
		ack = I2CS_WriteByte(buff[i]);
		I2CS_CHECK_ACK(ack);
	}
	
	I2CS_Stop();
	
	return ack;
}


I2CS_STATE_TypeDef EEPROM_Read(uint16_t addr, uint8_t buff_sz, uint8_t *buff)
{
	uint8_t i;
	I2CS_STATE_TypeDef ack, ack_read;
	
	assert_param(IS_EEPROM_BUFF_SIZE_OK(buff_sz));
	//assert_param(IS_EEPROM_ADDR_OK(addr, buff_sz));
	
	I2CS_Start();
	I2CS_WriteByte((uint8_t)(EEPROM_ID | EEPROM_I2C_WRITE_FLAG));
	
	I2CS_WriteByte(BYTE_H(addr));
	I2CS_WriteByte(BYTE_L(addr));	
	
	I2CS_Start();
	ack = I2CS_WriteByte((uint8_t)(EEPROM_ID | EEPROM_I2C_READ_FLAG));
	
	for (i = 0; i < buff_sz; ++i) {
		ack_read = (I2CS_STATE_TypeDef)((i == ((uint8_t)(buff_sz - 1))) ? I2CSS_NO_ACK : I2CSS_ACK);
		buff[i] = I2CS_ReadByte(ack_read);
	}
	
	I2CS_Stop();

	return ack;
}	


bool EEPROM_IsBusy(void)
{
	uint8_t temp[1];
	I2CS_STATE_TypeDef ack;
	
	ack = EEPROM_Read(0, 1, temp);
	return (bool)((ack != I2CSS_ACK) ? TRUE : FALSE);
}
