#include "LPC17xx.h"
#include "LPC17xx_dac.h"
#include "LPC17xx_timer.h"
#include "LPC17xx_pinsel.h"
#include "lpc17xx_gpdma.h"
#include "music.h"

void confDAC();
void confTimer();
void confPin();
void confDMA();

int n_lists = Mario8bits8k_raw_size / 4095; // 4095 max transfer size por LLI
GPDMA_LLI_Type *linked_list;
GPDMA_Channel_CFG_Type dma_config;

int main(void) {
	GPDMA_LLI_Type linked_list_array[n_lists];
	linked_list = linked_list_array;
	confPin();
	confDAC();
	confDMA();
	//confTimer();
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
	confDac1.DBLBUF_ENA = 1;
	confDac1.CNT_ENA 	= 1;
	confDac1.DMA_ENA 	= 1;

	DAC_ConfigDAConverterControl(LPC_DAC,&confDac1);
	DAC_SetDMATimeOut(LPC_DAC, 570);
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

void confDMA()
{
	GPDMA_LLI_Type lli0;
	lli0.SrcAddr = (uint32_t)Mario8bits8k_raw;
	lli0.DstAddr = (uint32_t)&(LPC_DAC->DACR);
	lli0.NextLLI = 0;
	lli0.Control = 4095					// transfer size
							 | 1<<19	// source width = word 32 bits
							 | 1<<22	// dest width = word 32 bits
							 | 1<<26;	// incremento automatico


	for (int i=0; i<n_lists; i++)
	{
		linked_list[i].Control = lli0.Control;
		linked_list[i].DstAddr = lli0.DstAddr;
		linked_list[i].SrcAddr = lli0.SrcAddr + i*4095;
		linked_list[i].NextLLI = i == (n_lists - 1) ?
				(uint32_t)&linked_list[0] : (uint32_t)&linked_list[i+1];
	}


	dma_config.ChannelNum = 0;
	dma_config.TransferSize = 4095;
	dma_config.TransferWidth = 0; // no se usa
	dma_config.TransferType = GPDMA_TRANSFERTYPE_M2P;
	dma_config.SrcConn = 0;
	dma_config.DstConn = GPDMA_CONN_DAC;
	dma_config.SrcMemAddr = (uint32_t)Mario8bits8k_raw;
	dma_config.DstMemAddr = 0;
	dma_config.DMALLI = (uint32_t) linked_list;

	GPDMA_Init();

	GPDMA_Setup(&dma_config);
	GPDMA_ChannelCmd(0, ENABLE);
}

void TIMER0_IRQHandler(){
	static uint32_t Contador = 0;
	if(TIM_GetIntStatus(LPC_TIM0,TIM_MR0_INT)){
		DAC_UpdateValue(LPC_DAC,(Mario8bits8k_raw[Contador]>>6));
		//DAC_UpdateValue(LPC_DAC,1023);
		Contador++;
		if(Contador == Mario8bits8k_raw_size){
			Contador = 0;
		}
		TIM_ClearIntPending(LPC_TIM0,TIM_MR0_INT);
	}
}
