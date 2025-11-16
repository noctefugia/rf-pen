#include "os.h"
#include "main.h"
#include "oled.h"
#include "vibro.h"
#include "button.h"
#include "taskman.h"
#include "eeprom.h"
#include "nrf24.h"
#include "aht20.h"

extern struct system_struct sys;
extern struct oled_struct oled;
struct os_struct os;


void OS_ShowChildObject(OS_OBJ_TypeDef id)
{
	assert_param(IS_OS_OBJ_OK(id));
	
	os.wait_object = id;
}


void OS_UpdateActiveObject(void)
{
	if (os.parent_object == OSO_NONE) 
		os.update_obj_flag = TRUE;
}


void OS_PrintTimerDigit(uint8_t val, uint8_t x)
{
	uint8_t i, n;
	
	for(i = 0; i < 2; ++i) {
		n = (i == 0) ? val / 10 : val % 10;
		OLED_SetChar(DIGIT_TO_ASCII(n), x++, 2);
	}
}


void OS_ShowSelectedLine(void)
{
	OLED_InverseArea(&os.cursor_pos, OLED_SIZE_X);
}


void OS_WakeUp(void)
{
	OLED_Command(OLED_DISPLAY_ON);
	os.idle_timer = 0;
	Button_Enable(FALSE); //disable input for a while
	Task_Create(ST_KEYLOCK, TICKS16(STP_KEYLOCK), TRUE, FALSE);
}

			
void OS_ShowObject(OS_OBJ_TypeDef id)
{
	uint8_t i, c1, c2, char_code, buff[OLED_TOTAL_SIZE];
	uint16_t addr;
	uint32_t temp_raw, hmdt_raw;
	
	c1 = id & OS_MSGBOX_MASK;
	id &= (~OS_MSGBOX_MASK);
	assert_param(IS_OS_OBJ_OK(id));
	
	OLED_Clear();
	os.parent_object = OSO_NONE;
	if ( (id >= OS_OBJ_WCAP_BEGIN) && (id < OS_OBJ_WCAP_END) )
		OLED_PrintString((OLED_STR_TypeDef)(CAPTION_STR_BEGIN + id - OS_OBJ_WCAP_BEGIN), 0);
	if (id != OSO_MSGBOX) {
		OLED_SetChar((os.normal_power ? OLED_ECHAR_BATT_H : OLED_ECHAR_BATT_L), 20, 0);
		if (os.rf_state == NRF24_LMS_TRANSMISSON_OK)
			OLED_SetChar(OLED_ECHAR_RF_ONLINE, 19, 0);
	}
	
	switch (id) {
		case OSO_LOGO:
			OLED_DrawImage(OLED_CORE_LOGO_ADDR);
			break;
			
		case OSO_MAIN_MENU:
			c1 = MENU_STR_PG1_BEGIN + (os.menu_page_no * MENU_PG_SIZE);
			c2 = c1 + MENU_PG_SIZE;
			OLED_PrintStringList(1, c1, c2);
			OS_ShowSelectedLine();
			break;
			
		case OSO_MSGBOX:
			OS_WakeUp();
			os.parent_object = os.active_object;
			if (c1 == OSMBT_RF_MESSAGE)
				OLED_PrintStringList(1, RF_MSG_STR_BEGIN, RF_MSG_STR_END);
			else
				OLED_PrintString((c1 ? OLEDSTR_OPERATION_OK : OLEDSTR_LOW_BATT), 2);
			Vibro_Tone(VT_ALERT);
			break;
			
		case OSO_ABOUT:
			if (os.flashlight) {
				OLED_DrawImage(OLED_FHASHLIGHT_KEYWORD);
			} else {
				OLED_PrintStringList(1, ABOUT_STR_BEGIN, ABOUT_STR_END);
				OLED_SetChar(DIGIT_TO_ASCII(CORE_VER_MAJOR), 18, 1);
				OLED_SetChar(DIGIT_TO_ASCII(CORE_VER_MINOR), 20, 1);
				OLED_SetChar(DIGIT_TO_ASCII(PCB_REVISION), 20, 2);
			}
			break;
			
		case OSO_APP_NOTES:
			switch (os.notes_state) {
				case OSNS_MENU:
					OLED_PrintStringList(1, NOTES_STR_BEGIN, NOTES_STR_END);
					OS_ShowSelectedLine();
					break;
					
				case OSNS_STATIC:
					assert_function((EEPROM_Read(os.notes_addr, OLED_TOTAL_SIZE, buff) == I2CSS_ACK));
					c1 = c2 = 0;
					for (i = 0; i < OLED_TOTAL_SIZE; ++i) {
						os.app_cur_val = buff[i];
						if (os.app_cur_val == '\0') {
							break;
						} else if ( (os.eeprom_action == 0) && (os.app_cur_val >= OLED_ECHAR_FIRST_IMG_ID) && (os.app_cur_val <= OLED_ECHAR_LAST_IMG_ID) ) {
								OLED_DrawImage((uint16_t)(OS_NOTES_DM_ADDR - ((uint16_t)(os.app_cur_val-(OLED_ECHAR_FIRST_IMG_ID-1))*OLED_BYTE_COUNT)));
								++os.eeprom_action;
							break;
						} else {
							OLED_SetChar(os.app_cur_val, c1, c2);
							if ((++c1) >= OLED_SIZE_X) {
								c1 = 0;
								++c2;
							}
						}
					}
					break;
					
				case OSNS_DYNAMIC:
					addr = OS_NOTES_DM_ADDR + os.notes_dm_page*OS_NOTES_DM_PAGE_SZ;
					for (c2 = 1; c2 < OLED_SIZE_Y; ++c2) {
						for (c1 = 0; c1 < OLED_SIZE_X; ++c1) {
							if (os.eeprom_action) {
								char_code = (os.eeprom_action == 2) ? os.app_cur_val : ' ';
								if ( (os.eeprom_action == 1) || ((c1 == os.cursor_pos.x) && (c2 == os.cursor_pos.y)) ) {
									assert_function((EEPROM_Write(addr, 1, &char_code) == I2CSS_ACK));
									while (EEPROM_IsBusy())
										;
								}
							}
							assert_function((EEPROM_Read(addr++, 1, &char_code) == I2CSS_ACK));
							OLED_SetChar(char_code, c1, c2); 
						}
					}
					OLED_PrintNumber(os.notes_dm_page, 1, 0);
					OLED_InverseArea(&os.cursor_pos, 1);
					os.eeprom_action = 0;
					break;
					
				case OSNS_DYN_MENU:
					OLED_PrintStringList(0, NDMENU_STR_BEGIN, NDMENU_STR_END);
					OS_ShowSelectedLine();
					break;
					
				case OSNS_KEYBOARD:
					OLED_PrintStringList(0, KEYBOARD_STR_BEGIN, KEYBOARD_STR_END);
					OLED_InverseArea(&os.cursor_pos2, 1);
					break;
			}
			break;
			
		case OSO_APP_CALC:
			OLED_PrintNumber(os.calc_v[1], 20, 1);
			if (!os.calc_result_flag) {
				OLED_PrintNumber(os.calc_v[0], 20, 2);
				OLED_SetChar(os.calc_state, 0, 2);
			}
			OLED_PrintString(OLEDSTR_APP_CALC_KEYS, 3);
			OLED_InverseArea(&os.cursor_pos, 1);
			break;
			
		case OSO_APP_CONFIG:
			OLED_PrintString((OLED_STR_TypeDef)(CONFIG_STR_BEGIN + os.config_pos - OS_CONFIG_BEGIN), 2);
			if (os.config_pos >= OS_CONFIG_BEGIN)
				OLED_PrintNumber((int32_t)os.app_cur_val, 20, 2);
			break;
			
		case OSO_APP_TIMER:
			OS_PrintTimerDigit(os.timer_min, 8);
			OLED_SetChar(':', 10, 2);
			OS_PrintTimerDigit(os.timer_sec, 11);
			break;
			
		case OSO_APP_IR_REMOTE:
			break;
			
		case OSO_APP_SENSOR:
			OLED_PrintStringList(2, SENSOR_STR_BEGIN, SENSOR_STR_END);
			assert_function((AHT20_ReadData(buff) == I2CSS_ACK));
			if ((buff[0] & AHT20_BIT_BUSY) == 0) {
				//temp_raw = DWORD_HML((buff[3] & 0x0F), buff[4], buff[5]);
				//hmdt_raw = (uint32_t)(DWORD_HML(buff[1], buff[2], buff[3]) >> 4);
				temp_raw = buff[3] & 0x0F;
				temp_raw <<=8;
				temp_raw += buff[4];
				temp_raw <<=8;
				temp_raw += buff[5];
				temp_raw *= 1000;
				temp_raw >>= 20;
				temp_raw /= 5;
				temp_raw -= 50;
				OLED_PrintNumber(temp_raw, 18, 2);
				hmdt_raw = buff[1];
				hmdt_raw <<= 8;
				hmdt_raw += buff[2];
				hmdt_raw <<= 4;
				hmdt_raw += buff[3] >> 4;
				hmdt_raw *= 100;
				hmdt_raw >>= 20;
				OLED_PrintNumber(hmdt_raw, 19, 3);
			}
			assert_function((AHT20_TriggerMeasurement() == I2CSS_ACK));
			break;
			
		case OSO_USB_MODE:
			Button_Enable(FALSE);
			Task_Enable(ST_UPDATE_SLOW, FALSE);
			OLED_PrintStringList(0, USBMODE_STR_BEGIN, USBMODE_STR_END);
			break;
	}
	
	os.active_object = id;
	os.update_obj_flag = FALSE;
	OLED_Redraw();
}


