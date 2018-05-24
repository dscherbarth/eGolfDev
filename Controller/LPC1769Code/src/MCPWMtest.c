/****************************************************************************
 *   $Id:: MCPWMtest.c 6097 2011-01-07 04:31:25Z nxp12832                   $
 *   Project: NXP LPC17xx Motor Control PWM example
 *
 *   Description:
 *     This file contains MCPWM test modules, main entry, to test MCPWM APIs.
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
#include <cr_section_macros.h>
#include <NXP/crp.h>

// Variable to store CRP value in. Will be placed automatically
// by the linker when "Enable Code Read Protect" selected.
// See crp.h header for more information
__CRP const unsigned int CRP_WORD = CRP_NO_CRP ;

#include "LPC17xx.h"
#include "type.h"
#include "timer.h"
#include "leds.h"
#include "adc.h"
#include "lcd.h"
#include "mcpwm.h"
#include "qei.h"
#include "waveform.h"
#include "control.h"
#include "button.h"
#include "app_setup.h"
#include "status.h"
#include "trans.h"
#include "power.h"
#include "gpio.h"
#include "can.h"
#include "wdt.h"
#include "snapshot.h"
#include "fault.h"
#include "sensors.h"
#include "command.h"
#include "models.h"
#include "ssp.h"
#include "mcpwm.h"


#define HEADSTOPUPDATE 10
#define HEADSTARTUPDATE 11


void vMC_FOC_Init(void);
void MCPWM_EnableLimInt(void);
/*****************************************************************************
** Function name:		initialize
**
** Descriptions:		initialize all of the controller cores
**
** parameters:			none
** Returned value:		nothing
**
*****************************************************************************/
void initialize (void)
{
	/* SystemClockUpdate() updates the SystemFrequency variable */
	SystemClockUpdate();

	setStatVal (SVDIR, 1);		// set direction to forward

	// select the model for the motor we are running
	ModelSelect (BIG);

	// initialization
	led2_init();	// Setup GPIO for LED2
	led2_off();

	// analog
	ADCInit( ADC_CLK );		//ADC_CLK = 13 MHz

	// motor pwm
	pwm_init();

	// generic gpio
	gpio_init();

	// quadrature encoder for rotor position and rpm
	qei_init();

	// mkiii button state init
	buttonInit();

	// external digital devices
	device_init ();

	// foc init

	vMC_FOC_Init();

	// start lim0 based interrupts
	MCPWM_EnableLimInt();

	// init the ssp interface
	SSP0Init( );
	SSP1Init( );

	// init the canbus for comms with the head unit and car
	CAN_Init( BITRATE125K18MHZ );
	CAN_SetACCF( ACCF_BYPASS );

	// start off with the welcome page
//	SetPage (PAGE_WELCOME);
//	DisplayPage ();
	delayMs (0, 1000);
//	SetPage (PAGE_OPERATE);
//	setStatVal (SVMODE, getMode());
//	DisplayPage ();
//	SetPage (PAGE_SELECT);

	// start snapshotting
	snapInit ();

	// init the ext interrupt and timer for vss measurement (may be able to get from the car can bus
	//	  transInit ();

	// init the power remaining
	zerowatthours ();
	setwatthourmax (9079.0);

	// init fault checking
	faultInit();

	// start the watchdog timer
	//	  WDTInit();

	// all set up, adjust interrupt priorities
	NVIC_SetPriority(MCPWM_IRQn, 0);
	NVIC_SetPriority(EINT3_IRQn, 0);		// perhaps should be higher than pwm?
	NVIC_SetPriority(SSP0_IRQn, 1);
	NVIC_SetPriority(SSP1_IRQn, 1);
	NVIC_SetPriority(ADC_IRQn, 2);
	NVIC_SetPriority(CAN_IRQn, 2);
//	NVIC_SetPriority(I2C0_IRQn, 2);
	NVIC_SetPriority(TIMER1_IRQn, 3);
}

