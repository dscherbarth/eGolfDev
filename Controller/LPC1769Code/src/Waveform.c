/****************************************************************************
 *   $Id:: Waveform.c                          $
 *   Project: NXP LPC17xx pwm acim waveform
 *
 *   Description:
****************************************************************************/
#include "lpc17xx.h"
#include "type.h"
#include "timer.h"
#include "mcpwm.h"
#include "leds.h"
#include "status.h"
#include "sensors.h"
#include "waveform.h"
#include "gpio.h"


#define DTV	25			// DeadTimeValue = 1.1 usec (PClk = 18 MHz, period = .055 usec x 25 = 1.37 usec
//#define DTV	20			// DeadTimeValue = 1.1 usec (PClk = 18 MHz, period = .055 usec x 20 = 1.1 usec
//#define DTV	2			// DeadTimeValue = 1.1 usec (PClk = 18 MHz, period = .055 usec x 2 = .11 usec

//#define PERVAL 900			// 18MHz / 900 / 2 = 10.000 kHz, 100.5 usec period
#define PERVAL 1125		// 18MHz / 1125 / 2 = 8.0KHz 125 usec / period

MCPWM_CHANNEL_SETUP_T channelsetup[3];

uint32_t cv32 = 0;
static uint32_t phase1 = 1;
static uint32_t	phase2 = 2;

static	uint32_t current_direction = FORWARD;

void set_wave_direction(uint32_t dir)
{

	if (dir != current_direction)
	{
		// responsibility is on the caller that we are at zero rpm... at speed this might be bad...
		if (dir == FORWARD)
		{
			phase1 = 1; phase2 = 2;
		}
		else
		{
			phase1 = 2; phase2 = 1;
		}
		current_direction = dir;
	}
}

uint32_t get_wave_direction(void)
{
	return current_direction;
}

/******************************************************************************
** Function name:		pwm_init
**
** Descriptions:		init the ac pwm channels
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void pwm_init(void)
{
	  MCPWM_Init();

	  channelsetup[0].channelType = 1;
	  channelsetup[0].channelPolarity = 0;
	  channelsetup[0].channelDeadtimeEnable = 1;
	  channelsetup[0].channelDeadtimeValue = DTV;
	  channelsetup[0].channelUpdateEnable = 1;
	  channelsetup[0].channelTimercounterValue = 0;
	  channelsetup[0].channelPeriodValue = PERVAL;
	  channelsetup[0].channelPulsewidthValue = 0;

	  channelsetup[1].channelType = 1;
	  channelsetup[1].channelPolarity = 0;
	  channelsetup[1].channelDeadtimeEnable = 1;
	  channelsetup[1].channelDeadtimeValue = DTV;
	  channelsetup[1].channelUpdateEnable = 1;
	  channelsetup[1].channelTimercounterValue = 0;
	  channelsetup[1].channelPulsewidthValue = 0;

	  channelsetup[2].channelType = 1;
	  channelsetup[2].channelPolarity = 0;
	  channelsetup[2].channelDeadtimeEnable = 1;
	  channelsetup[2].channelDeadtimeValue = DTV;
	  channelsetup[2].channelUpdateEnable = 1;
	  channelsetup[2].channelTimercounterValue = 0;
	  channelsetup[2].channelPulsewidthValue = 0;

	  MCPWM_Config(0, &channelsetup[0]);
	  MCPWM_Config(1, &channelsetup[1]);
	  MCPWM_Config(2, &channelsetup[2]);

	  MCPWM_acMode(1);

	  LPC_GPIO3->FIODIR = 0x02000000;
	  LPC_GPIO3->FIOSET = 0x02000000;
	  delayMs(0, 20);
	  LPC_GPIO3->FIOCLR = 0x02000000;

	  // disable mcpwm interrupts
	  LPC_MCPWM->INTEN_CLR = 0x00008777;

	  MCPWM_Start(1,1,1);
}

uint16_t aa, bb, cc;
void RetPWM (uint16_t * a, uint16_t * b, uint16_t * c )
{
	*a = aa;
	*b = bb;
	*c = cc;
}

void set_mcpwm(int a, int b, int c)
{

	// CAREFULL!!!!
	aa = a; bb = b; cc = c;
	channelsetup[0].channelPulsewidthValue = a;
	channelsetup[1].channelPulsewidthValue = b;
	channelsetup[2].channelPulsewidthValue = c;

	MCPWM_WriteToShadow(0, &channelsetup[0]);
	MCPWM_WriteToShadow(1, &channelsetup[1]);
	MCPWM_WriteToShadow(2, &channelsetup[2]);

}