void OS_Init(void)
{
	os.parent_object = os.wait_object = OSO_NONE;
	os.rf_state = os.normal_power = TRUE;
	os.idle_timer = os.rf_action = 0;
	os.oled_off_flag = os.reboot_flag = os.btn_event_flag = FALSE;
	
	if (os.rf_en = (bool)DataMemory_Read(OSOB_RF_EN))
		NRF24_Config(DataMemory_Read(OSOB_RF_CHANNEL), DataMemory_Read(OSOB_RF_ADDRESS));
	assert_function((AHT20_Calibrate() == I2CSS_ACK));
	Vibro_Enable((OLED_CONTRAST_TypeDef)DataMemory_Read(OSOB_VIBRO_EN));
	os.effects_en = (bool)DataMemory_Read(OSOB_EFFECTS_EN);
	os.idle_timer_thr = DataMemory_Read(OSOB_OLED_AUTOSLP);
	OLED_Init((OLED_CONTRAST_TypeDef)DataMemory_Read(OSOB_OLED_CONTRAST), DataMemory_Read(OSOB_OLED_ROTATION));
	OS_ShowObject(OSO_LOGO);
	OLED_Update();
	
	enableInterrupts();
	Vibro_Tone(VT_INIT);
	SleepTicks(TICKS8(OS_LOGO_TIME));
	OS_ShowMainMenu();
}


