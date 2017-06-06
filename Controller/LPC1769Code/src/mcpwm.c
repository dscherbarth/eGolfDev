/****************************************************************************
 *   $Id:: mcpwm.c 5747 2010-11-30 23:45:33Z usb00423                       $
 *   Project: NXP LPC17xx Motor Control PWM example
 *
 *   Description:
 *     This file contains MCPWM code example which include MCPWM 
 *     initialization, MCPWM interrupt handler, and APIs for MCPWM access.
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
#include "LPC17xx.h"
#include "type.h"
#include "mcpwm.h"
#include "adc.h"
#include "gpio.h"
#include "ssp.h"

extern uint32_t	schan;

/******************************************************************************
** Function name:		MCPWM_IRQHandler
**
** Descriptions:		MCPWM interrupt handler
**
** parameters:			None
** Returned value:		None
** 
******************************************************************************/
void MCPWM_IRQHandler(void)
{
	static	uint32_t	slowcounter = 0;

	volatile uint32_t regVal;
	regVal = LPC_MCPWM->INTEN;
	regVal = LPC_MCPWM->INTF;

	if (regVal & 0x01)		// a result of lim0 counter expiring.  This should go off at the pwm rate (currently 10kHz)
	{
		//start the ai read
//		device_on(FAN);		// timing
		SSPIntReadACI ();
		if (++slowcounter >= 50)
		{
		  // reset counter and choose next
		  slowcounter = 0;

		  // start next slow read
		  ADCRead( schan );
		}
	}

	LPC_MCPWM->INTF_CLR = regVal;
	return;
}

/******************************************************************************
** Function name:		MCPWM_EnableLimInt
**
** Descriptions:		MCPWM start limit based interrupts.
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void MCPWM_EnableLimInt(void)
{
	// make sure the counter is not zero,
	NVIC_EnableIRQ(MCPWM_IRQn);
	LPC_MCPWM->INTEN_CLR = 0xffff;			// disable all interrupts
	LPC_MCPWM->INTEN_SET = 1;			// enable interrupts for lim0

}

/******************************************************************************
** Function name:		MCPWM_DisableLimInt
**
** Descriptions:		MCPWM stop limit based interrupts.
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void MCPWM_DisableLimInt(void)
{
	LPC_MCPWM->INTEN_CLR = 0xffff;			// disable all interrupts
	NVIC_DisableIRQ(MCPWM_IRQn);

}

/******************************************************************************
** Function name:		MCPWM_Init
**
** Descriptions:		MCPWM initialization.
**
** parameters:			None
** Returned value:		None
** 
******************************************************************************/
void MCPWM_Init(void)
{
  LPC_SC->PCONP |= 0x00020000;		   /* Turn On MCPWM PCLK */

  /* P1.19~1.26, P1.28~29 as Motor Control PWM pins. (includes mcabort config)*/
  LPC_PINCON->PINSEL3 &= ~(0xFFFF<<6);
  LPC_PINCON->PINSEL3 |= (0x5555<<6);

  LPC_PINCON->PINSEL3 &= ~(0x0F<<24);
  LPC_PINCON->PINSEL3 |= (0x05<<24);

  return;
}

/******************************************************************************
** Function name:		MCPWM_Config
**
** Descriptions:		MCPWM configuration
**
** parameters:			ChannelNum, setup structure
** Returned value:		true or fase.
** 
******************************************************************************/
uint32_t MCPWM_Config(uint32_t channelNum, MCPWM_CHANNEL_SETUP_T * channelSetup)
{
  if (channelNum == 0)
  {
    LPC_MCPWM->TC0 = channelSetup->channelTimercounterValue;
    LPC_MCPWM->LIM0 = channelSetup->channelPeriodValue;
    LPC_MCPWM->MAT0 = channelSetup->channelPulsewidthValue;

	if (channelSetup->channelType)
	{
	  LPC_MCPWM->CON_SET = 1 << 1;
	}
	else
	{
	  LPC_MCPWM->CON_CLR = 1 << 1;
	}

	if (channelSetup->channelPolarity)
	{
	  LPC_MCPWM->CON_SET = 1 << 2;
	}
	else
	{
	  LPC_MCPWM->CON_CLR = 1 << 2;
	}

	if (channelSetup->channelDeadtimeEnable)
	{
	  LPC_MCPWM->CON_SET = 1 << 3;
	}
	else
	{
	  LPC_MCPWM->CON_CLR = 1 << 3;
	}
	LPC_MCPWM->DT &= ~0x3FF;
	LPC_MCPWM->DT |= channelSetup->channelDeadtimeValue & 0x3FF; 

	if (channelSetup->channelUpdateEnable)
	{
	  LPC_MCPWM->CON_CLR = 1 << 4;
	}
	else
	{
	  LPC_MCPWM->CON_SET = 1 << 4;
	}
  }
  else if (channelNum == 1)
  {
    LPC_MCPWM->TC1 = channelSetup->channelTimercounterValue;
    LPC_MCPWM->LIM1 = channelSetup->channelPeriodValue;
    LPC_MCPWM->MAT1 = channelSetup->channelPulsewidthValue;

	if (channelSetup->channelType)
	{
	  LPC_MCPWM->CON_SET = 1 << 9;
	}
	else
	{
	  LPC_MCPWM->CON_CLR = 1 << 9;
	}

	if (channelSetup->channelPolarity)
	{
	  LPC_MCPWM->CON_SET = 1 << 10;
	}
	else
	{
	  LPC_MCPWM->CON_CLR = 1 << 10;
	}

	if (channelSetup->channelDeadtimeEnable)
	{
	  LPC_MCPWM->CON_SET = 1 << 11;
	}
	else
	{
	  LPC_MCPWM->CON_CLR = 1 << 11;
	}
	LPC_MCPWM->DT &= ~(0x3FF << 10);
	LPC_MCPWM->DT |= (channelSetup->channelDeadtimeValue & 0x3FF) << 10; 

	if (channelSetup->channelUpdateEnable)
	{
	  LPC_MCPWM->CON_CLR = 1 << 12;
	}
	else
	{
	  LPC_MCPWM->CON_SET = 1 << 12;
	}
  }
  else if (channelNum == 2)
  {
    LPC_MCPWM->TC2 = channelSetup->channelTimercounterValue;
    LPC_MCPWM->LIM2 = channelSetup->channelPeriodValue;
    LPC_MCPWM->MAT2 = channelSetup->channelPulsewidthValue;

	if (channelSetup->channelType)
	{
	  LPC_MCPWM->CON_SET = 1 << 17;
	}
	else
	{
	  LPC_MCPWM->CON_CLR = 1 << 17;
	}

	if (channelSetup->channelPolarity)
	{
	  LPC_MCPWM->CON_SET = 1 << 18;
	}
	else
	{
	  LPC_MCPWM->CON_CLR = 1 << 18;
	}

	if (channelSetup->channelDeadtimeEnable)
	{
	  LPC_MCPWM->CON_SET = 1 << 19;
	}
	else
	{
	  LPC_MCPWM->CON_CLR = 1 << 19;
	}
	LPC_MCPWM->DT &= ~(0x3FF << 20);
	LPC_MCPWM->DT |= (channelSetup->channelDeadtimeValue & 0x3FF) << 20; 

	if (channelSetup->channelUpdateEnable)
	{
	  LPC_MCPWM->CON_CLR = 1 << 20;
	}
	else
	{
	  LPC_MCPWM->CON_SET = 1 << 20;
	}
  }
  else
  {
	return ( FALSE );		/* Unknown channel number */
  }
	
  return (TRUE);
}

