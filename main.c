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
#include "lpc17xx_uart.h"

//---------------------- GLOBAL VARIABLES ---------------------------------------------------
uint8_t buffer_input[20] = {};
uint8_t buffer_output[20] = {};
uint8_t password = 234;
uint8_t password_input[1] = {0};
uint32_t resistanceVal = 200;
uint8_t flag_alarma_on = 0;
uint8_t flag_antirrebote = 0;

//---------------------- PROTOTYPES ---------------------------------------------------

void timerMATConfig(LPC_TIM_TypeDef *timer, uint8_t prOption, uint32_t prValue, uint8_t mChannel, uint8_t inter, uint8_t stop, uint8_t reset, uint8_t extOutput, uint32_t value);
void pinConfig(uint8_t port, uint8_t pin, uint8_t func, uint8_t mode);
void configADC();
void configPin();
void configExtInt();
void configTimer();
void configUART();
void parse_uart_input();
void toggleAlarma();
void sound_alarm();

int main()
{
	configPin();
	configExtInt();
	configTimer();
	configADC();
	configUART();

	while (1)
	{
	}
	return 0;
}

/* ------------------------------------------
 * --                                      --
 * --           HANDLERS                   --
 * --                                      --
 * ------------------------------------------
 */
void TIMER1_IRQHandler()
{
	// antirrebote
	flag_antirrebote = 0;
	TIM_ClearIntPending(LPC_TIM1, 0);
}

void EINT0_IRQHandler()
{
	if (flag_alarma_on && !flag_antirrebote)
	{
		// sonar alarma
		sound_alarm();

		// antirrebote
		flag_antirrebote = 1;
		timerMATConfig(LPC_TIM1, TIM_PRESCALE_USVAL, 1000, 0, ENABLE, ENABLE, DISABLE, TIM_EXTMATCH_TOGGLE, 20);
	}
	EXTI_ClearEXTIFlag(0);
}

void ADC_IRQHandler()
{
	uint32_t value = 0;
	static uint8_t count = 0;
	static uint16_t vals[5] = {0};

	if (ADC_ChannelGetStatus(LPC_ADC, 0, 1))
	{
		value = ADC_ChannelGetData(LPC_ADC, 0);

		vals[count] = value;

		if (count == 4)
		{
			count = 0;
			// calcular promedio y mandar por uart
			uint32_t prom = 0;
			for (int i = 0; i < 5; i++)
			{
				prom += vals[i];
			}
			prom /= 5;
			buffer_output[0] = prom;
			UART_Send(LPC_UART2, buffer_output, sizeof(char), NONE_BLOCKING);
		}
		else
		{
			count++;
		}

		if (value > resistanceVal)
		{
			GPIO_ClearValue(2, 1 << 3);
		}
		else
		{
			GPIO_SetValue(2, 1 << 3);
		}
	}
}

void UART2_IRQHandler(void)
{
	uint32_t intsrc, tmp, tmp1;
	
	// Determina la fuente de interrupcion
	intsrc = UART_GetIntId(LPC_UART2);
	tmp = intsrc & UART_IIR_INTID_MASK;
	// Eval�a Line Status
	if (tmp == UART_IIR_INTID_RLS)
	{
		tmp1 = UART_GetLineStatus(LPC_UART2);
		tmp1 &= (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE | UART_LSR_BI | UART_LSR_RXFE);
		// ingresa a un Loop infinito si hay error
		if (tmp1)
		{
			while (1)
			{
			};
		}
	}
	
	// Receive Data Available or Character time-out
	if ((tmp == UART_IIR_INTID_RDA) || (tmp == UART_IIR_INTID_CTI))
	{
		UART_Receive(LPC_UART2, buffer_input, sizeof(char), NONE_BLOCKING);
		UART_Send(LPC_UART2, buffer_input, sizeof(char), NONE_BLOCKING);
		parse_uart_input();
	}
	
	return;
}

/* ------------------------------------------
 * --                                      --
 * --           SYSTEM FUNCTIONS           --
 * --                                      --
 * ------------------------------------------
 */

void toggleAlarma()
{
	if (flag_alarma_on)
		GPIO_ClearValue(2, 1 << 2);
	else
		GPIO_SetValue(2, 1 << 2);

	flag_alarma_on = ~flag_alarma_on & 1;
}

