#include "button.h" 

static struct button_struct btn;


void Button_Init(void (*click_evt)(uint8_t, bool))
{
	btn.click_event = click_evt;
	
	AssignIO(&btn.id[BTN_UP].io, PORT_BTN_UP, PIN_BTN_UP, TRUE, GPIO_MODE_IN_PU_NO_IT);
	AssignIO(&btn.id[BTN_DOWN].io, PORT_BTN_DOWN, PIN_BTN_DOWN, TRUE, GPIO_MODE_IN_PU_NO_IT);
	
	Button_Enable(TRUE);
}


void Button_Update(void)
{
	uint8_t i;
	bool long_click;
		
	if (!btn.enabled)
		return;
		
	for (i = 0; i < BTN_COUNT; ++i) {
		ReadIO(&btn.id[i].io, &btn.id[i].state);
		if (!btn.id[i].state) {
			if (btn.id[i].reload_time > 0)
				--btn.id[i].reload_time;
			else
				++btn.id[i].hold_time;
		} else if (btn.id[i].hold_time > 0) {
			long_click = (btn.id[i].hold_time >= BUTTON_LONG_CLICK_TIME) ? TRUE : FALSE;
			(*btn.click_event)(i, long_click);
			btn.id[i].hold_time = 0;
			btn.id[i].reload_time = BUTTON_RELOAD_TIME;
		}
	}
}


uint8_t Button_IsDown(uint8_t index)
{
	return ( (!btn.id[index].state) ? btn.id[index].hold_time : 0 );
}


void Button_Enable(bool state)
{
	uint8_t i;
	
	for (i = 0; i < BTN_COUNT; ++i) {
		btn.id[i].state = FALSE;
		btn.id[i].reload_time = 0;
		btn.id[i].hold_time = 0;
	}
	btn.enabled = state;
}