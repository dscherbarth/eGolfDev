//
// vss capture and software clutch routine
//

#include "LPC17xx.h"
#include "type.h"
#include "lcd.h"
#include "string.h"
#include "stdio.h"
#include "timer.h"

extern uint32_t timer1_counter;

uint32_t vssPeriod = 0;

#define EINT0_EDGE	0x00000001
#define EINT0		0x00000001

void transInit (void)
{
	// setup the external interrupt
	LPC_PINCON->PINSEL4 = 0x00100000;	/* set P2.10 as EINT0 and */
	LPC_GPIOINT->IO2IntEnF = 0x200;	/* Port2.10 is falling edge. */
	LPC_SC->EXTMODE = EINT0_EDGE;		/* INT0 edge trigger */
	LPC_SC->EXTPOLAR = 0;				/* INT0 is falling edge by default */

	// start the timer to measure microseconds
//	delayMicros(1, 100, NULL );

	// enable the interrupts
//	NVIC_EnableIRQ(EINT0_IRQn);

}

float transRPM (void)
{
	static float transRet = 0;

	if (vssPeriod != 0)
	{
		transRet = (transRet * .7) + (.3 * (float) (10000.0/vssPeriod) * 60.0);
	}

	return transRet;
}

int transGear (float mRPM, float vssRPM)
{
	int	transGear = 0;
	float ratio;
	float first = .1558, second = .2786, third = .4348, fourth = .6061, fifth = .7806, fudge = .02;


	if (mRPM != 0)			// protect against div by 0
		{
		ratio = vssRPM / mRPM;

		if (ratio < (first + fudge) && ratio > (first - fudge))
			{
			transGear = 1;
			}
		else if (ratio < (second + fudge) && second > (first - fudge))
			{
			transGear = 2;
			}
		else if (ratio < (third + fudge) && third > (first - fudge))
			{
			transGear = 3;
			}
		else if (ratio < (fourth + fudge) && fourth > (first - fudge))
			{
			transGear = 4;
			}
		else if (ratio < (fifth + fudge) && fifth > (first - fudge))
			{
			transGear = 5;
			}
		else
			{
			transGear = 0;	// neutral
			}
		}

	// return the current gear
	return transGear;
}

void transMatch (int gear)
{
	// decide what motor rpm is needed to match given the gear and current vss
}


