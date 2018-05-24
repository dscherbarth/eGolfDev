/****************************************************************************
 *   $Id:: ssp.c 5804 2010-12-04 00:32:12Z usb00423                         $
 *   Project: NXP LPC17xx SSP example
 *
 *   Description:
 *     This file contains SSP code example which include SSP initialization, 
 *     SSP interrupt handler, and APIs for SSP access.
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
#include "LPC17xx.h"			/* LPC17xx Peripheral Registers */
#include "ssp.h"
#include "gpio.h"
#include "adc.h"
#include "mc_foc.h"

// channel read state
uint32_t	rb0 = 0, rb1 = 0;
uint32_t	ra0 = 0, ra1 = 0;
uint32_t	busIVal = 0;
uint32_t	acc0Val = 0, acc1Val = 0;

/*****************************************************************************
** Function name:		SSP_IRQHandler
**
** Descriptions:		SSP port is used for SPI communication.
**						SSP interrupt handler
**						The algorithm is, if RXFIFO is at least half full, 
**						start receive until it's empty; if TXFIFO is at least
**						half empty, start transmit until it's full.
**						This will maximize the use of both FIFOs and performance.
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void SSP0_IRQHandler(void) 
{
  uint32_t regValue;


  // data for the adc device is in one 32bit word
  // make sure the data is ready and read the data and place in the appropriate location
  regValue = (LPC_SSP0->DR & 0xfff);
  if (rb0)
  {
	  busIVal = ((regValue + busIVal + busIVal + busIVal)/4);
	  rb0 = 0;
  }
  else  if (ra0)
  {
	  acc0Val = ((regValue + acc0Val + acc0Val + acc0Val)/4);
	  ra0 = 0;
  }
  else
  {
	  setADCValue (regValue, 6);
  }

  // shut off the chip select
  SSP_SSELToggle( 1, 0);

  // clear the interrupt condition
  regValue = LPC_SSP0->MIS;
  LPC_SSP0->ICR = regValue;
  return;
}

uint32_t getaccVal (void)
{
	return ((acc0Val + acc1Val) /2);
}
uint32_t getBusIVal (void)
{
	return busIVal;
}

extern uint32_t iCounter;

void SSP1_IRQHandler(void)
{
  uint32_t regValue;


  // data for the adc device is in one 32bit word
  // make sure the data is ready and read the data and place in the appropriate location
  regValue = (LPC_SSP1->DR & 0xfff);
  if (rb1)
  {
	  busIVal = ((regValue + busIVal + busIVal + busIVal)/4);
	  rb1 = 0;
  }
  else  if (ra1)
  {
	  acc1Val = ((regValue + acc1Val + acc1Val + acc1Val)/4);
	  ra1 = 0;
  }
  else
  {
	  setADCValue (regValue, 7);
  }

  // shut off the chip select
  SSP_SSELToggle( 1, 1);

  // clear the interrupt condition
  regValue = LPC_SSP1->MIS;
  LPC_SSP1->ICR = regValue;
//  device_off(FAN);		// timing 19.6uS from conversion completion to here
  iCounter--;
  vMC_FOC_Loop();
  return;
}

/*****************************************************************************
** Function name:		SSP0_SSELToggle
**
** Descriptions:		SSP0 CS manual set
**				
** parameters:			port num, toggle(1 is high, 0 is low)
** Returned value:		None
** 
*****************************************************************************/
void SSP_SSELToggle( uint32_t toggle, uint32_t sspdev )
{
	if (sspdev == 0)
	{
		if ( !toggle )
			LPC_GPIO0->FIOCLR |= (0x1<<16);
		else
			LPC_GPIO0->FIOSET |= (0x1<<16);
	}
	else
	{
	    if ( !toggle )
		  LPC_GPIO0->FIOCLR |= (0x1<<6);
	    else
		  LPC_GPIO0->FIOSET |= (0x1<<6);
	}
  return;
}

/*****************************************************************************
** Function name:		SSPInit
**
** Descriptions:		SSP port initialization routine
**				
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void SSP0Init( void )
{
  uint8_t i, Dummy=Dummy;

  /* Enable AHB clock to the SSP0. */
  LPC_SC->PCONP |= (0x1<<21);

//  /* Further divider is needed on SSP0 clock. Using default divided by 4 */
//  LPC_SC->PCLKSEL1 &= ~(0x3<<10);

  /* Further divider is needed on SSP0 clock. Using default divided by 2 */
  LPC_SC->PCLKSEL1 &= ~(0x3<<10);
  LPC_SC->PCLKSEL1 |= (0x2<<10);

  /* P0.15~0.18 as SSP0 */
  LPC_PINCON->PINSEL0 &= ~(0x3UL<<30);
  LPC_PINCON->PINSEL0 |= (0x2UL<<30);
  LPC_PINCON->PINSEL1 &= ~((0x3<<0)|(0x3<<2)|(0x3<<4));
  LPC_PINCON->PINSEL1 |= ((0x2<<0)|(0x2<<2)|(0x2<<4));
  
  LPC_PINCON->PINSEL1 &= ~(0x3<<0);
  LPC_GPIO0->FIODIR |= (0x1<<16);		/* P0.16 defined as GPIO and Outputs */
  LPC_GPIO0->FIODIR |= (0x1<<6);		/* SSP0 P0.6 defined as Outputs */
		
  /* Set DSS data to 14-bit, Frame format SPI, CPOL = 0, CPHA = 0, and SCR is 0 */
  LPC_SSP0->CR0 = 0x000d;

  /* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
  LPC_SSP0->CPSR = 14;		// should make 2MHz (expirementally 2.5MHz works fine (14))

  for ( i = 0; i < FIFOSIZE; i++ )
  {
	Dummy = LPC_SSP0->DR;		/* clear the RxFIFO */
  }

  /* Enable the SSP Interrupt */
  NVIC_EnableIRQ(SSP0_IRQn);
	
  LPC_SSP0->CR1 = SSPCR1_SSE;

  /* Set SSPINMS registers to enable interrupts */
  /* enable all error related interrupts */
  LPC_SSP0->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM | SSPIMSC_RXIM;
  return;
}

