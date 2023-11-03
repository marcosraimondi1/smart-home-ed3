/*
===============================================================================
 Name        :
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include "lpc17xx_gpdma.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_exti.h"
#include "lpc17xx_spi.h"

//---------------------- GLOBAL VARIABLES---------------------------------------------------

uint32_t resistanceVal= 200;



void singleListDMA(uint32_t *srcAddr, uint32_t *dstAddr, uint32_t dmaSize, uint32_t control, uint32_t transferType, uint32_t srcConn, uint32_t dstConn, uint32_t channelNum);
void timerMATConfig( uint8_t prOption, uint32_t prValue, uint8_t mChannel, uint8_t inter, uint8_t stop, uint8_t reset, uint8_t extOutput, uint32_t value);
void timerCAPConfig(uint8_t prOption, uint32_t prValue, uint8_t capChannel, uint8_t risEdge, uint8_t falEdge, uint8_t inter);
void pinConfig(uint8_t port, uint8_t pin, uint8_t func, uint8_t mode);
void configADC();
void configPin();
void configExtInt();
void configTimer();
int main()
{
	configPin();
	configExtInt();
	configTimer();
	configADC();

	while(1){}
	return 0;
}

/* ------------------------------------------
 * --                                      --
 * --           CONFIG FUNCTIONS           --
 * --                                      --
 * ------------------------------------------
 */

void configADC(void){
	ADC_Init(LPC_ADC, 200000);
	ADC_EdgeStartConfig(LPC_ADC, ADC_START_ON_RISING);
	ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01); //timer0 mat1
	ADC_IntConfig(LPC_ADC,ADC_ADINTEN0,SET);
	ADC_ChannelCmd(LPC_ADC,0,ENABLE);
	NVIC_EnableIRQ(ADC_IRQn);
}

void configTimer()
{
	// ADC para LDR
	timerMATConfig(TIM_PRESCALE_USVAL,1000,1,DISABLE,DISABLE,ENABLE,TIM_EXTMATCH_TOGGLE, 2500); //match para 15segs+== prende adc en 30
}

void configExtInt()
{
	EXTI_InitTypeDef puerta;
	puerta.EXTI_Line = EXTI_EINT0;
	puerta.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE;
	puerta.EXTI_polarity = EXTI_POLARITY_HIGH_ACTIVE_OR_RISING_EDGE;
	EXTI_Init();
	EXTI_Config(&puerta);
	EXTI_ClearEXTIFlag(0);
	NVIC_EnableIRQ(EINT0_IRQn);
}

void configPin()
{
	// semaforo, P2.2 gpio output
	pinConfig(2, 2, 0, 0);
	GPIO_SetDir(2, 1<<2, 1);
	GPIO_ClearValue(2, (1<<2));
	//
	pinConfig(0,23, 1,0);
	// luz nocturna, P2.3 gpio output
	pinConfig(2, 3, 0 , 0);
	GPIO_SetDir(2, 1<<3, 1);
	GPIO_ClearValue(2, 1<<3);

	// sensor puerta, P2.10 exti
	pinConfig(2,10,1,PINSEL_PINMODE_PULLUP);
}

/* ------------------------------------------
 * --                                      --
 * --           HANDLERS                   --
 * --                                      --
 * ------------------------------------------
 */

void EINT0_IRQHandler()
{
	uint32_t value = GPIO_ReadValue(2) & 1<<2;

	if (value)
		GPIO_ClearValue(2, 1<<2);
	else
		GPIO_SetValue(2, 1<<2);

	for(int i=0;i<999999; i++){}
	EXTI_ClearEXTIFlag(0);
}

void ADC_IRQHandler(){
	uint32_t value=0;
	if (ADC_ChannelGetStatus(LPC_ADC, 0, 1))
	{
		value = ADC_ChannelGetData(LPC_ADC, 0);
		if(value>resistanceVal){
			GPIO_ClearValue(2, 1<<3);
		}else{
			GPIO_SetValue(2, 1<<3);
		}
	}
}


/**
 * @brief configures and enables the timer for match
 *
 * @param prOption
 * @param prValue
 * @param mChannel
 * @param inter
 * @param stop
 * @param reset
 * @param extOutput
 * @param value
 */
void timerMATConfig( uint8_t prOption, uint32_t prValue, uint8_t mChannel, uint8_t inter, uint8_t stop, uint8_t reset, uint8_t extOutput, uint32_t value){
    TIM_TIMERCFG_Type timerConfig;
    TIM_MATCHCFG_Type matchConfig;
    timerConfig.PrescaleOption = prOption;
    timerConfig.PrescaleValue = prValue;
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerConfig);
    matchConfig.MatchChannel = mChannel;
    matchConfig.IntOnMatch = inter;
    matchConfig.StopOnMatch = stop;
    matchConfig.ResetOnMatch = reset;
    matchConfig.ExtMatchOutputType = extOutput;
    matchConfig.MatchValue = value;
    TIM_ConfigMatch(LPC_TIM0, &matchConfig);
    TIM_Cmd(LPC_TIM0, ENABLE);
}


/**
 * @brief Configures pinsel register's pin
 *
 * @param port
 * @param pin
 * @param func
 * @param mode
 */

void pinConfig(uint8_t port, uint8_t pin, uint8_t func, uint8_t mode){
    PINSEL_CFG_Type pinConfig;
    pinConfig.Portnum = port;
    pinConfig.Pinnum = pin;
    pinConfig.Funcnum = func;
    pinConfig.Pinmode = mode;
    pinConfig.OpenDrain = 0;
    PINSEL_ConfigPin(&pinConfig);
}
