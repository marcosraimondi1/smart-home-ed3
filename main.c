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
#include "sirena1.h"
#include "sirena2.h"

#define PASSWORD_LEN 4
#define DAC_TIMEOUT 1200

//---------------------- GLOBAL VARIABLES ---------------------------------------------------

// UART
uint8_t buffer_input[1] = {0};
uint8_t buffer_output[20] = {};

uint8_t password[PASSWORD_LEN] = {1, 2, 3, 4}; // password actual

enum OPS
{
	LIGHTS_ON = 0xA1,
	LIGHTS_OFF = 0xA2,
	CHANGE_PASSWORD = 0xA3,
	START_PASSWORD = 0xA4,
	END_PASSWORD = 0xFF,
	SEND_TIME = 0xA5,
	VELOCIDAD_ALARMA = 0xA6
};

// ADC
uint32_t resistanceVal = 150;

// DMA
uint32_t alarma_addr;
GPDMA_LLI_Type linked_list_array[24];
GPDMA_Channel_CFG_Type dma_config;

// FLAGS
uint8_t flags_uart[4] = {0}; // 1: change password, 2: start password, 3: velocidad alarma, 4: send time
uint8_t flag_alarma_activada = 0;
uint8_t flag_antirrebote = 0;

//---------------------- PROTOTYPES ---------------------------------------------------

/* CONFIG */
void configDAC(uint32_t timeout);
void configDMA();
void configADC();
void configPin();
void configExtInt();
void configTimer();
void configUART();
void timerMATConfig(LPC_TIM_TypeDef *timer, uint8_t prOption, uint32_t prValue, uint8_t mChannel, uint8_t inter, uint8_t stop, uint8_t reset, uint8_t extOutput, uint32_t value);
void pinConfig(uint8_t port, uint8_t pin, uint8_t func, uint8_t mode);

/* SYSTEM */
uint8_t check_password(uint8_t password[], uint8_t input_array[]);
void parse_uart_input();
void toggleAlarma();

// sonido
void sonar_alarma();
void mutear_alarma();

void enableInterrupts();
//------------------------------------------------------------------------

int main()
{
	alarma_addr = (uint32_t)alarma1; // alarma2
	configPin();
	configExtInt();
	configTimer();
	configADC();
	configUART();
	configDAC(DAC_TIMEOUT);
	configDMA();

	enableInterrupts();

	while (1)
	{
	}
	return 0;
}

/* ------------------------------------------
 * --                                      --
 * --             HANDLERS                 --
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
	if (flag_alarma_activada && !flag_antirrebote)
	{
		// sonar alarma
		sonar_alarma();

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
	static uint16_t vals_1[5] = {0};

	if (ADC_ChannelGetStatus(LPC_ADC, 0, 1))
	{
		value = ADC_ChannelGetData(LPC_ADC, 0);

		if (value > resistanceVal)
		{
			GPIO_ClearValue(2, 1 << 3);
		}
		else
		{
			GPIO_SetValue(2, 1 << 3);
		}
	}
	else if (ADC_ChannelGetStatus(LPC_ADC, 1, 1)) // muestras de la temperatura
	{
		value = ADC_ChannelGetData(LPC_ADC, 1);
		vals_1[count] = value;
		if (count == 4)
		{
			count = 0;
			// calcular promedio y mandar por uart
			uint32_t prom_1 = 0;
			for (int i = 0; i < 5; i++)
			{
				prom_1 += vals_1[i];
			}
			prom_1 /= 5;
			// ver si mandamos por uart las muestras de temp
			// buffer_output[0] = prom;
			// UART_Send(LPC_UART2, buffer_output, sizeof(char), NONE_BLOCKING);
		}
	}
}

void UART2_IRQHandler()
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
		// UART_Send(LPC_UART2, buffer_input, sizeof(char), NONE_BLOCKING);

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
	if (flag_alarma_activada)
		GPIO_ClearValue(2, 1 << 2);
	else
		GPIO_SetValue(2, 1 << 2);

	flag_alarma_activada = ~flag_alarma_activada & 1;
}

uint8_t check_password(uint8_t password[], uint8_t input_array[])
{
	// Compara los arreglos
	for (int i = 0; i < PASSWORD_LEN; i++)
		if (password[i] != input_array[i])
			return 0;
	return 1;
}

void parse_uart_input()
{
	// verifico el estado de los flags
	static uint8_t password_input[PASSWORD_LEN] = {0}; // entrada de usuario del password

	static int pass_index = 0;
	static uint8_t password_vieja_correcta_flag = 0;

	// verificar fin de trama de password
	if (buffer_input[0] == END_PASSWORD)
	{
		if (flags_uart[0])
		{
			// cambiar password
			if (password_vieja_correcta_flag)
				for (int i = 0; i < PASSWORD_LEN; i++)
					password[i] = password_input[i];

			flags_uart[0] = 0;
		}

		if (flags_uart[1])
		{
			static int errores_consecutivos = 0;

			// verificar password
			if (check_password(password, password_input))
			{
				toggleAlarma(); // LUCES
				errores_consecutivos = 0;
				mutear_alarma();
			}
			else
			{
				// error
				errores_consecutivos++;
				if (errores_consecutivos > 3)
				{
					// sonar alarma
					sonar_alarma();
				}
			}
			flags_uart[1] = 0;
		}

		pass_index = 0;
		return;
	}

	if (flags_uart[0])
	{
		// cambiar pass
		password_input[pass_index] = buffer_input[0];

		pass_index = (pass_index + 1) % PASSWORD_LEN;

		// tiene que ingresar primero el password actual y despues el nuevo
		if (!password_vieja_correcta_flag && check_password(password_input, password))
		{
			// empiezo a cargar la nueva contrasena
			pass_index = 0;
			password_vieja_correcta_flag = 1;
		}

		return;
	}

	if (flags_uart[1])
	{
		// start password
		password_input[pass_index] = buffer_input[0];
		pass_index = (pass_index + 1) % PASSWORD_LEN;
		return;
	}

	if (flags_uart[2])
	{
		// velocidad alarma DAC
		configDAC(DAC_TIMEOUT - buffer_input[0] * 100);
		flags_uart[2] = 0;
		return;
	}

	if (flags_uart[3])
	{
		// send time de ADC
		TIM_UpdateMatchValue(LPC_TIM0, 1, buffer_input[0] * 1000);
		flags_uart[3] = 0;
		return;
	}

	// si no estoy haciendo una operacion verifico que operacion inicia
	switch (buffer_input[0])
	{
	case LIGHTS_ON:
		// prender luz
		GPIO_SetValue(0, 1 << 16);
		break;
	case LIGHTS_OFF:
		// apagar luz
		GPIO_ClearValue(0, 1 << 16);

		break;
	case CHANGE_PASSWORD:
		// cambiar password
		flags_uart[0] = 1;
		break;
	case START_PASSWORD:
		// start password
		flags_uart[1] = 1;
		break;
	case VELOCIDAD_ALARMA:
		// velocidad alarma DAC
		flags_uart[2] = 1;
		break;
	case SEND_TIME:
		// send time de ADC
		flags_uart[3] = 1;
		break;
	default:
		// comando no reconocido
		break;
	}
}

void sonar_alarma()
{
	GPDMA_ChannelCmd(0, ENABLE); // suena la alarma
}

void mutear_alarma()
{
	GPDMA_ChannelCmd(0, DISABLE); // se apaga la alarma
}

/* ------------------------------------------
 * --                                      --
 * --           CONFIG FUNCTIONS           --
 * --                                      --
 * ------------------------------------------
 */

