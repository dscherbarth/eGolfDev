//
// sensors.c
//
// collect and condition data from current, voltage and temperature sensors
//
#include "lpc17xx.h"
#include "type.h"
#include "timer.h"
#include "adc.h"
#include "status.h"
#include "power.h"
#include "control.h"
#include "sensors.h"
#include "qei.h"
#include "ssp.h"

static float busvolt;			// current bus voltage
static float buscurrent;

static float avgTemp = 0;

static float accelValue = 0;       // the current reading from the motor speed potentiometer
static float accelOffset = 0;
static float tuneValue = 0;			// value of the tuning pot


typedef struct temptable {
	uint32_t	baseTemp;		// starting temp
	uint32_t	baseCounts;		// starting counts
} _temptable;

//static _temptable	temp[] = {	{0,			500},
//								{32, 		1040},
//								{70, 		1952},
//								{95,		2424},
//								{210,		3704},
//								{500,		4095},
//								{-1,		-1}
//};
static _temptable	temp[] = {	{500,		0},
								{180, 		482},
								{165, 		584},
								{150, 		745},
								{111, 		1385},
								{82, 		2108},
								{75, 		2191},
								{38,		2901},
								{25,		3079},
								{0,			4095},
								{-1,		-1}
};

/******************************************************************************
** Function name:		tempTab
**
** Descriptions:		return temp in degF from adc counts
**
** parameters:			adc counts
** Returned value:		temp
**
******************************************************************************/
int tempTab (uint32_t adc)
{
	int	tempIndex;
	int rtemp, itdif;
	float vperc, tdif;


	// search the table
	for (tempIndex = 0; temp[tempIndex].baseTemp != -1; tempIndex++)
		{
			if (adc >= temp[tempIndex].baseCounts && adc <temp[tempIndex+1].baseCounts)
			{
				// found the segment, calculate the voltage to return
				rtemp = temp[tempIndex].baseTemp;
				vperc = (float)(adc - temp[tempIndex].baseCounts) / (float)(temp[tempIndex+1].baseCounts - temp[tempIndex].baseCounts);
				tdif = (float)((temp[tempIndex].baseTemp - temp[tempIndex+1].baseTemp));
				tdif *= vperc;
				itdif = (int)tdif;
				rtemp -= itdif;
				return rtemp;
			}
		}

	// not found! zero should be safe
	return 0;
}

//
// getTunevalue
//
float getTunevalue (void)
{
	return tuneValue;
}

float accelRPM = 0.0;
float getAccelRPM (void)
{
	return (accelRPM / 21.0);	// accel/decel in G's
}

float getAccelvalue (void)
{
	return accelValue;
}

void saveOffsets()
{
	int i;
	static float acctot = 0.0;


	for (i=0; i<10; i++)
	{
		acctot += (float)getaccVal ();
		delayMicros(0, 10000, NULL );
	}

	acctot /=10;
	accelOffset = acctot;
}

float getBusvolt (void)
{
	return busvolt;
}

float GetSupVolt ()
{
	float supplyv;


	supplyv = getBusvolt();

	// range check
	if (supplyv < SUPPLYVOLTAGELOWRANGE)
	{
		supplyv = SUPPLYVOLTAGELOWRANGE;
	}
	if (supplyv > SUPPLYVOLTAGEHIGHRANGE)
	{
		supplyv = SUPPLYVOLTAGEHIGHRANGE;
	}

	return supplyv;

}

float getBuscurrent (void)
{
	return buscurrent;
}

float getTemp (void)
{
	return avgTemp;
}

extern	uint32_t			gRPM;
float accel_avg = 0.0;
int32_t last_rpm = 0;

void updateSensors (void)
{
	static uint32_t	lasttemp = 0;
	static uint32_t	lastvolt = 0;
	static uint32_t	lastbusa = 0;
	static uint32_t	tempDeg;
	static float		lRPM = 0;
	static int readfirst = 2;
	uint32_t	avgTemp32;
	float	accelScaleAdder;
	static float avgaccValue = 0.0;


	// make sure we have some good data then set offsets and zero values
	if (--readfirst == 0)
	{
		saveOffsets();
		zerovolts ((float)getADC (VOLTSINDEX));
		zeroamps ((float)getBusIVal ());
		readfirst = 0;
	}

	// temperature
	// read until we get 2 readings that match (if temp is changing quickly we will get no readings during change...)
	if ((getADC (TEMPINDEX) >> 2) == (lasttemp >> 2))
	{
		// convert to degF
		tempDeg = tempTab (getADC (TEMPINDEX));

		// average
		avgTemp = (avgTemp * .6) + ((float)tempDeg * .4);
		avgTemp32 = avgTemp;
		setStatVal (SVTEMP, avgTemp32);
	}
	lasttemp = getADC (TEMPINDEX);

	// bus voltage
	if ((getADC (VOLTSINDEX) >> 3) == (lastvolt >> 3))
	{
		busvolt = (busvolt * .4) + ((float)getADC (VOLTSINDEX) * .0912 * .6);	// Get the value
		setStatVal (SVVOLTS, busvolt);
	}
	lastvolt = getADC (VOLTSINDEX);

	// bus current
	// convert to Amps
	if ((getBusIVal () >> 2) == (lastbusa >> 2))
	{
		buscurrent = (buscurrent * .7) + ((((float)getBusIVal () - getampoffset()) * .2929) * .3);	// Get the value

		// average
		setStatVal (SVAMPS, buscurrent * 10);		// shift by 1 dec to make fixed point
	}
	lastbusa = getBusIVal ();

	// accelerator
	// read from pot position
	avgaccValue = (avgaccValue * .4) + ((float)getaccVal () * .6);
	accelValue = avgaccValue + ((avgaccValue - accelOffset) * .215); // rescale to full range
	accelValue -= 930;				// space for regen

	// tune
//	tuneValue = (tuneValue * .7) + ((float)getADC (TUNEINDEX) * .3);	// Get the value

//	setStatVal ( SVTUNE, (int32_t)((float)(tuneValue)) );

	// fetch rpm
	int32_t trpm = qei_readRPM();
	lRPM = (lRPM * .1) + (.9 * (float)trpm);
	gRPM = lRPM;

	// calc acceleration
	accelRPM = (accel_avg * .9) + (.1 * ((float)(trpm - last_rpm)));
	last_rpm = trpm;
	accel_avg = accelRPM;

	// get rpm and position data
	setStatVal (SVRRPM, gRPM);		//
	setStatVal (SVPOS, qei_readPos());

}
