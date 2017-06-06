//
// fault.c
//
//	code to handle igbt fault signal(s)
//
#include "lpc17xx.h"
#include "type.h"
#include "timer.h"
#include "adc.h"
#include "status.h"
#include "snapshot.h"
#include "leds.h"
#include "control.h"
#include "command.h"
#include "power.h"
#include "fault.h"
#include "sensors.h"
#include "gpio.h"
#include "models.h"

static int	faultcond = 0;

//
// faultInit	initialize the interrupt for the IGBT fault line
//
//		Active low, on interrupt make sure the line stays low for at least 1 ms before taking action
//
void faultInit (void)
{
	// setup the external interrupt
	LPC_PINCON->PINSEL4 |= 0x04000000;	/* set P2.13 as EINT3 */	//
	LPC_SC->EXTMODE = EINT3_EDGE;		/* INT3 edge trigger */
	LPC_SC->EXTPOLAR = 0;				/* INT3 set to falling edge */

	// enable the interrupts
	NVIC_EnableIRQ(EINT3_IRQn);
}

/*****************************************************************************
** Function name:		faulted
**
** Descriptions:		returns non-zero if fault is true
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
int faulted (void)
{
	return faultcond;
}

// reason is 1,2,3 refers to A,B,C
void faultReasonLoop (int reason)
{
	faultcond = reason;
}

void fault_reset (void)
{
	faultcond = 0;

	// cycle the fault reset to reset the latch
	device_on (FAULTRESET);
	delayMicros(0, 1, NULL );
	device_off (FAULTRESET);

	// visual indication
	led2_off();

}
void fault_d_reset (void)
{
	// cycle the fault reset to reset the latch
	device_on (FAULTRESET);
	delayMicros(0, 1, NULL );
	device_off (FAULTRESET);
}

void fault_set (void)
{
	// hold the latch off
	device_on (FAULTRESET);

}

void checkFault (int latchfault)
{
	int faultret = 0;

	setStatVal ( SVFAULT,faultret );
	if (latchfault)
		{
		faultcond = faultret;
		}

	if (getRunState() == RUNNING )
	{
		// make sure the precharge/hvsolenoid outputs are in the correct state
		if (device_test (PRECHARGE))
		{
			// wrong state
			device_off(PRECHARGE);
			SetSolPWM (0);
			stop ();

			faultret = FAULT_RELAYPOS;
		}

		// check bus voltage
		if (getBusvolt () > MAXVOLT)
		{
			// overvolt fault
			faultret = FAULT_OVERVOLT;
		}

		// account for observed voltage drop due to bus current
		if ((getBusvolt () + (getBuscurrent () * .12)) < MINVOLT)
		{
			// undervolt fault (battery empty)
			faultret = FAULT_UNDERVOLT;
		}

		// check bus current
		if (getBuscurrent () > MgetBusIFLimit ())
		{
			// overcurrent
			faultret = FAULT_OVERCURR;
		}


		// check temperature
		if (getTemp () > MAXTEMP)
		{
			// overtemp fault
			faultret = FAULT_OVERTEMP;
		}
	}

#define CTEMPTOP 95
#define CTEMPBOT 90	// need hysteresis

	// see if we need cooling
	if (getTemp () > CTEMPTOP)
	{
		device_off(FAN);	// really on
		SetOilPWM (1);				// running
	}
	if (getTemp () < CTEMPBOT)
	{
		device_on(FAN);	// really off
		SetOilPWM (0);				// running
	}

	if (latchfault)
		{
		faultcond = faultret;
		}

	if (faultcond)
	{
		fault();

		// visual indication
		led2_on();
	}

	return;
}

/*****************************************************************************
** Function name:		phaseCurrentsOK
**
** Descriptions:		compare the last few phase current readings in the snapshot buffer against
** 						the selected limit value.  (use the bus current slot and filter the crap out of it)
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
int phaseCurrentsOK (void)
{
	return 0;
}

void focpwmFaultDisable( void );

/*****************************************************************************
** Function name:		EINT0_Handler
**
** Descriptions:		external INT handler (hooked to IGBT fault line)
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
void EINT3_IRQHandler (void)
{
	// disable the interrupt
	NVIC_DisableIRQ(EINT3_IRQn);

	// clear the interrupt
	LPC_SC->EXTINT = EINT3;		/* clear interrupt */

	// connect int to raw fault output
	// this is a real fault
	// latch the fault condition
	faultcond = FAULT_DESATURATE;

	// set snapshot to stop
	SnapFault ();

	// disable foc updates
	focpwmFaultDisable();

	// run state update
	faultRunState ();

	// visual indication
	led2_on();

	// re-enable the fault interrupt
	NVIC_EnableIRQ(EINT3_IRQn);

}

