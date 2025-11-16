#include "utils.h"

static const uint8_t HSIDivFactor[4] = {1, 2, 4, 8};
static const uint8_t RBOLookup[16] = {0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};


uint32_t GetCurClockFreq(void)
{
  uint32_t clockfrequency = 0;
  CLK_Source_TypeDef clocksource = CLK_SOURCE_HSI;
  uint8_t tmp = 0, presc = 0;
  
  /* Get CLK source. */
  clocksource = (CLK_Source_TypeDef)CLK->CMSR;
  
  if (clocksource == CLK_SOURCE_HSI)
  {
    tmp = (uint8_t)(CLK->CKDIVR & CLK_CKDIVR_HSIDIV);
    tmp = (uint8_t)(tmp >> 3);
    presc = HSIDivFactor[tmp];
    clockfrequency = HSI_VALUE / presc;
  }
  else if ( clocksource == CLK_SOURCE_LSI)
  {
    clockfrequency = LSI_VALUE;
  }
  else
  {
    clockfrequency = HSE_VALUE;
  }
  
  return((uint32_t)clockfrequency);
}


void Sleep(uint32_t t) {
    while(--t);
}


void AssignIO(
	struct io_struct *cur_io, 
	GPIO_TypeDef *cur_port, GPIO_Pin_TypeDef cur_pin, 
	bool init_io, GPIO_Mode_TypeDef cur_mode
) 
{
	cur_io->port = cur_port;
	cur_io->pin = cur_pin;
	if (init_io)
		ModeIO(cur_io, cur_mode);
}


void WriteIO(struct io_struct *cur_io, IO_MODE_TypeDef io_mode) 
{
	assert_param(IS_UTIL_IO_MODE_OK(io_mode));
	
	switch (io_mode) {
		case IO_High:
			cur_io->port->ODR |= (uint8_t)cur_io->pin;
			break;
		case IO_Low:
			cur_io->port->ODR &= (uint8_t)(~cur_io->pin);
			break;
		case IO_Reverse:
			cur_io->port->ODR ^= (uint8_t)cur_io->pin;
			break;
	}
}


void ReadIO(struct io_struct *cur_io, bool* result) 
{
	BitStatus status;
	
	status = ((BitStatus)(cur_io->port->IDR & (uint8_t)cur_io->pin));
	*result = (bool)(status ? TRUE : FALSE);
}


void ModeIO(struct io_struct *cur_io, GPIO_Mode_TypeDef cur_mode) 
{
	GPIO_TypeDef* cur_port;
	GPIO_Pin_TypeDef cur_pin;
	
  assert_param(IS_GPIO_MODE_OK(cur_mode));
  assert_param(IS_GPIO_PIN_OK(cur_io->pin));

  cur_port = cur_io->port;
	cur_pin = cur_io->pin;
	
	cur_port->CR2 &= (uint8_t)(~(cur_pin));
  
  if ((((uint8_t)(cur_mode)) & (uint8_t)0x80) != (uint8_t)0x00) { /* Output mode */
    if ((((uint8_t)(cur_mode)) & (uint8_t)0x10) != (uint8_t)0x00) /* High level */
      cur_port->ODR |= (uint8_t)cur_pin;
    else /* Low level */
      cur_port->ODR &= (uint8_t)(~(cur_pin));
    cur_port->DDR |= (uint8_t)cur_pin; /* Set Output mode */
  } else { /* Input mode */
    cur_port->DDR &= (uint8_t)(~(cur_pin)); /* Set Input mode */
  }

  if ((((uint8_t)(cur_mode)) & (uint8_t)0x40) != (uint8_t)0x00) /* Pull-Up or Push-Pull */
    cur_port->CR1 |= (uint8_t)cur_pin;
  else /* Float or Open-Drain */
    cur_port->CR1 &= (uint8_t)(~(cur_pin));
  
  if ((((uint8_t)(cur_mode)) & (uint8_t)0x20) != (uint8_t)0x00) /* Interrupt or Slow slope */
    cur_port->CR2 |= (uint8_t)cur_pin;
  else /* No external interrupt or No slope control */
    cur_port->CR2 &= (uint8_t)(~(cur_pin));
}


uint8_t DecFromBCD(uint8_t bcd_val)
{
	uint8_t dec_val;
	
	dec_val = (uint8_t)((bcd_val >> 4)*10 + (bcd_val & 0x0F));
	
	return dec_val;
}


uint8_t DecToBCD(uint8_t dec_val)
{
	uint8_t bcd_val;
	
	if (dec_val > 99)
		dec_val = 99;
		
	bcd_val = (uint8_t)(((dec_val / 10) << 4) + (dec_val % 10));
	
	return bcd_val;
}


void CRC16(uint16_t *crc, uint8_t data)
{
	uint8_t i;
	
	*crc ^= data;
	for (i = 0; i < 8; ++i)
		*crc = (*crc & 1) ? ((*crc >> 1) ^ 0xA001) : (*crc >> 1);
}


int8_t Round8S(float x)
{
    if (x < 0.0f)
        return (int8_t)(x - 0.5f);
    else
        return (int8_t)(x + 0.5f);
}


uint8_t Round8U(float x)
{
	return (uint8_t)(x + 0.5f);
}


void DataMemory_Write(uint8_t byte_no, uint8_t data)
{
	uint32_t addr;
	
	addr = FLASH_DATA_START_PHYSICAL_ADDRESS + byte_no;
	assert_param(IS_FLASH_DATA_ADDRESS_OK(addr));
	
	FLASH->DUKR = FLASH_RASS_KEY2; /* Warning: keys are reversed on data memory !!! */
	FLASH->DUKR = FLASH_RASS_KEY1;
	*(PointerAttr uint8_t*) (MemoryAddressCast)addr = data;
	FLASH->IAPSR &= (uint8_t)FLASH_MEMTYPE_DATA;
}


uint8_t DataMemory_Read(uint8_t byte_no)
{
	uint32_t addr;
	
	addr = FLASH_DATA_START_PHYSICAL_ADDRESS + byte_no;
	assert_param(IS_FLASH_DATA_ADDRESS_OK(addr));

	return(*(PointerAttr uint8_t *) (MemoryAddressCast)addr); 
}


uint8_t ReverseBitOrder(uint8_t n) {
   return (RBOLookup[n&0b1111] << 4) | RBOLookup[n>>4];
}