int iaz, icz, iav, icv;
void read_and_dispIz (void)
{
	int i;

	delayMicros(0, 100000, NULL );	// settle
	iaz = icz = 0;
	for (i=0; i<1000; i++)
	{
		iaz += getADC (PHASEAINDEX);
		icz += getADC (PHASECINDEX);
		delayMicros(0, 100, NULL );
	}
	iaz /= 1000; icz /= 1000;
	setStatVal (SVPHAC, iaz);
	setStatVal (SVPHCC, icz);

}
void read_and_dispI (void)
{
	int i;

	delayMicros(0, 100000, NULL );	// settle
	iav = icv = 0;
	for (i=0; i<2000; i++)
	{
		iav += getADC (PHASEAINDEX);
		icv += getADC (PHASECINDEX);
		delayMicros(0, 200, NULL );
	}
	iav /= 2000; icv /=2000;
	setStatVal (SVPHAC, iav - iaz);
	setStatVal (SVPHCC, icv - icz);

}

extern uint32_t	gRPM;
static uint16_t	pending_head_request = 0;
void toggleDir (void);
void Isvalset (int val);
/*****************************************************************************
** Function name:		handleHeadRequest
**
** Descriptions:		head unit is requesting a command based action
**
** parameters:			simple request number
** Returned value:		nothing
**
*****************************************************************************/
void handleHeadRequest (uint16_t headReq)
{
	static int test_running = 0;


	headReq = headReq & 0x00ff;

	if (!pending_head_request)
	{
		pending_head_request = headReq;
		return;
	}
	else
	{
		headReq = pending_head_request;
		pending_head_request = 0;
	}

	if (headReq == HEADCMDSTART)
	{
		// start command
		start ();
	}
	else if (headReq == HEADCMDSTOP)
	{
		stop ();
	}
	else if (headReq == HEADCMDCLEAR)
	{
		// clear fault
		fault_reset();
	}
	else if (headReq == HEADCMDCHANGEDIR)
	{
		// re-commishened for now..
		stop ();
//		// change direction
//		if (gRPM == 0)
//		{
//			toggleDir();
//		}
	}
	else if (headReq == HEADCMDTESTDTR)
	{
		// test re-commed for oil pwm
		if (++test_running >= 10)
			test_running = 0;
		SetOilPWM (test_running);
		return;

		if( getRunState() != RUNNING)
		{
			// simply excrecise pwms high and low
			test_running++;
			setStatVal (SVPHAC, test_running);
			switch (test_running)
			{
			case 1:
				set_mcpwm(890, 890, 890);
//				test_outputs(90, 90, 90);
				break;
			case 2:
				set_mcpwm(889, 889, 889);
//				test_outputs(810, 810, 810);
				break;

			case 3:
				set_mcpwm(888, 888, 888);
//				test_outputs(810, 190, 90);
				break;
			case 4:
				set_mcpwm(887, 887, 887);
//				test_outputs(190, 810, 90);
				break;
			case 5:
				set_mcpwm(886, 886, 886);
//				test_outputs(90, 190, 810);
				break;
			case 6:
				set_mcpwm(885, 885, 885);
//				test_outputs(90, 810, 810);
				break;
			case 7:
				set_mcpwm(884, 884, 884);
//				test_outputs(810, 90, 810);
				break;
			case 8:
				set_mcpwm(883, 883, 883);
//				test_outputs(810, 810, 90);
				break;
			case 9:
				set_mcpwm(0, 0, 0);
				break;
			case 10:
				set_mcpwm(898, 898, 898);
				test_running = 0;
				break;
			}
		}
		else
		{

			test_running++;
			switch (test_running)
			{
			case 1:
					// set outputs to absolute zero and collect zero current values
					set_mcpwm(0, 0, 0);
					read_and_dispIz();
//					test_running = 0;	// done with test
					break;

				case 2:
					// set outputs to highest (zero) and collect zero current values
					set_mcpwm(898, 898, 898);
					read_and_dispIz();
					break;

				case 3:
					// set outputs to relative zero and collect zero current values
					set_mcpwm(450, 450, 450);
					read_and_dispIz();
					break;

				case 4:
					// set outputs to relative zero
					set_mcpwm(450, 450, 450);
					delayMicros(0, 200000, NULL );	// settle

					// set test output
					set_mcpwm(425, 450, 450);

					// read current and display
					read_and_dispI();

					// reset outputs to zero
					set_mcpwm(450, 450, 450);

					//Isvalset (4000);
					break;
				case 5:
					// set outputs to relative zero
					set_mcpwm(450, 450, 450);
					delayMicros(0, 200000, NULL );	// settle

					// set test output
					set_mcpwm(450, 425, 450);

					// read current and display
					read_and_dispI();

					// reset outputs to zero
					set_mcpwm(450, 450, 450);

					//Isvalset (4000);
					break;
				case 6:
					// set outputs to relative zero
					set_mcpwm(450, 450, 450);
					delayMicros(0, 200000, NULL );	// settle

					// set test output
					set_mcpwm(450, 450, 425);

					// read current and display
					read_and_dispI();

					// reset outputs to zero
					set_mcpwm(450, 450, 450);

					//Isvalset (4000);
					break;
				case 7:
					// set outputs to relative zero
					set_mcpwm(450, 450, 450);
					delayMicros(0, 200000, NULL );	// settle

					// set test output
					set_mcpwm(475, 450, 450);

					// read current and display
					read_and_dispI();

					// reset outputs to zero
					set_mcpwm(450, 450, 450);

					//Isvalset (4000);
					break;
				case 8:
					// set outputs to relative zero
					set_mcpwm(450, 450, 450);
					delayMicros(0, 200000, NULL );	// settle

					// set test output
					set_mcpwm(450, 475, 450);

					// read current and display
					read_and_dispI();

					// reset outputs to zero
					set_mcpwm(450, 450, 450);

					//Isvalset (4000);
					break;
				case 9:
					// set outputs to relative zero
					set_mcpwm(450, 450, 450);
					delayMicros(0, 200000, NULL );	// settle

					// set test output
					set_mcpwm(450, 450, 475);

					// read current and display
					read_and_dispI();

					// reset outputs to zero
					set_mcpwm(450, 450, 450);

					//Isvalset (4000);
					test_running = 0;	// done with test
					break;

	//		case 2:
	//			//Isvalset (6000);
	//			break;
	//		case 3:
	//			//Isvalset (8000);
	//			break;
	//		case 4:
	//			test_running = 0;
	//			//Isvalset (0);
	//			break;
			}

		}
	}
}

