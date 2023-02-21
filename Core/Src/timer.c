#include "stm32l476xx.h"
#include "timer.h"

// initializes the timer (Similar to USART2_Init(). Place in timer.c)
void TIM_Init(void) {

	uint32_t tmpreg;
	(void) tmpreg;

	/* Timer Enable and Prescale */
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
	tmpreg = RCC->APB1ENR1 & RCC_APB1ENR1_TIM2EN;
	TIM2->PSC |= 15; 	// PSC set to 16 to divide frequency down to a period of 1us
	TIM2->EGR |= TIM_EGR_UG;

	/* Timer Capture Configuration */
	TIM2->CCER &= ~(TIM_CCER_CC1E);
	TIM2->CCMR1 |= TIM_CCMR1_CC1S_0;
	TIM2->CCMR1 &= ~(TIM_CCMR1_IC1F);
	TIM2->CCER |= TIM_CCER_CC1E;

	/* Update on the rising edge */
	TIM2->CCER &= ~TIM_CCER_CC1P;
	TIM2->CCER &= ~TIM_CCER_CC1NP;

	// Setup GPIO for Alternate Function
	GPIOA->MODER &= ~GPIO_MODER_MODE0_Msk;
	GPIOA->MODER |= GPIO_MODER_MODE0_1;
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL0;
	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL0_0;

	return;
}
