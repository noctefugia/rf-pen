#include "utils.h" 

#ifndef __OS_H
#define __OS_H

#define OS_LOGO_TIME 1750
#define OS_BTN_CLICK_PULSE_T 50
#define OS_BTN_LONG_CLICK_PULSE_T 100
#define OS_BTN_CLICK_PULSE_PWR 75
#define OS_MAX_IDLE_TIME 30
#define OS_EXTRA_CLICK_TIME TICKS8(1000)
#define OS_RF_CHECK_PERIOD 5

#define IS_OS_OBJ_OK(id) \
	((id) < (OSO_INVALID)) 
	
typedef enum {
	OSO_NONE = 0,
	OSO_LOGO,
	OSO_MAIN_MENU,
	OSO_MSGBOX,
	OSO_ABOUT,
	OSO_APP_NOTES,
	OSO_APP_CALC,
	OSO_APP_CONFIG,
	OSO_APP_TIMER,
	OSO_APP_IR_REMOTE,
	OSO_APP_SENSOR,
	OSO_USB_MODE,
	OSO_INVALID //add new objets above this line
} OS_OBJ_TypeDef;
#define OS_OBJ_WCAP_BEGIN OSO_MAIN_MENU	//objects with captions list
#define OS_OBJ_WCAP_END (OSO_APP_SENSOR + 1)
#define OS_APPS_BEGIN OSO_APP_NOTES
#define OS_APPS_END (OSO_APP_SENSOR + 1)

typedef enum {
	OSOB_TIMER_MIN = 0,
	OSOB_VIBRO_EN,
	OSOB_EFFECTS_EN,
	OSOB_OLED_CONTRAST,
	OSOB_OLED_AUTOSLP,
	OSOB_OLED_ROTATION,
	OSOB_RF_CHANNEL,
	OSOB_RF_ADDRESS,
	OSOB_RF_EN,
	OSOB_NOTES_DM_PAGE,
	OSOB_INVALID //add new objets above this line
} OS_OPT_BYTE_TypeDef;
#define OS_CONFIG_BEGIN OSOB_VIBRO_EN
#define OS_CONFIG_END OSOB_RF_EN

typedef enum {
	OSNS_MENU = 0,
	OSNS_STATIC,
	OSNS_DYNAMIC,
	OSNS_DYN_MENU,
	OSNS_KEYBOARD,
	OSNS_INVALID //add new objets above this line
} OS_NOTES_STATE_TypeDef;
#define OS_NOTES_DM_PAGE_COUNT 65
#define OS_NOTES_DM_PAGE_SZ (OLED_SIZE_X*3)
#define OS_NOTES_DM_ADDR (EEPROM_MAX_ADDR - OS_NOTES_DM_PAGE_COUNT*OS_NOTES_DM_PAGE_SZ)

enum OS_MSGBOX_TYPE {
	OSMBT_LOW_BATT = 0x00,
	OSMBT_RF_MESSAGE = 0x40,
	OSMBT_OPERATION_OK = 0x80
};
#define OS_MSGBOX_MASK (OSMBT_OPERATION_OK | OSMBT_RF_MESSAGE | OSMBT_LOW_BATT) 

struct os_struct {
	OS_OBJ_TypeDef active_object, parent_object, wait_object;
	bool normal_power, flashlight, timer_en, update_obj_flag, btn_event_flag, btne_long_click, calc_result_flag, calc_mode2_flag, effects_en, reboot_flag, extra_click_flag, rf_state, rf_en, oled_off_flag;
	uint8_t menu_page_no, timer_min, timer_sec, timer_min_backup, btne_index, calc_state, config_pos, app_cur_val, idle_timer, idle_timer_thr, notes_dm_page, eeprom_action, rf_action;
	struct pos2d_struct cursor_pos, cursor_pos2;
	int32_t calc_v[2];
	OS_NOTES_STATE_TypeDef notes_state;
	uint16_t notes_addr;
};


void OS_ShowObject(OS_OBJ_TypeDef id);
void OS_Init(void);
void OS_Event_ButtonClick(uint8_t index, bool long_click);
void OS_Event_TaskTick(uint8_t index);
void OS_Update(void);
void OS_ShowChildObject(OS_OBJ_TypeDef id);
void OS_UpdateActiveObject(void);
void OS_ShowMainMenu(void);
void OS_ButtonEventHandler(void);
void OS_PrintTimerDigit(uint8_t val, uint8_t x);
void OS_ResetTimerApp(void);
void OS_ResetCalcApp(void);
void OS_ShowSelectedLine(void);
uint8_t OS_ButtonScrollVariable(uint8_t *var, uint8_t min, uint8_t max);
void OS_WakeUp(void);

#endif /* __OS_H */