/*****************************************************************************
** Function name:		main
**
** Descriptions:		main processing loop
**
** parameters:			none
** Returned value:		nothing
**
*****************************************************************************/
int main (void)
{
	static int faultCheckWait = 30;		// 3 seconds to let stuff settle before we start checking
	int i;


	// init controller
	initialize ();

	// main control loop, never exit
	for (;;)
	{
		// condition volts, amps, temp...
		updateSensors ();

//		WDTFeed();			// feed the watchdog :-)

		// check for fault conditions (wait for a bit before we act on them)
		if (!faultCheckWait)
		{
			// check for fault conditions (latch a fault condition)
			if (!faulted())
			{
				checkFault (1);
			}
		}
		else
		{
			checkFault (0);
			faultCheckWait--;
		}

		// update the current run and fault state
		setStatVal ( SVSTATE, getRunState() );
		setStatVal ( SVFAULT, faulted() );

		//// handle button selections
		handleButtons ();

		// execute pending command requests
		handleHeadRequest (0);

		// control only if we are not faulted and the state is set to run
		if (!faulted() && getRunState() == RUNNING)
		{
			// execute control algorithm
			for (i=0; i<10; i++)
			{
				// if we get faulted in the middle, break out
				if (faulted() || (getRunState() != RUNNING))
					break;
				exeControl();
				delayMicros(1, 10000, NULL );		// 100 exe/sec
			}
		}
		else
		{
			SetOilPWM (0);				// not running
			for (i=0; i<10; i++)
			{
				delayMicros(1, 10000, NULL );		// 100 exe/sec
			}
		}

		// integrate remaining power
		addwatthours (getBusvolt(), getBuscurrent(), .095);
		setStatVal ( SVBATP,  (uint32_t)batterypercent ());

		// propagate info to the head unit
		sendHeadData();
	}
}

