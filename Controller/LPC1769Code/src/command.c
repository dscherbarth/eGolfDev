#include "LPC17xx.h"
#include "type.h"
#include "adc.h"
#include "lcd.h"
#include "can.h"
#include "qei.h"
#include "gpio.h"
#include "control.h"
#include "snapshot.h"
#include "status.h"
#include "sensors.h"
#include "power.h"
#include "timer.h"
#include "test.h"
#include "ssp.h"
#include "fault.h"
#include "models.h"
#include "gpio.h"
#include "waveform.h"


#define MSGID		20

#define MSGWAVEOFF	0
#define MSGCHKPRESS	1
#define MSGZEROVOLT	2
#define MSGPRECHRG	3
#define MSGHVON		4
#define MSGZEROAMP	5
#define MSGOILON	6
#define MSGINVON	7
#define MSGZEROTHR	8
#define MSGWAVEON	9
#define MSGOILFLT	10
#define MSGOILSWFLT	11
#define MSGFLUXCHG	12
#define MSGGWAVAIL	13
#define MSGRAMPDN	14
#define MSGINVOFF	15
#define MSGOILOFF	16
#define MSGHVOFF	17
#define MSGPRECHGOF	18
#define MSGBLANK	19
#define PREPRECHARGEFAIL	20
#define PRECHARGEFAIL	21

static uint32_t	runStatus = 4;		// 1 run request, 2 running, 3 stop request, 4 stopped, 5 faulted


/*****************************************************************************
** Function name:		msg
**
** Descriptions:		send a message to the lcd screen and also to the head unit
**
** parameters:			message number
** Returned value:		nothing
**
*****************************************************************************/
void msg (int16_t msgnum)
{
	CAN_MSG		TxMsg;


	TxMsg.Frame = 0x00080000; /* 11-bit, no RTR, DLC is 8 bytes */
	TxMsg.MsgID = EXP_STD_ID + MSGID; /* Explicit Standard ID + offset*/
	TxMsg.DataA = msgnum;
	CAN1_SendMessage( &TxMsg );

	sendHeadData();
	delayMicros(0, 100000, NULL );	// long delay for visibility
	sendHeadData();
	delayMicros(0, 100000, NULL );	// long delay for visibility
	sendHeadData();
	delayMicros(0, 100000, NULL );	// long delay for visibility

//	WDTFeed();			// we are not executing the main loop here so feed the watchdog :-)

}

#define MINVOLTCNT 		2200		// counts at 220 volts
#define PREPREVOLTCNT	600			// 85 volts
#define PRECHARGELIMIT	60			// 6 seconds should be plenty of time to pre-charge

uint32_t getRunState (void)
{
	return runStatus;
}

void faultRunState (void)
{
	runStatus = FAULTED;
}

void fault (void)
{
	if (runStatus == RUNNING)
	{
		// execute stop
		focpwmDisable();

	}
	runStatus = FAULTED;
}

void SetRunning (void)
{
//		ModelSelect (MEDIUM);
	ModelSelect (BIG);
	vMC_FOC_Init();
	runStatus = RUNNING;		// running
	fault_reset ();
}