void OS_ShowMainMenu(void)
{
	os.flashlight = FALSE;
	os.cursor_pos.x = os.menu_page_no = 0;
	os.cursor_pos.y = 1;
	OS_ShowObject(OSO_MAIN_MENU);
}


void OS_Update(void)
{
	uint8_t rf_buff[NRF24_PAYLOAD_LEN];
	
	if (os.active_object == OSO_USB_MODE)
		return;
	
	if (os.rf_en) {
		if (NRF24_DataReady()) {
			NRF24_GetData(rf_buff);
			if ( (rf_buff[0] >= 1) && (rf_buff[0] <= 3) ) {
			if (rf_buff[0] == 3) //[!]ADDRESS SHOULD BE ALIGNED WITHIN EEPROM PAGE
					OS_ShowChildObject(OSO_MSGBOX | OSMBT_RF_MESSAGE);
				assert_function((EEPROM_Write(OLED_CORE_STRINGS_ADDR + (RF_MSG_STR_BEGIN-1)*OLED_STRING_BUFF_SZ + rf_buff[0]*OLED_STRING_BUFF_SZ, OLED_STRING_BUFF_SZ, &rf_buff[1]) == I2CSS_ACK));
				while (EEPROM_IsBusy())
					;
			}
		} else if (os.rf_action) {
			rf_buff[0] = 0;
			if (os.rf_action & 0x04) {
				do {
					assert_function((EEPROM_Read(OS_NOTES_DM_ADDR + os.notes_dm_page*OS_NOTES_DM_PAGE_SZ + (rf_buff[0]++)*OLED_STRING_BUFF_SZ, OLED_STRING_BUFF_SZ, &rf_buff[1]) == I2CSS_ACK));
					NRF24_Send(rf_buff);
					SleepTicks(TICKS8(100));
					if (NRF24_LastMessageStatus() != NRF24_LMS_TRANSMISSON_OK)
						goto rf_tx_fail;
				} while (os.rf_action >>= 1);
				OS_ShowChildObject(OSO_MSGBOX | OSMBT_OPERATION_OK);
			} else {
				NRF24_Send(rf_buff);
				while (NRF24_IsSending())
					;
				os.rf_state = NRF24_LastMessageStatus();
				NRF24_PowerUpRx();
				if (!((os.active_object == OSO_APP_NOTES) && (os.notes_state == OSNS_STATIC)))
					OS_UpdateActiveObject();
			}
rf_tx_fail:
			os.rf_action = 0;
		}
	}
				
	if (Button_IsDown(oled.rotation) > OS_EXTRA_CLICK_TIME)
		os.extra_click_flag = TRUE;
		
	if (os.btn_event_flag) {
		OS_ButtonEventHandler();
		os.btn_event_flag = FALSE;
	}
		
	if (os.update_obj_flag)
		OS_ShowObject(os.active_object);
	
	if ( (os.wait_object != OSO_NONE) && (os.parent_object == OSO_NONE) ) { //no other active child objects
		OS_ShowObject(os.wait_object);
		os.wait_object = OSO_NONE;
	}
	
	if (os.oled_off_flag) {
		OLED_Command(OLED_DISPLAY_OFF);
		os.oled_off_flag = FALSE;
	}
	OLED_Update();
}


