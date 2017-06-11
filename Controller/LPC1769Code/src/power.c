//
// power.c
//
// functions to support read/display of volts and amps and integrate and display watt hours consumed
//
#include "LPC17xx.h"
#include "type.h"
#include "status.h"
#include "sensors.h"

#define MAXWATTHOUR		10560		// current lion pack

float	watthours;
float 	voffset = 0.0, aoffset = 0.0;		// set zero volts and amps before bus is turned on
float	battcapacity = 1;
uint32_t	aoffset32 = 2036;

void zerovolts (float volts)
{
	// with the solenoid off there should be no voltage or current numbers so save these values as the 'zero' point
//	voffset = volts * .0912;
}

void zeroamps (float amps)
{
	// with the solenoid off there should be no voltage or current numbers so save these values as the 'zero' point
//	aoffset = amps * .2929;
//	aoffset32 = amps;
}

uint32_t getampoffset (void)
{
	// with the solenoid off there should be no voltage or current numbers so save these values as the 'zero' point
	return aoffset32;
}
int sec_count = 0;
void zerowatthours (void)
{

	sec_count = 0;
	watthours = 0.0;
}

void addwatthours (float volts, float amps, float seconds)
{
	watthours += ((amps - aoffset) * (volts - voffset) * (seconds / 3600.0) );		// should handle regen too if amps is negative
	sec_count++;
//	setStatVal (SVPHAC, (uint32_t)(getBusIVal ()));
//	setStatVal (SVSRPM, (uint32_t)(aoffset));
	setStatVal (SVPHCC, (uint32_t)watthours);

}
void setwatthourmax (float maxwatthour)
{
	battcapacity = MAXWATTHOUR;
}

float batterypercent (void)
{
	float battpercent;

	if (watthours == 0 || battcapacity == 0)
	{
		battpercent = 0.0;
	}
	else
	{
		if (watthours > battcapacity)
		{
			watthours = battcapacity;	// ideally should not happen :-)
		}
		battpercent = (1.0 - (watthours / battcapacity)) * 100.0;
	}
	if (battpercent > 100.0)
	{
		battpercent = 100.0;
	}
	return battpercent;
}

