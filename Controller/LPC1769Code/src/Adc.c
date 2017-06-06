/****************************************************************************
 *   $Id:: adc.c 6089 2011-01-06 04:38:09Z nxp12832                         $
 *   Project: NXP LPC17xx ADC example
 *
 *   Description:
 *     This file contains ADC code example which include ADC 
 *     initialization, ADC interrupt handler, and APIs for ADC
 *     reading.
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
#include "lpc17xx.h"
#include "type.h"
#include "adc.h"
#include "gpio.h"
#include "mc_foc.h"

#define NUMCHAN	8
#define SAMPLES	3

static volatile uint32_t ADCValue[ADC_NUM];

static volatile uint32_t rawadc [NUMCHAN][SAMPLES];		// space for 3 values for each of 3 channels
static volatile uint32_t readchan = 0;	// which one are we working on
static volatile uint32_t adcin = 0;

//
// public functions
//
uint32_t getADC (int index)
{
	// just an accessor
	return ADCValue[index];
}

void setADCValue (uint32_t val, int index)
{
//	ADCValue[index] = (ADCValue[index]  + val) >> 1;	// 2 element average
//	ADCValue[index] = (ADCValue[index] + ADCValue[index] + ADCValue[index]  + val) >> 2;	// 4 element average
	ADCValue[index] = val;
}

/*****************************************************************************
** Function name:		ADCInit
**
** Descriptions:		initialize ADC channel
**
** parameters:			ADC clock rate
** Returned value:		None
**
*****************************************************************************/
void ADCInit( uint32_t ADC_Clk )
{
  uint32_t i, pclkdiv, pclk;

  /* Enable CLOCK into ADC controller */
  LPC_SC->PCONP |= (1 << 12);

  for ( i = 0; i < ADC_NUM; i++ )
  {
	ADCValue[i] = 0x0;
  }

  /* all the related pins are set to ADC inputs, AD0.0~7 */
  LPC_PINCON->PINSEL0 &= ~0x000000F0;	/* P0.2~3, A0.6~7, function 10 */
  LPC_PINCON->PINSEL0 |= 0x000000A0;
  LPC_PINCON->PINSEL1 &= ~0x001FC000;	/* P0.23~26, A0.0~2, function 01  A3 set to DAC*/
  LPC_PINCON->PINSEL1 |= 0x00254000;
  LPC_PINCON->PINSEL3 |= 0xF0000000;	/* P1.30~31, A0.4~5, function 11 */
  /* No pull-up no pull-down (function 10) on these ADC pins. */
  LPC_PINCON->PINMODE0 &= ~0x000000F0;
  LPC_PINCON->PINMODE0 |= 0x000000A0;
  LPC_PINCON->PINMODE1 &= ~0x003FC000;
  LPC_PINCON->PINMODE1 |= 0x002A8000;
  LPC_PINCON->PINMODE3 &= ~0xF0000000;
  LPC_PINCON->PINMODE3 |= 0xA0000000;

  /* By default, the PCLKSELx value is zero, thus, the PCLK for
  all the peripherals is 1/4 of the SystemFrequency. */
  /* Bit 24~25 is for ADC */
  pclkdiv = (LPC_SC->PCLKSEL0 >> 24) & 0x03;
  switch ( pclkdiv )
  {
	case 0x00:
	default:
	  pclk = SystemFrequency/4;
	break;
	case 0x01:
	  pclk = SystemFrequency;
	break;
	case 0x02:
	  pclk = SystemFrequency/2;
	break;
	case 0x03:
	  pclk = SystemFrequency/8;
	break;
  }

  LPC_ADC->CR = ( 0x01 << 0 ) |  /* SEL=1,select channel 0~7 on ADC0 */
		( ( pclk  / ADC_Clk - 1 ) << 8 ) |  /* CLKDIV = Fpclk / ADC_Clk - 1 */
		( 0 << 16 ) | 		/* BURST = 0, no BURST, software controlled */
		( 0 << 17 ) |  		/* CLKS = 0, 11 clocks/10 bits */
		( 1 << 21 ) |  		/* PDN = 1, normal operation */
		( 0 << 24 ) |  		/* START = 0 A/D conversion stops */
		( 0 << 27 );		/* EDGE = 0 (CAP/MAT singal falling,trigger A/D conversion) */

  NVIC_EnableIRQ(ADC_IRQn);
  LPC_ADC->INTEN = 0x1FF;		/* Enable all interrupts */

  // init dac
  LPC_DAC->CNTVAL = 0x00FF;
  LPC_DAC->CTRL = (0x1<<1)|(0x1<<2);

  return;
}