void OS_ResetTimerApp(void)
{
	os.timer_sec = 0;
	os.timer_min_backup = os.timer_min = DataMemory_Read(OSOB_TIMER_MIN);
	os.timer_en = FALSE;
}


void OS_ResetCalcApp(void)
{
	os.cursor_pos.y = 3;
	os.cursor_pos.x = 0;
	os.calc_state = ' ';
	os.calc_v[0] = os.calc_v[1] = 0;
	os.calc_mode2_flag = os.calc_result_flag = FALSE;
}


void OS_Event_ButtonClick(uint8_t index, bool long_click)
{
	os.btne_index = index;
	os.btne_long_click = long_click;
	os.btn_event_flag = TRUE;
	if (oled.rotation)
		--os.btne_index;
}


uint8_t OS_ButtonScrollVariable(uint8_t *var, uint8_t min, uint8_t max)
{
	uint8_t min2;
	
	min2 = min - 1;
	if (os.btne_index == 0) {
		if ((--*var) == min2) {
			*var = max;
			return 1;
		}
	} else {
		if ((++*var) > max) {
			*var = min;
			return 1;
		}
	}
	
	return 0;
}


void OS_ButtonEventHandler(void)
{
	uint8_t pulse_t, last_y, cur_char, buff[OLED_TOTAL_SIZE], i;
	OS_OBJ_TypeDef cur_obj;
	
	if (os.idle_timer > os.idle_timer_thr) {
		OLED_Command(OLED_DISPLAY_ON);
		goto btne_skip;
	}
	
	if ( (os.btne_long_click) && (os.btne_index == 0) && (os.active_object >= OS_APPS_BEGIN) && (os.active_object < OS_APPS_END) \
	&& ( (os.active_object != OSO_APP_NOTES) || (os.extra_click_flag)) ) {
		if (os.reboot_flag)
			WWDG_SWReset();
		OS_ShowMainMenu();
		goto btne_skip;
	}
		
	switch (os.active_object) {
		case OSO_MSGBOX:
			OS_ShowObject(os.parent_object);
			break;

		case OSO_MAIN_MENU:
			if (os.btne_long_click) {
				if (os.btne_index == 0) {
					OS_ShowObject(OSO_ABOUT);
				} else {
					last_y = os.cursor_pos.y - 1;
					OS_ResetTimerApp();
					OS_ResetCalcApp();
					os.config_pos = OS_CONFIG_BEGIN - 1;
					cur_obj = OS_APPS_BEGIN + last_y + (3*os.menu_page_no);
					if (cur_obj == OSO_APP_NOTES) {
						os.notes_state = OSNS_MENU;
						os.cursor_pos.y = 1;
					}
					OS_ShowObject(cur_obj);
				}
			} else {
				if (os.btne_index == 0)
					--os.cursor_pos.y;
				else
					++os.cursor_pos.y;
				if ( (os.cursor_pos.y == 0) || (os.cursor_pos.y > 3) ) {
					os.menu_page_no = (os.menu_page_no == 0) ? 1 : 0;
					os.cursor_pos.y = (os.btne_index == 0) ? 3 : 1;
				}
			}
			OS_UpdateActiveObject();
			break;
			
		case OSO_ABOUT:
			if (os.btne_long_click) {
				os.flashlight = !os.flashlight;
				OS_UpdateActiveObject();
			} else {
				OS_ShowMainMenu();
			}
			break;
			
		case OSO_APP_TIMER:
			if (os.btne_long_click) {
				os.timer_en = !os.timer_en;
				os.timer_sec = 0; os.timer_min = os.timer_min_backup;
			} else if (!os.timer_en) {
				OS_ButtonScrollVariable(&os.timer_min, 1, 99);
				os.timer_min_backup = os.timer_min;
				DataMemory_Write(OSOB_TIMER_MIN, os.timer_min);
			}
			OS_UpdateActiveObject();
			break;
			
		case OSO_APP_CALC:
			if (os.calc_result_flag)
				os.calc_result_flag = FALSE;
			if (os.btne_long_click) {
				cur_char = OLED_GetChar(os.cursor_pos.x, os.cursor_pos.y);
				if ( (cur_char >= '0') && (cur_char <= '9') ) {
					os.calc_v[0] = (os.calc_v[0] * 10) + DIGIT_FROM_ASCII(cur_char);
				} else if (cur_char == 'C') {
					OS_ResetCalcApp();
				} else if (cur_char == 'E') {
					os.calc_v[0] = 0;
				} else if ( (cur_char == '=') && (os.calc_state != ' ') ) {
					if ( (os.calc_state == '-') && (os.calc_v[0] >= 0) )
						os.calc_v[0] = -os.calc_v[0];
					switch (os.calc_state) {
						case '+':
						case '-':
							os.calc_v[1] += os.calc_v[0];
							break;
						case '*':
							os.calc_v[1] *= os.calc_v[0];
							break;
						case '/':
							os.calc_v[1] = os.calc_v[1] / os.calc_v[0];
							break;
					}
					os.calc_mode2_flag = os.calc_result_flag = TRUE;
				} else {
					os.calc_state = cur_char;
					if (!os.calc_mode2_flag) {
						os.calc_v[1] = os.calc_v[0];
						os.calc_v[0] = 0;
					}
				}
			} else {
move_calc_cursor:
				OS_ButtonScrollVariable(&os.cursor_pos.x, 0, (OLED_SIZE_X - 1));
				if (OLED_GetChar(os.cursor_pos.x, os.cursor_pos.y) == ' ')
					goto move_calc_cursor;
			}
			OS_UpdateActiveObject();
			break;
			
		case OSO_APP_CONFIG:
			if ( (os.btne_long_click) && (os.config_pos >= OS_CONFIG_BEGIN) ) {
				switch (os.config_pos) {
					case OSOB_OLED_CONTRAST:
						if (os.app_cur_val < OLEDCT_MEDIUM)
							os.app_cur_val = OLEDCT_MEDIUM;
						else if (os.app_cur_val < OLEDCT_HIGH)
							os.app_cur_val = OLEDCT_HIGH;
						else
							os.app_cur_val = OLEDCT_LOW;
						break;

					case OSOB_OLED_AUTOSLP:
						if ((os.app_cur_val+=15) > 180)
							os.app_cur_val = 0;
						break;

					case OSOB_RF_CHANNEL:
						if ((++os.app_cur_val) > 127)
							os.app_cur_val = 0;
						break;

					case OSOB_RF_ADDRESS:
						++os.app_cur_val;
						break;

					default:
						os.app_cur_val = !os.app_cur_val;
				}
				DataMemory_Write(os.config_pos, os.app_cur_val);
				os.reboot_flag = TRUE;
			} else  {
				OS_ButtonScrollVariable(&os.config_pos, OS_CONFIG_BEGIN, OS_CONFIG_END);
				os.app_cur_val = DataMemory_Read(os.config_pos);
			}
			OS_UpdateActiveObject();
			break;
			
		case OSO_APP_NOTES:
			switch (os.notes_state) {
				case OSNS_MENU:
					if (os.btne_long_click) {
						os.notes_state = os.cursor_pos.y;
						os.notes_dm_page = DataMemory_Read(OSOB_NOTES_DM_PAGE);
						os.cursor_pos.y = 1;
						os.eeprom_action = 0;
						os.cursor_pos2.x = os.cursor_pos2.y = 0;
						assert_function((EEPROM_Read(OLED_CORE_EEPCONFIG_ADDR, 2, (uint8_t *)&os.notes_addr) == I2CSS_ACK));
					} else {
						OS_ButtonScrollVariable(&os.cursor_pos.y, 1, (NOTES_STR_END - NOTES_STR_BEGIN));
					}
					break;
				
				case OSNS_DYNAMIC:
					if (os.btne_long_click) {
						if (os.btne_index == 0) {
							os.notes_state = OSNS_DYN_MENU;
							os.cursor_pos.x = os.cursor_pos.y = 0;
						} else {
							os.notes_state = OSNS_KEYBOARD;
						}
					} else {
						if (OS_ButtonScrollVariable(&os.cursor_pos.x, 0, (OLED_SIZE_X - 1)))
							OS_ButtonScrollVariable(&os.cursor_pos.y, 1, (OLED_SIZE_Y - 1));
					}
					break;
				
				case OSNS_STATIC:
					if ((os.eeprom_action++) == 1) {
						break;
					}
spage_next_search:
					if (os.btne_index != 0) { 
						if (os.app_cur_val != '\0')
							os.notes_addr += OLED_TOTAL_SIZE;
					} else {
						os.notes_addr -= OLED_TOTAL_SIZE;
					}
					if (os.btne_long_click) {
						assert_function((EEPROM_Read(os.notes_addr, OLED_TOTAL_SIZE, buff) == I2CSS_ACK));
						for (i = 0; i < OLED_TOTAL_SIZE; ++i) {
							cur_char = buff[i];
							if ( (cur_char == '\0') || (cur_char == OLED_ECHAR_PAGE_HEADER) ) {
								os.notes_addr += i;
								goto spage_stop_search;
							}
						}
						goto spage_next_search;
					} 
spage_stop_search:
					os.eeprom_action = 0;
					if (os.notes_addr < OLED_CUSTOM_DATA_ADDR)
						os.notes_addr = OLED_CUSTOM_DATA_ADDR;
					assert_function((EEPROM_Write(OLED_CORE_EEPCONFIG_ADDR, 2, (uint8_t *)&os.notes_addr) == I2CSS_ACK));
					while (EEPROM_IsBusy())
						;
					break;
					
				case OSNS_DYN_MENU:
					if (os.btne_long_click) {
						if (os.btne_index != 0) {
							switch (os.cursor_pos.y) {
								case 0:
									if ((++os.notes_dm_page) >= OS_NOTES_DM_PAGE_COUNT)
										os.notes_dm_page = 0;
									break;
								case 1:
									if ((--os.notes_dm_page) >= OS_NOTES_DM_PAGE_COUNT)
										os.notes_dm_page = (OS_NOTES_DM_PAGE_COUNT - 1);
									break;
								case 2:
									os.rf_action |= 0x04;
									break;
								default:
									os.eeprom_action = 1;
									break;
							}
							DataMemory_Write(OSOB_NOTES_DM_PAGE, os.notes_dm_page);
						}
						os.cursor_pos.y = 1;
						os.notes_state = OSNS_DYNAMIC;
					} else {
						OS_ButtonScrollVariable(&os.cursor_pos.y, 0, (NDMENU_STR_END - NDMENU_STR_BEGIN - 1));
					}
					break;
					
				case OSNS_KEYBOARD:
					if (os.btne_long_click) {
						if (os.btne_index != 0) {
							os.app_cur_val = OLED_GetChar(os.cursor_pos2.x, os.cursor_pos2.y);
							os.eeprom_action = 2;
							os.notes_state = OSNS_DYNAMIC;
						} else {
							OS_ButtonScrollVariable(&os.cursor_pos2.y, 0, (OLED_SIZE_Y - 1));
						}
					} else {
						if (OS_ButtonScrollVariable(&os.cursor_pos2.x, 0, (OLED_SIZE_X - 1)))
							OS_ButtonScrollVariable(&os.cursor_pos2.y, 0, (OLED_SIZE_Y - 1));
					}
					break;
			}
			OS_UpdateActiveObject();
			break;
	}
	
btne_skip:
	os.idle_timer = 0;
	os.extra_click_flag = FALSE;
	if (os.effects_en) {
		pulse_t = (os.btne_long_click) ? TICKS8(OS_BTN_LONG_CLICK_PULSE_T) : TICKS8(OS_BTN_CLICK_PULSE_T);
		Vibro_Pulse(OS_BTN_CLICK_PULSE_PWR, pulse_t);
	}
}


