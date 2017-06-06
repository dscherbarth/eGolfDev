//
// hold and display current status
//

#include "LPC17xx.h"
#include "type.h"
#include "lcd.h"
#include "can.h"
#include "string.h"
#include "stdio.h"
#include "app_setup.h"
#include "status.h"
#include "control.h"
#include "power.h"
#include "waveform.h"
#include "gpio.h"
#include "timer.h"
#include "fault.h"
#include "snapshot.h"

static int32_t	sva[SV_MAX] = {0};	// array of status values

extern char *modestrings[];
extern uint32_t mode;
extern uint32_t	gRPM;

extern uint32_t dacRValue;
extern uint32_t	limitState;

void setStatVal (int valindex, int32_t value)
{
	if (valindex >= 0 && valindex < SV_MAX)
	{
		sva [valindex] = value;
	}
}

void sendHeadData (void)
{
	uint32_t	dataArray[32];
	uint32_t	ret;
	int16_t		i;
	CAN_MSG		TxMsg;
	static	int16_t		lastsent = 0;


	// format and send the current data items
	dataArray[0] = sva[SVRRPM];
	dataArray[1] = sva[SVSRPM];
	dataArray[2] = sva[SVACCEL];
	dataArray[3] = sva[SVVOLTS];
	dataArray[4] = sva[SVAMPS];
	dataArray[5] = sva[SVRAWF];
	dataArray[6] = sva[SVRAWV];
	dataArray[7] = sva[SVTEMP];
	dataArray[8] = sva[SVPHAC];
	dataArray[9] = sva[SVPHCC];
	dataArray[10] = sva[SVFAULT];
	dataArray[11] = sva[SVBATP];
	dataArray[12] = sva[SVSTATE];
	dataArray[13] = dacRValue;
	dataArray[14] = limitState;

	// send can-bus messages
	TxMsg.Frame = 0x00080000; /* 11-bit, no RTR, DLC is 8 bytes */
	if (lastsent >= 14) lastsent = 0;
	for (i=lastsent; i<15; i++)
		{
		if (i == 14)
		{
			limitState = 0;		// reset on send
		}
		TxMsg.MsgID = EXP_STD_ID + i; /* Explicit Standard ID + index*/
		TxMsg.DataA = dataArray[i];
		ret = CAN1_SendMessage( &TxMsg );
		if (0 == ret)
			{
			lastsent = i;
			break;
			}
		else
			{
			lastsent = i;
			}
		}

}

