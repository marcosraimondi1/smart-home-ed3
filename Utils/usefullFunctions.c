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
#include "lpc17xx_spi.h"

void singleListDMA(uint32_t *srcAddr, uint32_t *dstAddr, uint32_t dmaSize, uint32_t control, uint32_t transferType, uint32_t srcConn, uint32_t dstConn, uint32_t channelNum);
void timerMATConfig( uint8_t prOption, uint32_t prValue, uint8_t mChannel, uint8_t inter, uint8_t stop, uint8_t reset, uint8_t extOutput, uint32_t value);
void timerCAPConfig(uint8_t prOption, uint32_t prValue, uint8_t capChannel, uint8_t risEdge, uint8_t falEdge, uint8_t inter);
void pinConfig(uint8_t port, uint8_t pin, uint8_t func, uint8_t mode);




/**
 * @brief   Single List DMA channel configuration
 *          Note: This function doesn't enable the channel.
 * 
 * @param   srcAddr: Source address
 * @param   dstAddr: Destination address
 * @param   dmaSize: DMA size
 * @param   control: Control register
 * @param   transferType: Transfer type
 * @param   srcConn: Source connection
 * @param   dstConn: Destination connection
 * @param   channelNum: Channel number
 * 
 * @return  None
 *  
*/
void singleListDMA(uint32_t *srcAddr, uint32_t *dstAddr, uint32_t dmaSize, uint32_t control, uint32_t transferType, uint32_t srcConn, uint32_t dstConn, uint32_t channelNum){
    GPDMA_LLI_Type LLI1;
    GPDA_Chanel_CFG_Type channelConfig;
/*LLI config*/ 
    LLI1.SrcAddr = (uint32_t) srcAddr;
    LLI1.DstAddr = (uint32_t) dstAddr;
    LLI1.NextLLI = (uint32_t) &LLI1;
    LLI1.Control = control;
    GPDMA_Init();
/*Channel configuration*/
    channelConfig.ChannelNum = channelNum;
    channelConfig.SrcConn = srcConn;
    channelConfig.DstConn = dstConn;
    channelConfig.TransferSize = dmaSize;
    channelConfig.TransferWidth = 0;
    channelConfig.TransferType = transferType;
    channelConfig.SrcMemAddr = (uint32_t) srcAddr;
    channelConfig.DstMemAddr = (uint32_t) dstAddr;
    channelConfig.DMALLI = (uint32_t) &LLI1;
    GPDMA_Setup(&channelConfig);

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
 * @brief configures and enables the timer for capture
 * 
 * @param prOption 
 * @param prValue 
 * @param capChannel 
 * @param risEdge 
 * @param falEdge 
 * @param inter 
 */

void timerCAPConfig(uint8_t prOption, uint32_t prValue, uint8_t capChannel, uint8_t risEdge, uint8_t falEdge, uint8_t inter){
    TIM_TIMERCFG_Type timerConfig;
    TIM_CAPTURECFG_Type captureConfig;
    timerConfig.PrescaleOption = prOption;
    timerConfig.PrescaleValue = prValue;
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerConfig);
    captureConfig.CaptureChannel = capChannel;
    captureConfig.RisingEdge = risEdge;
    captureConfig.FallingEdge = falEdge;
    captureConfig.IntOnCaption = inter;
    TIM_ConfigCapture(LPC_TIM0, &captureConfig);
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




/*----------------- PRUEBA ----------------*/

void configSPI(){
    SPI_CFG_Type cfg;
    cfg.CPHA = SPI_CPHA_SECOND;
    cfg.DataBit = SPI_DATABIT_8;
    cfg.CPOL = SPI_CPOL_LO;
    cfg.Mode = SPI_MASTER_MODE;//ver
    cfg.DataOrder = SPI_DATA_MSB_FIRST;
    cfg.ClockRate = 1000000; ///ver
    SPI_Init(LPC_SPI, &cfg);
    SPI_TRANSFER_Type transferType = SPI_TRANSFER_POLLING; //para q tengamos q preguntar si hay datos

    
}