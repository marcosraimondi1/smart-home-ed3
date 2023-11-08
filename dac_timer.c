#include "LPC17xx.h"
#include "LPC17xx_dac.h"
#include "LPC17xx_timer.h"
#include "LPC17xx_pinsel.h"
#include "music.h"

void confDAC();
void confTimer();
void confPin();

int main(void) {

	confPin();
	confDAC();
	confTimer();
    while(1) {
    }

    return 0 ;
}

void confPin(){
	PINSEL_CFG_Type confpin;

	confpin.Portnum = 0;
	confpin.Pinnum = 26;
	confpin.Funcnum = 2;
	confpin.Pinmode = PINSEL_PINMODE_TRISTATE;
	confpin.OpenDrain = PINSEL_PINMODE_NORMAL;

	PINSEL_ConfigPin(&confpin);

	return;

}
void confDAC(){

	DAC_CONVERTER_CFG_Type confDac1;
	confDac1.DBLBUF_ENA = 0;
	confDac1.CNT_ENA = 0;
	confDac1.DMA_ENA = 0;

	DAC_ConfigDAConverterControl(LPC_DAC,&confDac1);
	DAC_Init(LPC_DAC);

	return;
}

void confTimer(){

	TIM_TIMERCFG_Type confTimer;
	TIM_MATCHCFG_Type confTimerMatch;

	confTimer.PrescaleOption = TIM_PRESCALE_TICKVAL;
	confTimer.PrescaleValue = 6;

	confTimerMatch.MatchChannel = 0;
	confTimerMatch.IntOnMatch = ENABLE;
	confTimerMatch.StopOnMatch = DISABLE;
	confTimerMatch.ResetOnMatch = ENABLE;
	confTimerMatch.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	confTimerMatch.MatchValue = 94;

	TIM_Init(LPC_TIM0,TIM_TIMER_MODE,&confTimer);
	TIM_ConfigMatch(LPC_TIM0,&confTimerMatch);
	TIM_Cmd(LPC_TIM0,ENABLE);
	NVIC_EnableIRQ(TIMER0_IRQn);

	return;
}

void TIMER0_IRQHandler(){
	static uint32_t Contador = 0;
	if(TIM_GetIntStatus(LPC_TIM0,TIM_MR0_INT)){
		DAC_UpdateValue(LPC_DAC,(Mario8bits8k_raw[Contador]>>2));
		//DAC_UpdateValue(LPC_DAC,1023);
		Contador++;
		if(Contador == Mario8bits8k_raw_size){
			Contador = 0;
		}
		TIM_ClearIntPending(LPC_TIM0,TIM_MR0_INT);
	}
}