uint32_t started = 0;
/*****************************************************************************
** Function name:		start
**
** Descriptions:		execute the motor controller startup sequence
**
** parameters:			none
** Returned value:		nothing
**
*****************************************************************************/
void start (void)
{
	int				volts, zvolts, nzvolts, vdif;
	int				prechargetime;
	int				i;
	uint32_t		az;


	runStatus = FAULTED;	// initial assumption is faulted

	// see if we are already pre-charged, as we might be if the start is occuring after a fault
	if (started)
	{
		// already pre-charged and contacted
		SetRunning ();
	}
	else
	{
		// 3  make sure oil pressure switch is open (closed at this point is a fail)
		msg(MSGCHKPRESS);
		if (device_test(OILPRESS))
		{
			// 4  collect zero number for buss voltage
			msg(MSGZEROVOLT);
			zerovolts ((float)getADC(VOLTSINDEX));

			// make sure the igbt's are off
			set_mcpwm(0,0,0);	// off

			// 5  close the pre-charge relay and wait for bus voltage to reach minval and stabilize
			// dave and doug's new precharge...
			msg(MSGPRECHRG);

			// collect a relative voltage stating point
			zvolts = getADC(VOLTSINDEX);

			// pre-precharge 600ms on needs voltage to go from 0 to 99v else fault
			for (i=0; i<8; i++)
			{
				updateSensors ();
				sendHeadData();
				device_on (PRECHARGE);
				delayMicros(0, 100000, NULL );
			}
			nzvolts = getADC(VOLTSINDEX);
			if ((nzvolts - zvolts) > PREPREVOLTCNT)
			{
				// pre-precharge is ok
				// while not timing out wait for voltage to settle above min threshold
				prechargetime = 0;
				volts = getADC(VOLTSINDEX);
				while (1)
				{
					updateSensors ();

					// wait for voltage to settle
					delayMicros(0, 100000, NULL );		//


					vdif = getADC(VOLTSINDEX) - volts;
					if ( getADC(VOLTSINDEX) > MINVOLTCNT && vdif < 50)
					{
						// pre-charge complete
						break;
					}

					if (prechargetime > PRECHARGELIMIT)
					{
						device_off (PRECHARGE);
						SetSolPWM (0);
						msg(PRECHARGEFAIL);
						delayMicros(0, 1000000, NULL );
						msg(MSGBLANK);
						return;				// fault
					}
					prechargetime++;
					volts = getADC(VOLTSINDEX);
				}
			}
			else
			{
				// pre-precharge fail, might be a short
				SetSolPWM (0);
				device_off (PRECHARGE);
				setStatVal (SVPHAC, zvolts);
				setStatVal (SVPHCC, nzvolts);
				msg(PREPRECHARGEFAIL);					// fault
				delayMicros(0, 1000000, NULL );
				msg(MSGBLANK);
				return;
			}

			// 7  close the main controller contactor
			msg(MSGHVON);
			SetSolPWM (10);
			for (i=0; i<6; i++)
			{
				updateSensors ();
				sendHeadData();
				delayMicros(0, 100000, NULL );
			}
			SetSolPWM (4);		// reduce coil energy
			started = 1;
			device_on (PRECHARGE);

			// 8  collect zero number for buss current
			msg(MSGZEROAMP);
			for (i=0; i<5; i++)
			{
				updateSensors ();
				sendHeadData();
				delayMicros(0, 100000, NULL );
			}
			for (i=0, az=0; i<10; i++)
			{
				updateSensors ();
				az += getBusIVal ();
				sendHeadData();
				delayMicros(0, 100000, NULL );	// wait for current to settle
			}

			zeroamps ((float)(az / 10));

			if (1)			// till hw catches up
			{
				// 11 collect zero for throttle pos
				msg(MSGZEROTHR);
				saveOffsets();

				// 12A test that we can exercise the pwm's and get reasonable results
	//			TestPhaseCurrent(&phdata);
	//			if (TestValidate (&phdata))		// wait till we can collect reasonable data to validate against
				if (1)
				{
					// 12 enable the waveforms!
					msg(MSGWAVEON);

					// init the foc state (this is really a reset, initialization happens much earlier)
					SetRunning ();
				}
				else
				{
					runStatus = FAULTED;	// start fail, fault
					msg(MSGOILSWFLT);

					// something is wrong, shut stuff off!
					SetSolPWM (0);
					device_off (PRECHARGE);
					delayMicros(0, 1000000, NULL );
					msg(MSGBLANK);

					return;
				}
			}
			else
			{
				msg(MSGOILFLT);
				delayMicros(0, 1000000, NULL );
				msg(MSGBLANK);
				return;		// fault
			}
		}
		else
		{
			runStatus = FAULTED;	// start fail, fault
			msg(MSGOILSWFLT);
			delayMicros(0, 500000, NULL );
			msg(MSGBLANK);
			delayMicros(0, 1000000, NULL );
			return;		// fault
		}
	}

	// clear faults
	fault_reset();

	msg(MSGGWAVAIL);
	msg(MSGBLANK);

}

/*****************************************************************************
** Function name:		stop
**
**
** Descriptions:		execute the motor controller stop sequence
**
** parameters:			none
** Returned value:		nothing
**
*****************************************************************************/
void stop (void)
{
	int i;


	// stop command

	msg(MSGWAVEOFF);

	// wait till heatsink temp is less than 100F and turn off cooling pump
	runStatus = STOPPED;		// stopped
	focpwmDisable();
	msg(MSGOILOFF);

	// open main contactor
	delayMicros(0, 200000, NULL );	// hopefully there is no current on the contactor now
	SetSolPWM (0);

	started = 0;
	for (i=0; i<100; i++)
	{
		// open pre-charge relay
		device_off (PRECHARGE);
		delayMicros(0, 1000, NULL );
	}


	msg(MSGHVOFF);

	msg(MSGBLANK);

	// reset the fault condition
	fault_reset();
}