void enableInterrupts()
{
	NVIC_SetPriority(ADC_IRQn, 30);
	NVIC_SetPriority(EINT0_IRQn, 20);
	NVIC_SetPriority(UART2_IRQn, 10);
	NVIC_SetPriority(UART2_IRQn, 25);

	NVIC_EnableIRQ(ADC_IRQn);
	NVIC_EnableIRQ(EINT0_IRQn);
	NVIC_EnableIRQ(UART2_IRQn);
	NVIC_EnableIRQ(TIMER1_IRQn);
}

void configADC()
{
	ADC_Init(LPC_ADC, 200000);
	ADC_EdgeStartConfig(LPC_ADC, ADC_START_ON_RISING);
	ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01); // timer0 mat1
	ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, SET);
	ADC_ChannelCmd(LPC_ADC, 0, ENABLE);
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

	// LUZ INTERIOR, UART, P0.16
	pinConfig(0, 16, 0, 0);
	GPIO_SetDir(0, 1 << 16, 1);
	GPIO_ClearValue(0, (1 << 16));

	// DAC, P0.26
	pinConfig(0, 26, 2, 0);
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

	return;
}

void configDAC(uint32_t timeout)
{

	DAC_CONVERTER_CFG_Type confDac1;
	confDac1.DBLBUF_ENA = 0;
	confDac1.CNT_ENA = 1;
	confDac1.DMA_ENA = 1;

	DAC_ConfigDAConverterControl(LPC_DAC, &confDac1);
	DAC_SetDMATimeOut(LPC_DAC, timeout);
	DAC_Init(LPC_DAC);

	return;
}

void configDMA()
{

	for (int i = 0; i < 24; i++)
	{
		linked_list_array[i].Control = 4095 | (1 << 18) // source width 16 bit
									   | 1 << 22		// dest width = word 32 bits
									   | 1 << 26;		// incremento automatico
		linked_list_array[i].DstAddr = (uint32_t) & (LPC_DAC->DACR);
		linked_list_array[i].SrcAddr = (uint32_t)(alarma1 + i * 4095);
		linked_list_array[i].NextLLI = i == (24 - 1) ? (uint32_t)&linked_list_array[0] : (uint32_t)&linked_list_array[i + 1];
	}

	dma_config.ChannelNum = 0;
	dma_config.TransferSize = 4095;
	dma_config.TransferWidth = 0; // no se usa
	dma_config.TransferType = GPDMA_TRANSFERTYPE_M2P;
	dma_config.SrcConn = 0;
	dma_config.DstConn = GPDMA_CONN_DAC;
	dma_config.SrcMemAddr = (uint32_t)alarma1;
	dma_config.DstMemAddr = 0;
	dma_config.DMALLI = (uint32_t)&linked_list_array[0];

	GPDMA_Init();

	GPDMA_Setup(&dma_config);

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
