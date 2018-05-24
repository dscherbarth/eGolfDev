#include "LPC17xx.h"
#include "type.h"
#include "control.h"
#include "status.h"
#include "sensors.h"
#include "models.h"


uint32_t	gRPM = 0;

void MCEsetQandD (int Q, uint32_t D);

float getF(int val, int bottom, int top)
{
	if (val <= bottom)
	{
		return 1.1;
	}
	else if (val > bottom)
	{
		return (1.1 - (float)(val - bottom) / (float)(top - bottom));
	}
	else if (val > top)
	{
		return 0.1;
	}
	return 0.1;
}

// return a linear 0 - 1 factor based on where we are in this range
float voltFactorGet(int volts, int bottom, int top)
{
	float factor;
	static float avg_factor = 0.0;

	factor = getF(volts, bottom, top);

	factor = (avg_factor * .7) + (factor * .3);
	avg_factor = factor;

	return factor;
}

float ampFactorGet(int amps, int bottom, int top)
{
	float factor;
	static float avg_factor = 0.0;

	factor = getF(amps, bottom, top);

	factor = (avg_factor * .6) + (factor * .4);
	avg_factor = factor;

	return factor;
}

float speedFactorGet(void)
{
	float factor;
	static float avg_factor = 0.0;

	// less than about 45 mph is full factor
	if (gRPM < 2800)
	{
		factor = 1.0;
	}
	else if (gRPM > 2800 && gRPM < 4500) // 45 to 60 sees a linear reduction
	{
		factor = 1.0 - ((float)(gRPM - 2800) / 1700.0) + .05;
	}
	else	// beyond 60 no regen
	{
		factor = .05;
	}
	factor = (avg_factor * .6) + (factor * .4);
	avg_factor = factor;

	return factor;
}

static float Iq_avg = 0.0;

/*
 * can control be stateless?  at the least we probably need to average the resultant Iq, Id
 */
extern int32_t regenVal;
/******************************************************************************
** Function name:		exeControl
**
** Descriptions:		execute the control algorithm
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void exeControl(void)
{
	uint32_t	Id_cmd = 0;
	int Iq_cmd = 0;


//	setStatVal (SVSRPM, getAccelvalue());
//	setStatVal (SVPHAC, (uint32_t)(getAccelRPM() * 100)); // G's * 100

	// greater than 100 is accelerating, less than -50 is decelerating, in between is no action
	if (getAccelvalue() > 200) // motoring :-)
	{
		// get Is_sq based on throttle position (table lookup)
		Iq_cmd = MgetAccSq (getAccelvalue()/2);			//0 - 13500 / 3

		// get Id_cmd based on present rpm (table lookup)
		Id_cmd = MgetMagVal (gRPM);

		Iq_avg = 0.0;
	}
	else if (getAccelvalue() > -420 && getAccelvalue() < -50 && (regenVal & 1))	// decel (regen)
	{
		float mvFactor;

		// use regen if requested and bus volts and amps allow
		if (gRPM > 400 )
		{
#define VOLTSMINHEADROOM 355
#define VOLTSHEADROOMRANGE 15
#define MAXAMPS 25
#define AMPSRANGE 10
			float voltFactor, ampFactor, speedFactor;
			voltFactor = voltFactorGet(getBusvolt(), VOLTSMINHEADROOM, VOLTSMINHEADROOM + VOLTSHEADROOMRANGE);
			ampFactor = ampFactorGet(-getBuscurrent(), MAXAMPS, MAXAMPS + AMPSRANGE);
			speedFactor = speedFactorGet();
			Iq_cmd = (int)(1.5 * ((float)getAccelvalue() * 4.0) * ampFactor * voltFactor * speedFactor);
			if (Iq_cmd < -3000) Iq_cmd = -3000;
			mvFactor = 1.0;
		}
		else if (gRPM <= 400)
		{
			// taper to zero for a soft disconnect
			Iq_cmd = 0.0;	// soft stop
			mvFactor = (float)gRPM / 500.0;
		}
		// slow the delta change
//		Iq_avg = (Iq_avg * .6) + (Iq_cmd * .4);
//		Iq_cmd = Iq_avg;

		if (Iq_cmd != 0)
		{
			Id_cmd = (int)((float)MgetMagVal (gRPM) * mvFactor);
		}
	}
	setStatVal (SVSRPM, getAccelvalue());
//	setStatVal (SVSRPM, Iq_cmd);
	setStatVal (SVPHAC, Iq_cmd);
//extern uint32_t iCounter;
//setStatVal (SVPHAC, iCounter);

	// load cmd values
	MCEsetQandD(Iq_cmd, Id_cmd);

	focpwmEnable();
}