/*****************************************************************************
** Function name:		SSPInit
**
** Descriptions:		SSP port initialization routine
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
void SSP1Init( void )
{
  uint8_t i, Dummy=Dummy;

  /* Enable AHB clock to the SSP0. */
  LPC_SC->PCONP |= (0x1<<10);

//  /* Further divider is needed on SSP1 clock. Using default divided by 4 */
//  LPC_SC->PCLKSEL0 &= ~(0x3<<20);

  /* Further divider is needed on SSP0 clock. Using default divided by 2 */
  LPC_SC->PCLKSEL0 &= ~(0x3<<20);
  LPC_SC->PCLKSEL0 |= (0x2<<20);

  /* P0.6~0.9 as SSP1 */
  LPC_PINCON->PINSEL0 &= ~((0x3<<12)|(0x3<<14)|(0x3<<16)|(0x3<<18));
  LPC_PINCON->PINSEL0 |= ((0x2<<12)|(0x2<<14)|(0x2<<16)|(0x2<<18));

  LPC_PINCON->PINSEL0 &= ~(0x3<<12);
  LPC_GPIO0->FIODIR |= (0x1<<6);		/* P0.6 defined as GPIO and Outputs */

  /* Set DSS data to 14-bit, Frame format SPI, CPOL = 0, CPHA = 0, and SCR is 0 */
  LPC_SSP1->CR0 = 0x000d;

  /* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
//  LPC_SSP1->CPSR = 0x6;		// with clk/4 makes 3 MHz
  LPC_SSP1->CPSR = 14;			// should make 2MHz (expirementally 2.5MHz works fine (14))

  for ( i = 0; i < FIFOSIZE; i++ )
  {
	Dummy = LPC_SSP1->DR;		/* clear the RxFIFO */
  }

  /* Enable the SSP Interrupt */
  NVIC_EnableIRQ(SSP1_IRQn);

  /* Device select as master, SSP Enabled */
  /* Master mode */
  LPC_SSP1->CR1 = SSPCR1_SSE;

  /* Set SSPINMS registers to enable interrupts */
  /* enable all error related interrupts */
  LPC_SSP1->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM | SSPIMSC_RXIM;
  return;
}

static int	toggleBCAC = 0;

#define BUSRDEL	100		// read bus current every x phase current reads
// cause a read of phase A and phase C current values
void SSPIntReadACI ()
{
	volatile uint32_t	Dummy;		// compiler complains, but the read on this variable is required..
	static int	readBusCandAcc = 0;
	int wait;


	// enable the ssp devices
	SSP_SSELToggle( 0 , 0);
	SSP_SSELToggle( 0 , 1);

	// trigger the channel 0 on both chips
	if (readBusCandAcc++ > BUSRDEL)
	{
		if (toggleBCAC)
		{
			ra0 = ra1 = 1;
			wait = 100000; while ( wait-- && (LPC_SSP0->SR & (SSPSR_TNF|SSPSR_BSY)) != SSPSR_TNF );
			LPC_SSP0->DR = 0x1b;	// single ended channel 3
			wait = 100000; while ( wait-- && (LPC_SSP1->SR & (SSPSR_TNF|SSPSR_BSY)) != SSPSR_TNF );
			LPC_SSP1->DR = 0x1b;	// single ended channel 3
			toggleBCAC = 0;
		}
		else
		{
			rb0 = rb1 = 1;
			wait = 100000; while ( wait-- && (LPC_SSP0->SR & (SSPSR_TNF|SSPSR_BSY)) != SSPSR_TNF );
			LPC_SSP0->DR = 0x19;	// single ended channel 1
			wait = 100000; while ( wait-- && (LPC_SSP1->SR & (SSPSR_TNF|SSPSR_BSY)) != SSPSR_TNF );
			LPC_SSP1->DR = 0x19;	// single ended channel 1
			toggleBCAC = 1;
		}
		readBusCandAcc = 0;
	}
	else
	{
		wait = 100000; while ( wait-- && (LPC_SSP0->SR & (SSPSR_TNF|SSPSR_BSY)) != SSPSR_TNF );
		LPC_SSP0->DR = 0x18;	// single ended channel 0
		wait = 100000; while ( wait-- && (LPC_SSP1->SR & (SSPSR_TNF|SSPSR_BSY)) != SSPSR_TNF );
		LPC_SSP1->DR = 0x18;	// single ended channel 0
	}

	// wait for the conversion takes 6.9uS
	wait = 100000; while ( wait-- && (LPC_SSP0->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE );
	LPC_SSP0->DR = 0xFF;
	Dummy = LPC_SSP0->DR;
	wait = 100000; while ( wait-- && (LPC_SSP1->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE );
	LPC_SSP1->DR = 0xFF;
	Dummy = LPC_SSP1->DR;
	wait = Dummy;		// to appease compiler :(

//	device_off(FAN);		// timing
//	device_on(FAN);		// timing

	// interrupt will read the value, place it in the adc array and de-select the cs
}

/******************************************************************************
**                            End Of File
******************************************************************************/

