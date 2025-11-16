#ifndef __AHT20_H
#define __AHT20_H

#include "utils.h" 
#include "i2c_soft.h" 

#define AHT20_I2C_PAIR I2CSP_MAIN
#define AHT20_I2C_ADDRESS ((uint8_t)0b01110000)
#define AHT20_I2C_WRITE_FLAG ((uint8_t)0x00)
#define AHT20_I2C_READ_FLAG ((uint8_t)0x01)

typedef enum {
	AHT20C_INIT = 0xBE,
	AHT20C_TRIG_MEASUREMENT = 0xAC,
	AHT20C_SOFT_RESET = 0xBA,
	AHT20C_STATUS_WORD = 0x71
} AHT20_CMD_TypeDef;

#define AHT20_MEASUREMENT_BYTE_COUNT ((uint8_t)6) //without crc

//command parameters
#define AHT20CP_INIT1 ((uint8_t)0x08)
#define AHT20CP_INIT2 ((uint8_t)0x00)
#define AHT20CP_TRIG_MEASUREMENT1 ((uint8_t)0x33)
#define AHT20CP_TRIG_MEASUREMENT2 ((uint8_t)0x00)

#define AHT20_BIT_BUSY ((uint8_t)0b10000000)
#define AHT20_BIT_CAL_EN ((uint8_t)0b00001000)


I2CS_STATE_TypeDef AHT20_GetStatus(uint8_t *status);
I2CS_STATE_TypeDef AHT20_Calibrate(void);
I2CS_STATE_TypeDef AHT20_TriggerMeasurement(void);
I2CS_STATE_TypeDef AHT20_SoftReset(void);
I2CS_STATE_TypeDef AHT20_ReadData(uint8_t *data_buff);

#endif /* __AHT20_H */


