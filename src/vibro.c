#include "vibro.h"

static struct vibro_struct vibro;
static const uint8_t TONE_SRC[VT_COUNT][VIBRO_TONE_MAX_SZ*2] = {
	{75,TICKS8(100), 0,TICKS8(150), 75,TICKS8(100), 0,TICKS8(150), 100,TICKS8(250)},	//VT_ALERT
	{100,TICKS8(300), 0,TICKS8(300), 100,TICKS8(150), 0,TICKS8(0), 0,TICKS8(0)},	//VT_INIT	
	{100,TICKS8(2000), 0,TICKS8(1000), 100,TICKS8(2000), 0,TICKS8(1000), 100,TICKS8(2000)},	//VT_TIME_IS_UP
};

void Vibro_Init(void)
{
	struct io_struct cur_io;
	AssignIO(&cur_io, PORT_VIBRO_MOT, PIN_VIBRO_MOT, TRUE, GPIO_MODE_OUT_PP_LOW_FAST);
	
	vibro.pulse_timer = 0;
	vibro.tone_pos = 0;
	vibro.tone_id = VT_NONE;
	
	/*TIM2_DeInit();
	TIM2_TimeBaseInit(TIM2_PRESCALER_8, (uint16_t)VIBRO_PWM_DIV);*/
  TIM2->PSCR = (uint8_t)(TIM2_PRESCALER_8);
  /* Set the Autoreload value */
  TIM2->ARRH = (uint8_t)(VIBRO_PWM_DIV >> 8);
  TIM2->ARRL = (uint8_t)(VIBRO_PWM_DIV);
	
	Vibro_Off();
	vibro.enabled = TRUE;
}


void Vibro_OC1Init(TIM2_OutputState_TypeDef TIM2_OutputState, uint16_t TIM2_Pulse)
{
  /* Disable the Channel 1: Reset the CCE Bit, Set the Output State , the Output Polarity */
  TIM2->CCER1 &= (uint8_t)(~( TIM2_CCER1_CC1E | TIM2_CCER1_CC1P));
  /* Set the Output State &  Set the Output Polarity  */
  TIM2->CCER1 |= (uint8_t)((uint8_t)(TIM2_OutputState & TIM2_CCER1_CC1E ) | 
                           (uint8_t)(TIM2_OCPOLARITY_HIGH & TIM2_CCER1_CC1P));
  
  /* Reset the Output Compare Bits  & Set the Ouput Compare Mode */
  TIM2->CCMR1 = (uint8_t)((uint8_t)(TIM2->CCMR1 & (uint8_t)(~TIM2_CCMR_OCM)) |
                          (uint8_t)TIM2_OCMODE_PWM1);
  
  /* Set the Pulse value */
  TIM2->CCR1H = (uint8_t)(TIM2_Pulse >> 8);
  TIM2->CCR1L = (uint8_t)(TIM2_Pulse);
}


void Vibro_Off(void)
{
	vibro.pulse_timer = 0;
	/*TIM2_OC1Init(TIM2_OCMODE_PWM1, TIM2_OUTPUTSTATE_DISABLE, (uint16_t)0, TIM2_OCPOLARITY_HIGH);
	TIM2_Cmd(DISABLE);*/
	Vibro_OC1Init(TIM2_OUTPUTSTATE_DISABLE, 0);
	TIM2->CR1 &= (uint8_t)(~TIM2_CR1_CEN);
}


void Vibro_Pulse(uint8_t power, uint8_t ticks)
{
	uint16_t pulse_power;
	
	if (!vibro.enabled)
		return;
		
	vibro.pulse_timer = ticks;
	pulse_power = (uint16_t)(((uint32_t)VIBRO_PWM_DIV * (uint32_t)power) / 100);
	/*TIM2_OC1Init(TIM2_OCMODE_PWM1, TIM2_OUTPUTSTATE_ENABLE, pulse_power, TIM2_OCPOLARITY_HIGH);
	TIM2_Cmd(ENABLE);*/
	Vibro_OC1Init(TIM2_OUTPUTSTATE_ENABLE, pulse_power);
	TIM2->CR1 |= (uint8_t)TIM2_CR1_CEN;
}


void Vibro_Update(void)
{
	if (!vibro.enabled)
		return;
		
	if (vibro.pulse_timer) {
		if (--vibro.pulse_timer == 0) {
			Vibro_Off();
			if (vibro.tone_id != VT_NONE) {
				vibro.tone_pos += 2;
				if ( (vibro.tone_pos >= VIBRO_TONE_MAX_SZ*2) \
				|| (TONE_SRC[vibro.tone_id][vibro.tone_pos+1] == 0) )
					vibro.tone_id = VT_NONE;
				else
					Vibro_Pulse( \
						TONE_SRC[vibro.tone_id][vibro.tone_pos], \
						TONE_SRC[vibro.tone_id][vibro.tone_pos+1] \
					);
			}
		}
	}
}


void Vibro_Tone(VIBRO_TONE_TypeDef id)
{
	assert_param(IS_VIBRO_TONE_OK(id));
	
	Vibro_Off();
	vibro.tone_id = id;
	vibro.tone_pos = 0;
	Vibro_Pulse(TONE_SRC[vibro.tone_id][0], TONE_SRC[vibro.tone_id][1]);
}


void Vibro_Enable(bool state)
{
	vibro.enabled = state;
	Vibro_Off();
}