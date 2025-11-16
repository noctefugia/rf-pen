#ifndef __BUTTON_H
#define __BUTTON_H

#include "utils.h"

#define BUTTON_RELOAD_TIME TICKS8(50) //100ms
#define BUTTON_LONG_CLICK_TIME TICKS8(300) //500ms

typedef enum {
	BTN_UP,
	BTN_DOWN,
	BTN_COUNT, //add new buttons above this line
	BTN_INVALID = 0xFF
} BTN_ID_TypeDef;

struct button_id_struct {
	bool state;
	uint8_t reload_time, hold_time;
	struct io_struct io;
};

struct button_struct {
	struct button_id_struct id[BTN_COUNT];
	void (*click_event)(uint8_t, bool);
	bool enabled;
};

void Button_Init(void (*click_evt)(uint8_t, bool));
void Button_Update(void);
void Button_Enable(bool state);
uint8_t Button_IsDown(uint8_t index);

#endif /* __BUTTON_H */