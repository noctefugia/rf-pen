#ifndef __EEPROM_H
#define __EEPROM_H

#include "utils.h" 
#include "i2c_soft.h" 

#define EEPROM_ID ((uint8_t)0b10100000)
#define EEPROM_I2C_WRITE_FLAG ((uint8_t)0x00)
#define EEPROM_I2C_READ_FLAG ((uint8_t)0x01)

#define EEPROM_PAGE_SIZE ((uint8_t)128)
#define EEPROM_MAX_ADDR ((uint16_t)0xFFFF)
#define IS_EEPROM_BUFF_SIZE_OK(buff_sz) \
	((((uint8_t)(buff_sz)) <= (EEPROM_PAGE_SIZE)) \
	&& (((uint8_t)(buff_sz)) > ((uint8_t)(0)))) 
#define IS_EEPROM_ADDR_OK(addr, buff_sz) \
	(((uint16_t)addr + (uint16_t)buff_sz) <= (EEPROM_MAX_ADDR))  //FIX IT!
	
I2CS_STATE_TypeDef EEPROM_Write(uint16_t addr, uint8_t buff_sz, uint8_t *buff);
I2CS_STATE_TypeDef EEPROM_Read(uint16_t addr, uint8_t buff_sz, uint8_t *buff);
bool EEPROM_IsBusy(void);
#endif /* __EEPROM_H */