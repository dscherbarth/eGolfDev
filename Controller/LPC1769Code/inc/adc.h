/****************************************************************************
 *   $Id:: adc.h 6089 2011-01-06 04:38:09Z nxp12832                         $
 *   Project: NXP LPC17xx ADC example
 *
 *   Description:
 *     This file contains ADC code header definition.
 *
 ****************************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
****************************************************************************/
#ifndef __ADC_H 
#define __ADC_H

// adc allocations
//	0 accelerator				J6-15
//	1 tune						J6-16
//	2 supply current			J6-17
//	3 adc reflector	(not ai)	J6-18
//	4 cooling loop temp			J6-19
//	5 bus voltage				J6-20
//	6 IGBT 1 current (shown as P0[3] / RXD0 / AD0[6])	J6-22		//DWB
//	7 IGBT 2 current (shown as P0[2] / TXD0 / AD0[7])	J6-21

/* In DMA mode, BURST mode and ADC_INTERRUPT flag need to be set. */
/* In BURST mode, ADC_INTERRUPT need to be set. */
#define ADC_INTERRUPT_FLAG	1	/* 1 is interrupt driven, 0 is polling */
#define BURST_MODE			0   /* Burst mode works in interrupt driven mode only. */
#define ADC_DEBUG			1

#define ADC_OFFSET          0x10
#define ADC_INDEX           4

#define ADC_DONE            0x80000000
#define ADC_OVERRUN         0x40000000
#define ADC_ADINT           0x00010000

#define PHASE0CURRENT	6
#define PHASE1CURRENT	7

#define ADC_NUM			8		/* for LPCxxxx */

#define ACCELINDEX		0
#define TUNEINDEX		1
#define AMPSINDEX		2
#define TEMPINDEX		4
#define VOLTSINDEX		5
#define PHASEAINDEX		6
#define PHASECINDEX		7

//#define ADC_CLK			1000000		/* set to 1Mhz */
//#define ADC_CLK			12430750		/* set to 13Mhz (should be max speed) */
#define ADC_CLK			13000000		/* set to 13Mhz (should be max speed) */

extern void ADC_IRQHandler( void );
extern void ADCInit( uint32_t ADC_Clk );
extern void ADCRead( uint8_t channelNum );
extern void ADCBurstRead( void );
void DACReflect (uint32_t dr);
uint32_t getADC (int index);
void setADCValue (uint32_t val, int index);
#endif /* end __ADC_H */


/*****************************************************************************
**                            End Of File
******************************************************************************/
