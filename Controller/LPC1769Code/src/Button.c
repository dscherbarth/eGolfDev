/****************************************************************************
 *   Description:
 *     This file contains Button handling code
 *
 ****************************************************************************
****************************************************************************/
#include "lpc17xx.h"
#include "type.h"
#include "button.h"
#include "test.h"
#include "command.h"
#include "status.h"
#include "lcd.h"
#include "timer.h"
#include "fault.h"
#include "waveform.h"

uint32_t	buttonStates[4] = {0};
uint32_t	buttonBits[4] = {8, 10, 11, 12};
void buttonInit (void)
{
	LPC_GPIO2->FIODIR &= ~(1 << BUTTON1);	// set to inputs
	LPC_GPIO2->FIODIR &= ~(1 << BUTTON2);
	LPC_GPIO2->FIODIR &= ~(1 << BUTTON3);
	LPC_GPIO2->FIODIR &= ~(1 << BUTTON4);

}

int buttonIsPressed (int buttonNumber)
{
	// gpio bit clear indicates button is pressed
	return (!(LPC_GPIO2->FIOPIN & (1 << buttonBits[buttonNumber])) );
}

int buttonstateChanged (int buttonNumber)
{
	int retval;


	// gpio bit clear indicates button is pressed
	retval = buttonStates[buttonNumber] != (LPC_GPIO2->FIOPIN & (1 << buttonBits[buttonNumber]));

	buttonStates[buttonNumber] = (LPC_GPIO2->FIOPIN & (1 << buttonBits[buttonNumber])) ;
	return retval;
}

extern uint32_t mode;
static float jog = 0;

static uint32_t torque = 0;
float getJog(void)
{
	return jog;
}

void setJog(float sjog)
{
	jog = sjog;
}

uint32_t getTorque(void)
{
	return torque;
}

void setTorque(uint32_t storque)
{
	torque = storque;
}

void waitForB3 (char *msg)
{
	int v1, v2;

	while (1)
	{
		v1 = buttonIsPressed (3); v2 = buttonstateChanged (3);
		if (v1 && v2)
		{
			break;
		}
		delayMicros(0, 1000, NULL );		// .001Sec delay
	}
}

/*****************************************************************************
** Function name:		handle buttons
**
** Descriptions:		at the 1/10 second update, handle mkiii buttons
**
** parameters:			none
** Returned value:		nothing
**
*****************************************************************************/
void handleButtons( void )
{
	int v1, v2;
#ifdef a
	phase_t	phdata;
#endif

#ifdef buttons
	// step / jog speed up
	v1 = buttonIsPressed (0); v2 = buttonstateChanged (0);
	if ( v1 && v2 )
	{
		if (getRunState() == RUNNING)
		{
			jog += 68.49;	// add 100 rpm
			if (jog > 4000)
			{
				jog = 4000;		// this is actually a 6000 rpm limit
			}
		}
		else
		{
			stepPage();
		}
	}

	// select /jog speed down
	v1 = buttonIsPressed (1); v2 = buttonstateChanged (1);
	if ( v1 && v2 )
	{
		if (getRunState() == RUNNING)
		{
			jog -= 68.49;	// subtrack 100 rpm
			if (jog < 0)
			{
				jog = 0;
			}
		}
	}

	// home
	v1 = buttonIsPressed (2); v2 = buttonstateChanged (2);
	if ( v1 && v2 )
	{
		lcd_clear();
		SetPage (PAGE_SELECT);

		// fallback reset of the i2c comm to the lcd
		delayMs(0, 10);
		lcd_init_cmd();
	}

	// start/stop/clear abort/ run test
	v1 = buttonIsPressed (3); v2 = buttonstateChanged (3);
	if ( v1 && v2 )
	{
		if (!faulted() || getRunState() == FAULTED)
		{
			if (getRunState() == STOPPED)	// execute the phase test (make sure we are stopped first)
			{
				// we need some confidence that cableing is correct
				// set to 10% phase A then wait till button 3 is pressed (this is A low)
				set_mcpwm(90, 810, 0);
				waitForB3("A10 B90  ");

				// set to 90% phase A then wait till button 3 is pressed (this is A high)
				set_mcpwm(90, 0, 810);
				waitForB3("A10 C90  ");

				// set to 10% phase B then wait till button 3 is pressed (this is B low)
				set_mcpwm(0, 90, 810);
				waitForB3("B10 C90  ");

				set_mcpwm(0, 0, 0);	// off
			}
			else if (getRunState() == STOPPED)
			{
				start();
				jog = 0;
			}
			else if (getRunState() == RUNNING || getRunState() == FAULTED)
			{
				// stop sequence
				stop();

				// reset the fault condition (might re-fault right away...)
				fault_reset();
			}
		}
		else
		{
			// reset fault condition and restart the snapshot
			fault_reset();
//			startSnap (0, 0, 0);
		}
	}
#endif
}
