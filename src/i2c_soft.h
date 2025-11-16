#ifndef __I2C_SOFT_H
#define __I2C_SOFT_H

#include "utils.h" 

typedef enum {
	I2CSS_NO_ACK = HIGH,
	I2CSS_ACK = LOW,
	I2CSS_ERROR,
	I2CSS_DISABLED
} I2CS_STATE_TypeDef;


#define I2CS_DELAY 0

#define I2CS_CHECK_ACK(ack) \
	if ((ack) != (I2CSS_ACK)) {/*I2CS_Reset();*/ return ack;}
	
struct i2cs_struct {
	struct io_struct io_sda, io_scl;
};

void I2CS_Init(void);
void I2CS_Start(void);
void I2CS_Stop(void);
void I2CS_Reset(void);
void I2CS_Sleep(void);
void I2CS_SetSCL(IO_MODE_TypeDef io_mode);
void I2CS_SetSDA(IO_MODE_TypeDef io_mode);
I2CS_STATE_TypeDef I2CS_WriteByte(uint8_t data);
uint8_t I2CS_ReadByte(I2CS_STATE_TypeDef ack);


#endif /* __I2C_SOFT_H */