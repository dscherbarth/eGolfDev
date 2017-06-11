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

extern int32_t regenVal;

// return a linear 0 - 1 factor based on where we are in this range
float voltFactorGet(int volts, int bottom, int top)
{
	float factor = 1.0;

	if (volts > bottom)
	{
		factor = 1.0 - (float)(volts - bottom) / (top - bottom);
	}
	else if (volts > top)
	{
		factor = 0.0;
	}
	return factor;
}

float ampFactorGet(int amps, int bottom, int top)
{
	float factor = 1.0;

	if (amps > bottom)
	{
		factor = 1.0 - (float)(amps - bottom) / (top - bottom);
	}
	else if (amps > top)
	{
		factor = 0.0;
	}

	return factor;
}

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
	uint32_t	Is = 0, Id_cmd = 0;
	int Iq_cmd = 0;
	int last_Iqc;
	int regen;
	uint32_t localRegenVal;


	setStatVal (SVSRPM, getAccelvalue());
	setStatVal (SVPHAC, (uint32_t)regenVal);

	if (getAccelvalue() > 300) // motoring :-)
	{
		// get Is_sq based on throttle position (table lookup)
		Is = MgetAccSq (getAccelvalue()/2);			//0 - 13500 / 3

		Is *= Is;

		// get Id_cmd based on present rpm (table lookup)
		Id_cmd = MgetMagVal (gRPM);

		if (Id_cmd < Is)
		{
			Iq_cmd = sqrt (Is - (Id_cmd * Id_cmd)); // as big as 27000
		}
		else
		{
			Id_cmd = 0;
		}
	}
	else if (getAccelvalue() < 0)	// decel
	{
		// use regen if requested and bus volts and amps allow
		if (regenVal > 200 &&		// commanded by cockpit lever for debug
			(gRPM > 100) )
		{
#define VOLTSMINHEADROOM 350
#define VOLTSHEADROOMRANGE 10
#define MAXAMPS 20
#define AMPSRANGE 20
			float voltFactor, ampFactor;
			voltFactor = voltFactorGet(getBusvolt(), VOLTSMINHEADROOM, VOLTSMINHEADROOM + VOLTSHEADROOMRANGE);
			ampFactor = ampFactorGet(-getBuscurrent(), MAXAMPS, MAXAMPS + AMPSRANGE);
			setStatVal (SVPHAC, 9999);		// indicator that we are regening..
			regen = (getAccelvalue() * 3) * voltFactor * ampFactor;
			Iq_cmd = (regen * 1.1);

			// slow the delta change
			Iq_cmd = last_Iqc - ((last_Iqc - Iq_cmd) >> 3);

			last_Iqc = Iq_cmd;
			if (Iq_cmd != 0)
			{
				Id_cmd = MgetMagVal (gRPM);
			}
		}
	}

	// load cmd values
	MCEsetQandD(Iq_cmd, Id_cmd);

	focpwmEnable();
}