/******************************************************************************
** Function name:		MCPWM_WriteToShadow
**
** Descriptions:		Write to MCPWM shadow registers.
**
** parameters:			ChannelNum, setup structure
** Returned value:		true or fase.
** 
******************************************************************************/
uint32_t MCPWM_WriteToShadow(uint32_t channelNum, MCPWM_CHANNEL_SETUP_T * channelSetup)
{
  if (channelNum == 0)
  {
    LPC_MCPWM->LIM0 = channelSetup->channelPeriodValue;
    LPC_MCPWM->MAT0 = channelSetup->channelPulsewidthValue;
  }
  else if (channelNum == 1)
  {
    LPC_MCPWM->LIM1 = channelSetup->channelPeriodValue;
    LPC_MCPWM->MAT1 = channelSetup->channelPulsewidthValue;
  }
  else if (channelNum == 2)
  {
    LPC_MCPWM->LIM2 = channelSetup->channelPeriodValue;
    LPC_MCPWM->MAT2 = channelSetup->channelPulsewidthValue;
  }
  else
  {
	return ( FALSE );		/* Unknown channel number */
  }
	
  return (TRUE);
}

/******************************************************************************
** Function name:		EMPWM_Start
**
** Descriptions:		Start MCPWM channels
**
** parameters:			channel number
** Returned value:		None
** 
******************************************************************************/
void MCPWM_Start(uint32_t channel0, uint32_t channel1, uint32_t channel2)
{
  uint32_t regVal = 0;
  
  if (channel0 == 1)
  {
    regVal |= 1;
  }
  if (channel1 == 1)
  {
    regVal |= (1 << 8);
  }
  if (channel2 == 1)
  {
    regVal |= (1 << 16);
  }

  LPC_MCPWM->CON_SET = regVal;
  return;
}

/******************************************************************************
** Function name:		MCPWM_Stop
**
** Descriptions:		Stop MCPWM channels
**
** parameters:			channel number
** Returned value:		None
** 
******************************************************************************/
void MCPWM_Stop(uint32_t channel0, uint32_t channel1, uint32_t channel2)
{
  uint32_t regVal = 0;

  if (channel0 == 1)
  {
    regVal |= 1;
  }
  if (channel1 == 1)
  {
    regVal |= (1 << 8);
  }
  if (channel2 == 1)
  {
    regVal |= (1 << 16);
  }

  LPC_MCPWM->CON_CLR = regVal;
  return;
}

/******************************************************************************
** Function name:		MCPWM_acMode
**
** Descriptions:		MCPWM ac Mode enable/disable
**
** parameters:			acMode
** Returned value:		None
** 
******************************************************************************/
void MCPWM_acMode(uint32_t acMode)
{
  if (acMode)
  {
    LPC_MCPWM->CON_SET = 0x40000000;
  }
  else
  {
    LPC_MCPWM->CON_CLR = 0x40000000;
  }

  return;
}

/******************************************************************************
** Function name:		Map
**
** Descriptions:		maps value between limits
**
** parameters:
** Returned value:
**
******************************************************************************/

inline uint32_t map(uint32_t x, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max)
{
  return (((x - in_min) * (out_max - out_min)) / (in_max - in_min)) + out_min;
}


/******************************************************************************
**                            End Of File
******************************************************************************/
