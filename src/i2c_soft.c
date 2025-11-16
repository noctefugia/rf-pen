#include "I2C_soft.h"

static struct i2cs_struct i2cs;


void I2CS_Init(void) {
	AssignIO(&i2cs.io_sda, PORT_I2C_SDA, PIN_I2C_SDA, TRUE, GPIO_MODE_OUT_OD_HIZ_FAST);
	AssignIO(&i2cs.io_scl, PORT_I2C_SCL, PIN_I2C_SCL, TRUE, GPIO_MODE_OUT_OD_HIZ_FAST);
	
	//I2CS_Sleep();	
	//I2CS_Reset();
}


void I2CS_Start(void) {	
	I2CS_SetSCL(IO_High);
	I2CS_SetSDA(IO_Low);
}


void I2CS_Stop(void) {
	I2CS_SetSDA(IO_Low);
	I2CS_SetSCL(IO_High);
	I2CS_SetSDA(IO_High);
}


void I2CS_Reset(void)
{  
	uint8_t i;
	
	ModeIO(&i2cs.io_sda, GPIO_MODE_OUT_OD_HIZ_FAST);
	ModeIO(&i2cs.io_scl, GPIO_MODE_OUT_OD_HIZ_FAST);
	I2CS_Stop();
	
	I2CS_SetSCL(IO_Low);
	I2CS_Start();
	I2CS_SetSCL(IO_Low);
	
	for (i = 0; i<9; ++i) {
		I2CS_SetSCL(IO_High);
		I2CS_SetSCL(IO_Low);
	}
	
	I2CS_Start();
	I2CS_SetSCL(IO_Low);
	I2CS_Stop();
} 


I2CS_STATE_TypeDef I2CS_WriteByte(uint8_t data)
{
	uint8_t i;
	I2CS_STATE_TypeDef ack;
		
	for (i = 0; i<8; ++i){
		I2CS_SetSCL(IO_Low);
		if (((data << i) & 0x80) == 0)
			I2CS_SetSDA(IO_Low);
		else
			I2CS_SetSDA(IO_High);
		I2CS_SetSCL(IO_High);
	}
	
	I2CS_SetSCL(IO_Low);
		
	ModeIO(&i2cs.io_sda, GPIO_MODE_IN_FL_NO_IT);
	I2CS_Sleep();
	I2CS_SetSCL(IO_High);
	
	ReadIO(&i2cs.io_sda, &ack);
	ModeIO(&i2cs.io_sda, GPIO_MODE_OUT_OD_HIZ_SLOW);
	
	I2CS_SetSCL(IO_Low);
	
	return ack;
}


uint8_t I2CS_ReadByte(I2CS_STATE_TypeDef ack)
{
	uint8_t i, data;
	bool state;
	GPIO_Mode_TypeDef io_mode;
		
	data = 0;
	ModeIO(&i2cs.io_sda, GPIO_MODE_IN_FL_NO_IT);
	I2CS_Sleep();
	for (i = 0; i<8; ++i){
		I2CS_SetSCL(IO_Low);
		I2CS_SetSCL(IO_High);
		data = (uint8_t)(data << 1);
		
		ReadIO(&i2cs.io_sda, &state);
		if (state)
			data = (uint8_t)(data + 1);
	}
	
	I2CS_SetSCL(IO_Low);
	
	if (ack == I2CSS_ACK)
		io_mode = GPIO_MODE_OUT_OD_LOW_SLOW;
	else
		io_mode = GPIO_MODE_OUT_OD_HIZ_SLOW;
	ModeIO(&i2cs.io_sda, io_mode);
	
	I2CS_Sleep();
	I2CS_SetSCL(IO_High);
	I2CS_SetSCL(IO_Low);
	
	return data;
}


void I2CS_Sleep(void)
{	
	#if (I2CS_DELAY != 0)
	Sleep(I2CS_DELAY);
	#endif
}


void I2CS_SetSCL(IO_MODE_TypeDef io_mode)
{	
	WriteIO(&i2cs.io_scl, io_mode);
	I2CS_Sleep();
}


void I2CS_SetSDA(IO_MODE_TypeDef io_mode)
{	
	WriteIO(&i2cs.io_sda, io_mode);
	I2CS_Sleep();
}
