#include "LPC17xx.h"
#include "type.h"
#include "adc.h"
#include "mcpwm.h"
#include "qei.h"
#include "lcd.h"
#include "can.h"
#include "gpio.h"
#include "control.h"
#include "status.h"
#include "sensors.h"
#include "timer.h"
#include "waveform.h"
#include "leds.h"
#include "wdt.h"
#include "power.h"
#include "app_setup.h"
#include "models.h"
#include "sensors.h"
#include "ssp.h"


#define NUMPOLES	4		// big motor

uint32_t enableController=1;


uint32_t	gRPM = 0;

void MCEsetQandD (int Q, uint32_t D);

#define OVERI		250
#define OVERILIMADJ	10
#define REGEN		25
#define OVERINEG 	-50
#define OVERV 		330

/******************************************************************************
** Function name:		regenval
**
** Descriptions:		calculate regen value based on rpm, bus voltage and bus current
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
int regenval()
{
	int rval = 0;


	// inital val based on rpm
	rval = gRPM / 25;

	// limit if bus current too high
	if (getBuscurrent() < OVERINEG)
	{
		// toomuch regen current
		rval = rval * (1 - ((getBuscurrent() + OVERINEG) / OVERINEG));
	}

	// limit if bus current too high
	if (getBuscurrent() < OVERV)
	{
		// toomuch regen current
		rval = rval * (1 - ((getBusvolt() + OVERV) / OVERV));
	}


	return rval;
}
extern uint32_t regenVal;

/******************************************************************************
** Function name:		readAndControl
**
** Descriptions:		execute the control algorithm
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void exeControl(void)
{
	uint32_t	Is = 0, Id_cmd = 1;
	int Iq_cmd = 0;
	int regen;


	setStatVal (SVSRPM, getAccelvalue()/2);

	setStatVal ( SVACCEL, getAccelvalue()/2);

	Id_cmd = 1;
	Iq_cmd = 0;

	// get Is_sq based on throttle position (table lookup)
	Is = MgetAccSq (getAccelvalue()/2);			//0 - 13500 / 3

	Is *= Is;
	if (Is > 0) // motoring :-)
	{
		if (Id_cmd < Is)
		{
			Iq_cmd = sqrt (Is - (Id_cmd * Id_cmd)); // as big as 27000

			// get Id_cmd based on present rpm (table lookup)
			Id_cmd = MgetMagVal (gRPM);
		}
	}
	else	// zero throttle
	{
		// use regen if requested and bus volts and amps allow
		if ((gRPM > 100) &&
			(getBusvolt () < 310) &&
			(getBuscurrent () < 40))
		{
//			if (regenVal > 0)
//			{
//				Iq_cmd = - (regenVal * 2);
//			}
		}
	}

	// bus current limit
	if (getBuscurrent() > OVERI)
	{
		Iq_cmd -= OVERILIMADJ;
	}

	// load cmd values
	MCEsetQandD(Iq_cmd, Id_cmd);

	focpwmEnable();
}