void DACReflect (uint32_t dr)
{
	LPC_DAC->CR = (dr << 6);
}

/*****************************************************************************
** Function name:		ADCRead
**
** Descriptions:		Read ADC channel
**
** parameters:			Channel number
** Returned value:		Value read, if interrupt driven, return channel #
**
*****************************************************************************/
inline void ADCRead( uint8_t channelNum )
{
	static uint8_t toggleChan = 6;


	readchan = channelNum;

  // toggle between 6 and 7 for phase current reads
  if (channelNum == 9)
  {
	  readchan = channelNum = toggleChan;
	  if (toggleChan == 6) toggleChan = 7; else toggleChan = 6;
  }

  /* channel number is 0 through 7 */
  if ( channelNum >= ADC_NUM )
  {
	channelNum = 0;		/* reset channel number to 0 */
  }

  LPC_ADC->CR &= 0xFFFFFF00;
  LPC_ADC->CR |= (1 << 24) | (1 << channelNum);
}

//
// internal private functions
//
static uint32_t median (uint32_t chan)
{
	int i;
	uint32_t val = 0;
	uint32_t high = 0;
	uint32_t low = 4095;
	uint32_t highi, lowi;


	// all the same
	if (rawadc[chan][0] == rawadc[chan][1] && rawadc[chan][0] == rawadc[chan][2])
	{
		return rawadc[chan][0];
	}

	// find high and low to locate median (only works for SAMPLES = 3)
	for (i=0; i<SAMPLES; i++)
	{
		if (rawadc[chan][i] > high)
		{
			high = rawadc[chan][i];
			highi = i;
		}
		if (rawadc[chan][i] < low)
		{
			low = rawadc[chan][i];
			lowi = i;
		}
	}
	for (i=0; i<SAMPLES; i++)
	{
		if (i != lowi && i != highi)
		{
			val = rawadc[chan][i];
			break;
		}
	}
	return val;
}
uint32_t	schan = 0;

/******************************************************************************
** Function name:		ADC_IRQHandler
**
** Descriptions:		ADC interrupt handler
**
** parameters:			None
** Returned value:		None
** 
******************************************************************************/
void ADC_IRQHandler (void)
{
  static	uint32_t	sread = 0;

  // if we are not done with the 'read' just save the value and start the next read
//  adcin = 1;
  LPC_ADC->CR &= ~(0x7<<24);	/* stop ADC now */

  rawadc[readchan][sread] = (LPC_ADC->DR[readchan] >> 4) & 0xfff;
  if (sread >= 2)		// read a total of 3 sets, now we can find the median values
  {
	  ADCValue[readchan] = median (readchan);
	  schan++;
	  if (schan == 3) schan++;		// skip dac
	  if (schan >= 6)
	  {
		  schan = 0;	// roll over
		  sread++;
	  }
	  if (readchan == 5)
	  {
		  sread = 0;
	  }
  }
  else
  {
	  schan++;
	  if (schan == 3) schan++;		// skip dac
	  if (schan >= 6)
	  {
		  schan = 0;	// roll over
		  sread++;
	  }

  }
}


/*********************************************************************************
**                            End Of File
*********************************************************************************/
