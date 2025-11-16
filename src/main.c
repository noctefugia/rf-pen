#include "main.h"
#include "utils.h"
#include "vibro.h"
#include "button.h"
#include "i2c_soft.h"
#include "eeprom.h"
#include "uart.h"
#include "taskman.h"
#include "oled.h"
#include "os.h"
#include "spi_soft.h"
#include "nrf24.h"
#include "aht20.h"

struct system_struct sys = {0};
extern struct os_struct os;


void main(void)
{
	disableInterrupts();
	Clock_Init();
	Sleep(100000); //5000
	
	AssignIO(&sys.io_volt_mon, PORT_VOLT_MON, PIN_VOLT_MON, TRUE, GPIO_MODE_IN_FL_NO_IT);
	AssignIO(&sys.io_led[LED_SYS_STATE], PORT_LED_A, PIN_LED_A, TRUE, GPIO_MODE_OUT_PP_LOW_SLOW);
	Timer_Init();
	Vibro_Init();
	Button_Init(&OS_Event_ButtonClick);
	I2CS_Init();
	SPIS_Init();
	NRF24_Init();
	UART_Init(&Event_Command);
	Taskman_Init(&OS_Event_TaskTick);
	Task_Create(ST_UPDATE_SLOW, TICKS16(STP_UPDATE_SLOW), TRUE, TRUE);
	
	sys.counter = 0;
	sys.verify_page_addr = 0xFFFF;
	
	OS_Init();
	MainCycle();
	WWDG_SWReset();
	return;
}


void CalcPageCRC(void)
{
	uint8_t i, temp_buff[EEPROM_PAGE_SIZE];
	uint16_t cur_crc;
	
	cur_crc = 0;
	EEPROM_Read(sys.verify_page_addr, EEPROM_PAGE_SIZE, temp_buff);
	for (i = 0; i < EEPROM_PAGE_SIZE; ++i)
		CRC16(&cur_crc, temp_buff[i]);
	temp_buff[0] = CMDO_PAGE_CRC;
	temp_buff[1] = BYTE_H(cur_crc);
	temp_buff[2] = BYTE_L(cur_crc);
	UART_Send(3, temp_buff);
}


void MainCycle(void)
{
	while (TRUE) {
		OS_Update();
		
		if ( (sys.verify_page_addr != 0xFFFF) && (!EEPROM_IsBusy()) && (UART_TXidle()) ) {
			CalcPageCRC();
			sys.verify_page_addr = 0xFFFF;
		}
	}
}

	
void Event_Command(uint8_t cmd_size, uint8_t *cmd_buff)
{
	CommandIn_TypeDef cmd_id;
	uint8_t out_buff_sz;
	uint8_t out_buff[UART_TX_PACKET_MAX_SIZE-1];
	
	os.idle_timer = 0;
	cmd_id = cmd_buff[0];
	out_buff_sz = 0;
	switch (cmd_id) {
		case CMDI_VERSION:
			out_buff[0] = CMDO_VERSION;
			out_buff[1] = CORE_VER_MAJOR;
			out_buff[2] = CORE_VER_MINOR;
			out_buff_sz = 3;
			break;
			
		case CMDI_WRITE_PAGE:
			if ( (EEPROM_IsBusy()) || (sys.verify_page_addr != 0xFFFF) ) {
				out_buff[0] = CMDO_EEPROM_BUSY;
				out_buff_sz = 1;
			} else {
				sys.verify_page_addr = WORD_HL(cmd_buff[1], cmd_buff[2]);
				assert_function((EEPROM_Write(sys.verify_page_addr, EEPROM_PAGE_SIZE, &cmd_buff[3]) == I2CSS_ACK));
			}
			break;
			
		case CMDI_RESET:
			WWDG_SWReset();
			break;
			
		case CMDI_EEPROM_BYTE:
			assert_function((EEPROM_Read(WORD_HL(cmd_buff[1], cmd_buff[2]), 1, &out_buff[1]) == I2CSS_ACK));
			out_buff[0] = CMDO_EEPROM_BYTE;
			out_buff_sz = 2;
			break;
			
		case CMDI_USB_MODE:
			OS_ShowObject(OSO_USB_MODE);
			OLED_Update();
			break;
		
		case CMDI_WRITE_CONFIG:
			DataMemory_Write(cmd_buff[1], cmd_buff[2]);
			out_buff[0] = CMDO_OK;
			out_buff_sz = 1;
			break;
			
		default:
			out_buff[0] = CMDO_UNDEFINED;
			out_buff_sz = 1;
	}
	if (out_buff_sz > 0)
		UART_Send(out_buff_sz, out_buff);
}