void parse_uart_input()
{
	if (buffer_input[0] == password)
	{
		toggleAlarma();
	}

	// parseo el input de uart
	// uint8_t op = buffer_input[0];
	// if (op == 1)
	// {
	// 	toggleAlarma();
	// }
	// if (op == 2)
	// {
	// 	// cambiar pass
	// }
}

void sound_alarm()
{
	// config DMA
}

/* ------------------------------------------
 * --                                      --
 * --           CONFIG FUNCTIONS           --
 * --                                      --
 * ------------------------------------------
 */

void configADC(void)
{
	ADC_Init(LPC_ADC, 200000);
	ADC_EdgeStartConfig(LPC_ADC, ADC_START_ON_RISING);
	ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01); // timer0 mat1
	ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, SET);
	ADC_ChannelCmd(LPC_ADC, 0, ENABLE);
	NVIC_EnableIRQ(ADC_IRQn);
}

void configTimer()
{
	// ADC para LDR
	timerMATConfig(LPC_TIM0, TIM_PRESCALE_USVAL, 1000, 1, DISABLE, DISABLE, ENABLE, TIM_EXTMATCH_TOGGLE, 500); // match para .5segs+== prende adc en 1

	// Antirrebote, en timer1, se configura en EINT0_IRQHandler
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
	GPIO_SetDir(2, 1 << 2, 1);
	GPIO_ClearValue(2, (1 << 2));

	// sensor de luz, P0.23 adc
	pinConfig(0, 23, 1, 0);

	// luz nocturna, P2.3 gpio output
	pinConfig(2, 3, 0, 0);
	GPIO_SetDir(2, 1 << 3, 1);
	GPIO_ClearValue(2, 1 << 3);

	// sensor puerta, P2.10 exti
	pinConfig(2, 10, 1, PINSEL_PINMODE_PULLUP);

	// UART2, Tx P0.10 y Rx P0.11
	pinConfig(0, 10, 1, 0);
	pinConfig(0, 11, 1, 0);
}

void configUART()
{

	UART_CFG_Type UARTConfigStruct;
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;

	// configuracion por defecto:
	UART_ConfigStructInit(&UARTConfigStruct);

	// inicializa periferico
	UART_Init(LPC_UART2, &UARTConfigStruct);

	// Inicializa FIFO
	UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
	UART_FIFOConfig(LPC_UART2, &UARTFIFOConfigStruct);

	// Habilita interrupcion por el RX del UART
	UART_IntConfig(LPC_UART2, UART_INTCFG_RBR, ENABLE);

	// Habilita interrupcion por el estado de la linea UART
	UART_IntConfig(LPC_UART2, UART_INTCFG_RLS, ENABLE);

	// Habilita el TX
	UART_TxCmd(LPC_UART2, ENABLE);

	NVIC_EnableIRQ(UART2_IRQn);
	return;
}

/* ------------------------------------------
 * --                                      --
 * --           UTILS FUNCTIONS            --
 * --                                      --
 * ------------------------------------------
 */

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
void timerMATConfig(LPC_TIM_TypeDef *timer, uint8_t prOption, uint32_t prValue, uint8_t mChannel, uint8_t inter, uint8_t stop, uint8_t reset, uint8_t extOutput, uint32_t value)
{
	TIM_TIMERCFG_Type timerConfig;
	TIM_MATCHCFG_Type matchConfig;
	timerConfig.PrescaleOption = prOption;
	timerConfig.PrescaleValue = prValue;
	TIM_Init(timer, TIM_TIMER_MODE, &timerConfig);
	matchConfig.MatchChannel = mChannel;
	matchConfig.IntOnMatch = inter;
	matchConfig.StopOnMatch = stop;
	matchConfig.ResetOnMatch = reset;
	matchConfig.ExtMatchOutputType = extOutput;
	matchConfig.MatchValue = value;
	TIM_ConfigMatch(timer, &matchConfig);
	TIM_Cmd(timer, ENABLE);
}

/**
 * @brief Configures pinsel register's pin
 *
 * @param port
 * @param pin
 * @param func
 * @param mode
 */
void pinConfig(uint8_t port, uint8_t pin, uint8_t func, uint8_t mode)
{
	PINSEL_CFG_Type pinConfig;
	pinConfig.Portnum = port;
	pinConfig.Pinnum = pin;
	pinConfig.Funcnum = func;
	pinConfig.Pinmode = mode;
	pinConfig.OpenDrain = 0;
	PINSEL_ConfigPin(&pinConfig);
}