void OS_Event_TaskTick(uint8_t index)
{	/* WARNING: DO NOT USE I2C-RELATED FUNCTIONS INSIDE THIS SUB 
	TO PREVENT INTERFACE ACCESS COLLISIONS */
	static bool last_pwr_state = TRUE;
	static uint8_t counter = 1;
	
	switch (index) {
			
		case ST_UPDATE_SLOW:
			if (os.idle_timer_thr > 0) {
				if (os.idle_timer == os.idle_timer_thr)
					os.oled_off_flag = TRUE;
				if ( (os.idle_timer <= os.idle_timer_thr) && (!os.flashlight) )
					++os.idle_timer;
			}
				
			if (os.effects_en)
				WriteIO(&sys.io_led[LED_SYS_STATE], IO_Reverse);
			if (os.active_object != OSO_LOGO) {
				ReadIO(&sys.io_volt_mon, &os.normal_power);
				if ( (os.normal_power != last_pwr_state) && (!os.normal_power) )
					OS_ShowChildObject(OSO_MSGBOX | OSMBT_LOW_BATT);
				last_pwr_state = os.normal_power;
			}
			
			if ( (os.active_object == OSO_APP_TIMER) && (os.timer_en) ) {
				if ((--os.timer_sec) == 0xFF) {
					os.timer_sec = 59;
					if (os.timer_min == 0) {
						OS_ResetTimerApp();
						OS_WakeUp();
						Vibro_Tone(VT_TIME_IS_UP);
					} else {
						--os.timer_min;
					}
				}
				OS_UpdateActiveObject();
			} else if (os.active_object == OSO_APP_SENSOR) {
				OS_UpdateActiveObject();
			}
			
			if (!(--counter)) {
				++os.rf_action;
				counter = OS_RF_CHECK_PERIOD;
			}
			break;
			
		case ST_KEYLOCK:
			Button_Enable(TRUE);
			break;
				
		#ifdef USE_FULL_ASSERT
		default:
			assert_failed((uint8_t *)__FILE__, __LINE__);
		#endif
	}
}