void Clock_Init(void)
{
	/*CLK_DeInit();
								 
	CLK_HSECmd(DISABLE);
	CLK_LSICmd(DISABLE)*/;
	//while(CLK_GetFlagStatus(CLK_FLAG_LSIRDY) == FALSE)
		//;
	/*CLK_HSICmd(ENABLE);
	while(CLK_GetFlagStatus(CLK_FLAG_HSIRDY) == FALSE)
		;*/
	//*removed*CLK->ICKR |= CLK_ICKR_HSIEN; 
	//*removed*while (!((CLK->ICKR & (uint8_t)CLK_FLAG_HSIRDY) != (uint8_t)RESET))
		//*removed*;
	
	/*CLK_ClockSwitchCmd(ENABLE);
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
	CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1);
	*/
	//*removed*CLK->SWCR |= CLK_SWCR_SWEN;
	//*removed*CLK->CKDIVR &= (uint8_t)(~CLK_CKDIVR_HSIDIV);
	//CLK->CKDIVR |= (uint8_t)CLK_PRESCALER_HSIDIV1; useless: (x|0x00) = x
	//*removed*CLK->CKDIVR &= (uint8_t)(~CLK_CKDIVR_CPUDIV);
	//CLK->CKDIVR |= (uint8_t)((uint8_t)CLK_PRESCALER_CPUDIV1 & (uint8_t)CLK_CKDIVR_CPUDIV); useless: (x|0x00) = x
	CLK->CKDIVR = 0;
	//CLK_ClockSwitchConfig(CLK_SWITCHMODE_AUTO, CLK_SOURCE_HSI, DISABLE, CLK_CURRENTCLOCKSTATE_ENABLE);
		
	/*CLK_PeripheralClockConfig(CLK_PERIPHERAL_SPI, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_ADC, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_AWU, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART1, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER4, DISABLE);*/
	CLK->PCKENR1 |= (uint8_t)((uint8_t)1 << ((uint8_t)CLK_PERIPHERAL_UART1));
	CLK->PCKENR1 |= (uint8_t)((uint8_t)1 << ((uint8_t)CLK_PERIPHERAL_TIMER1));
	CLK->PCKENR1 |= (uint8_t)((uint8_t)1 << ((uint8_t)CLK_PERIPHERAL_TIMER2));
	
	//sys.cpu_freq = GetCurClockFreq();
}


void Timer_Init(void)
{
	uint16_t tim_period;
	
	//TIM1_DeInit();
	tim_period = (uint16_t)(CORE_FCPU / (1024 * (1000 / CORE_PERIOD)));
	/*TIM1_TimeBaseInit(1024, TIM1_COUNTERMODE_UP, tim_period, 0); 
	TIM1_ITConfig(TIM1_IT_UPDATE, ENABLE);
	TIM1_Cmd(ENABLE);*/
	
  /* Set the Autoreload value */
  TIM1->ARRH = (uint8_t)(tim_period >> 8);
  TIM1->ARRL = (uint8_t)(tim_period);
  /* Set the Prescaler value */
  TIM1->PSCRH = (uint8_t)(1024 >> 8);
  TIM1->PSCRL = (uint8_t)(1024);
  /* Select the Counter Mode */
  TIM1->CR1 = (uint8_t)((uint8_t)(TIM1->CR1 & (uint8_t)(~(TIM1_CR1_CMS | TIM1_CR1_DIR)))
                        | (uint8_t)(TIM1_COUNTERMODE_UP));
  /* Set the Repetition Counter value */
  TIM1->RCR = 0;
	
	TIM1->IER |= (uint8_t)TIM1_IT_UPDATE;
	
	TIM1->CR1 |= TIM1_CR1_CEN;
}


void Timer_Interrupt(void)
{
	++sys.counter;
	
	/* update code begin */
	Vibro_Update();
	Button_Update();
	UART_Update();
	Taskman_Update(sys.counter);
	/* update code end */
	
	/*TIM1_ClearFlag(TIM1_FLAG_UPDATE);*/
  TIM1->SR1 = (uint8_t)(~(uint8_t)(TIM1_FLAG_UPDATE));
  TIM1->SR2 = (uint8_t)((uint8_t)(~((uint8_t)((uint16_t)TIM1_FLAG_UPDATE >> 8))) & 
                        (uint8_t)0x1E);
}





void SleepTicks(uint8_t ticks)
{
	uint16_t last_time;
	
	last_time = sys.counter;
	while ((sys.counter - last_time) < (uint16_t)ticks)
		;
}


#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(u8* file, u32 line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
	disableInterrupts();
  while (1)
  {
		Sleep(10000);
		WriteIO(&sys.io_led[LED_SYS_STATE], IO_Reverse);
  }
}
#endif

/*
> .text, .const and .vector are ROM
> .bsct, .ubsct, .data and .bss are all RAM
> .info. and .debug are symbol tables that do not use target resources/memory.
*/
