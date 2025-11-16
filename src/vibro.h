#ifndef __VIBRO_H
#define __VIBRO_H

#include "utils.h" 

#define VIBRO_PWM_FREQ 10000	//10kHz
#define VIBRO_TONE_MAX_SZ 5

#define VIBRO_PWM_DIV ((uint32_t)CORE_FCPU / ((uint32_t)8 * (uint32_t)VIBRO_PWM_FREQ))
	
typedef enum {
	VT_ALERT = 0,
	VT_INIT,
	VT_TIME_IS_UP,
	VT_COUNT, //add new tones above this line
	VT_NONE
} VIBRO_TONE_TypeDef;
#define IS_VIBRO_TONE_OK(id) \
	((id) < (VT_COUNT)) 
	
struct vibro_struct {
	uint8_t pulse_timer, tone_pos, tone_id;
	bool enabled;
};

void Vibro_Init(void);
void Vibro_Pulse(uint8_t power, uint8_t ticks);
void Vibro_Off(void);
void Vibro_Update(void);
void Vibro_Tone(VIBRO_TONE_TypeDef id);
void Vibro_Enable(bool state);
void Vibro_OC1Init(TIM2_OutputState_TypeDef TIM2_OutputState, uint16_t TIM2_Pulse);
#endif /* __VIBRO_H */


