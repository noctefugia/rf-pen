#include "aht20.h"


I2CS_STATE_TypeDef AHT20_GetStatus(uint8_t *status)
{         
	I2CS_STATE_TypeDef ack;
	
	*status = 0;
		
	I2CS_Start();
	I2CS_WriteByte((uint8_t)(AHT20_I2C_ADDRESS | AHT20_I2C_WRITE_FLAG));
	
	I2CS_WriteByte((uint8_t)AHT20C_STATUS_WORD);
	
	I2CS_Start();
	ack = I2CS_WriteByte((uint8_t)(AHT20_I2C_ADDRESS | AHT20_I2C_READ_FLAG));
	
	*status = I2CS_ReadByte(I2CSS_NO_ACK);
	
	I2CS_Stop();

	return ack;
}	


I2CS_STATE_TypeDef AHT20_Calibrate(void)
{ 
	I2CS_STATE_TypeDef ack;
			
	I2CS_Start();
	I2CS_WriteByte((uint8_t)(AHT20_I2C_ADDRESS | AHT20_I2C_WRITE_FLAG));
	
	I2CS_WriteByte((uint8_t)AHT20C_INIT);
	I2CS_WriteByte((uint8_t)AHT20CP_INIT1);
	ack = I2CS_WriteByte((uint8_t)AHT20CP_INIT2);
	
	I2CS_Stop();
	
	return ack;
}


I2CS_STATE_TypeDef AHT20_TriggerMeasurement(void)
{ 
	I2CS_STATE_TypeDef ack;
		
	I2CS_Start();
	I2CS_WriteByte((uint8_t)(AHT20_I2C_ADDRESS | AHT20_I2C_WRITE_FLAG));
	
	I2CS_WriteByte((uint8_t)AHT20C_TRIG_MEASUREMENT);
	I2CS_WriteByte((uint8_t)AHT20CP_TRIG_MEASUREMENT1);
	ack = I2CS_WriteByte((uint8_t)AHT20CP_TRIG_MEASUREMENT2);
	
	I2CS_Stop();
	
	return ack;
}


I2CS_STATE_TypeDef AHT20_SoftReset(void)
{ 
	I2CS_STATE_TypeDef ack;
		
	I2CS_Start();
	I2CS_WriteByte((uint8_t)(AHT20_I2C_ADDRESS | AHT20_I2C_WRITE_FLAG));
	
	ack = I2CS_WriteByte((uint8_t)AHT20C_TRIG_MEASUREMENT);
	
	I2CS_Stop();
	
	return ack;
}


I2CS_STATE_TypeDef AHT20_ReadData(uint8_t *data_buff)
{
	uint8_t i, data, n;
	I2CS_STATE_TypeDef ack, ack_read;
	
	I2CS_Start();
	I2CS_WriteByte((uint8_t)(AHT20_I2C_ADDRESS | AHT20_I2C_READ_FLAG));
	
	for (i = 0; i < AHT20_MEASUREMENT_BYTE_COUNT; ++i) {
		ack_read = (I2CS_STATE_TypeDef)((i == ((uint8_t)(AHT20_MEASUREMENT_BYTE_COUNT - 1))) ? I2CSS_NO_ACK : I2CSS_ACK);
		data_buff[i] = I2CS_ReadByte(ack_read);
	}
	
	I2CS_Stop();
	
	return ack;
}
