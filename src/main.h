#include "utils.h" 

#ifndef __MAIN_H
#define __MAIN_H

typedef enum {
	LED_SYS_STATE = 0,
	LED_COUNT, //add new leds above this line
	LED_INVALID = 0xFF
} LED_ID_TypeDef;

typedef enum {
	CMDI_VERSION = 1,			//get firmware ver
	CMDI_WRITE_PAGE,			//write single page to eeprom
	CMDI_RESET,						//reset device
	CMDI_EEPROM_BYTE,			//read byte from EEPROM
	CMDI_USB_MODE,				//switch device to USB mode
	CMDI_WRITE_CONFIG,		//write to internal device EEPROM
	CMDI_UNDEFINED = 0xFF //place before this flag
} CommandIn_TypeDef;

typedef enum {
	CMDO_VERSION = 1,			//return firmware ver
	CMDO_PAGE_CRC,				//last written page CRC
	CMDO_EEPROM_BUSY,			//cannot access eeprom now
	CMDO_EEPROM_BYTE,			//byte read from eeprom
	CMDO_OK,							//command ok
	CMDO_UNDEFINED = 0xFF	//place before this flag
} CommandOut_TypeDef;

typedef enum {
	ST_UPDATE_SLOW,
	ST_KEYLOCK,
	ST_COUNT //add new tasks above this line
} SysTask_TypeDef;
//system task periods
#define STP_UPDATE_SLOW 1000 //1sec
#define STP_KEYLOCK 1500 //1sec

struct system_struct {
	struct io_struct io_volt_mon, io_led[LED_COUNT];
	uint16_t counter, verify_page_addr;
};

void MainCycle(void);
void CalcPageCRC(void);
void Clock_Init(void);
void Timer_Init(void);
void Timer_Interrupt(void);
void Event_Command(uint8_t cmd_size, uint8_t *cmd_buff);
void SleepTicks(uint8_t ticks);

#endif /* __MAIN_H */