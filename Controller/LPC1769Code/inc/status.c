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
#include "snapshot.h"
#include "fault.h"

static int32_t	sva[SV_MAX] = {0};	// array of status values

static char *modestrings[] = {
						"PH Test ",
						"FOC     ",
						"SVM     ",
						"SVM Clos"};

static	uint32_t CurrentPage = 0;

void setStatVal (int valindex, int32_t value)
{
	if (valindex >= 0 && valindex < SV_MAX)
	{
		sva [valindex] = value;
	}
}

void DisplayStat (void)
{
	char	temp[24];


	sprintf (temp, "%04d %04d %04d %04d", sva[SVRRPM], sva[SVSRPM], sva[SVACCEL], sva[SVTUNE]);
	lcd_cursor (0, 0);
	lcd_write (temp, strlen(temp));

//	sprintf (temp, "%04d %04d %04d %04d", sva[SVCAP], sva[SVINDEX], sva[SVPOS], sva[SVTEMP]);
	sprintf (temp, "%s  %c %c %03d", modestrings[sva[SVMODE]], sva[SVENAB]? 'E':'D', sva[SVDIR]? 'F':'R', sva[SVTEMP]);
	lcd_cursor (1, 1);
	lcd_write (temp, strlen(temp));

}

static uint32_t	pageSelection = 0;
static uint32_t pageSelRow[] = {0,0,0,1,1,1};
static uint32_t pageSelCol[] = {0,5,9,0,6,12};

static uint32_t	cmdSelection = 0;
static uint32_t cmdSelRow[] = {0,0,0,1,1};
static uint32_t cmdSelCol[] = {0,6,11,0,8};

void SetPage (uint32_t page)
{
	CurrentPage = page;

	// set page appropriate initial conditions
	switch (page)
	{
	case PAGE_OPERATE :		// main operation page
		break;

	case PAGE_COMMAND :		// page to select state change operations
		cmdSelection = 0;
		break;

	case PAGE_SELECT :		// select new display
		pageSelection = 0;
		break;

	case PAGE_POWER	:		// power/battery related page
		break;

	case PAGE_TUNE	:		// display and modify tuning parameters
		break;

	case PAGE_DIAG1	:		// diagnostic info
		break;

	case PAGE_DIAG2	:		// diagnostic info
		break;


	}
}

void stepPage ( void )
{
	// set page appropriate initial conditions
	switch (CurrentPage)
	{
	case PAGE_OPERATE :		// main operation page
		break;

	case PAGE_COMMAND :		// page to select state change operations
		cmdSelection++;
		if (cmdSelection > 4) cmdSelection = 0;
		lcd_cursor_on ( cmdSelRow[cmdSelection], cmdSelCol[cmdSelection] );
		break;

	case PAGE_SELECT :		// select new display
		pageSelection++;
		if (pageSelection > 5) pageSelection = 0;
		lcd_cursor_on ( pageSelRow[pageSelection], pageSelCol[pageSelection] );
		break;

	case PAGE_POWER	:		// power/battery related page
		break;

	case PAGE_TUNE	:		// display and modify tuning parameters
		break;

	case PAGE_DIAG1	:		// diagnostic info
		break;

	case PAGE_DIAG2	:		// diagnostic info
		break;
	}

}

uint32_t	pageArray[] = {PAGE_OPERATE, PAGE_COMMAND, PAGE_POWER, PAGE_DIAG1, PAGE_DIAG2, PAGE_TUNE};

void selectPage ( void )
{
	switch (CurrentPage)
	{
	case PAGE_OPERATE :		// main operation page
		// select means step control mode
		setMode(getMode() + 1);
		if ((getMode()) >= MAXMODE)
			{
			setMode(0);
			}
		setStatVal (SVMODE, getMode());
		break;

	case PAGE_COMMAND :		// page to select state change operations
		switch ((cmdSelection + 1))
		{
		case CMDSTART :
			break;

		case CMDSTOP :
			break;

		case CMDCLEAR :
			// clear the fault and turn off the led's
			break;

		}
		break;

	case PAGE_SELECT :		// select new display
		setStatVal (SVMODE, getMode());
		CurrentPage = pageArray[pageSelection];
		break;

	case PAGE_POWER	:		// power/battery related page
		break;

	case PAGE_TUNE	:		// display and modify tuning parameters
		break;

	case PAGE_DIAG1	:		// diagnostic info
		break;

	case PAGE_DIAG2	:		// diagnostic info
		break;
	}

}

void DisplayPage (void)
{
	static char	temp[24];
	float	bp;


	switch (CurrentPage)
	{
	case PAGE_WELCOME :		// initial welcome page
		lcd_cursor ( 0,0 );
		lcd_write("eGolf v1.8", 10);
		lcd_cursor ( 1,1 );
		lcd_write("06/25/14", 8);
		delayMicros(0, 5000000, NULL );	// 5 seconds
		break;

	case PAGE_OPERATE :		// main operation page
//		sprintf (temp, "%04d %04d %03d %03d", sva[SVRRPM], sva[SVSRPM], sva[SVVOLTS], sva[SVAMPS]);
		sprintf (temp, "%04d %04d %04d", sva[SVRRPM], sva[SVSRPM], sva[SVTUNE]);
		lcd_cursor (0, 0);
		lcd_write (temp, strlen(temp));

	//	sprintf (temp, "%04d %04d %04d %04d", sva[SVCAP], sva[SVINDEX], sva[SVPOS], sva[SVTEMP]);
		sprintf (temp, "%s  %c %c %03d", modestrings[sva[SVMODE]], sva[SVENAB]? 'E':'D', sva[SVDIR]? 'F':'R', sva[SVTEMP]);
		lcd_cursor (1, 0);
		lcd_write (temp, strlen(temp));
		break;

	case PAGE_COMMAND :		// page to select state change operations
		// display the commands
		sprintf (temp, "START STOP CLEAR");
		lcd_cursor (0, 0);
		lcd_write (temp, strlen(temp));
		sprintf (temp, "FORWARD REVERSE");
		lcd_cursor (1, 0);
		lcd_write (temp, strlen(temp));
		break;

	case PAGE_SELECT :		// select new display
		sprintf (temp, "OPER CMD POWER");
		lcd_cursor (0, 0);
		lcd_write (temp, strlen(temp));
		sprintf (temp, "DIAG1 DIAG2 TUNE");
		lcd_cursor (1, 0);
		lcd_write (temp, strlen(temp));
		break;

	case PAGE_POWER	:		// power/battery related page
		sprintf (temp, "%04dv %04da %03dd", sva[SVVOLTS], sva[SVAMPS], sva[SVTEMP]);
		lcd_cursor (0, 0);
		lcd_write (temp, strlen(temp));
		bp = batterypercent();
		sprintf (temp, "Battery %03d", (uint32_t)bp);
		lcd_cursor (1, 0);
		lcd_write (temp, strlen(temp));

		break;

	case PAGE_TUNE	:		// display and modify tuning parameters
		break;

	case PAGE_DIAG1	:		// diagnostic info
		break;

	case PAGE_DIAG2	:		// diagnostic info
		break;

	}

	// display current state and direction characters
	lcd_cursor (0,18);
	sprintf (temp, "%c%c", sva[SVSTATE]==RUNNING?'R':sva[SVSTATE]==FAULTED?'F':'S', '>');
	lcd_write (temp, strlen(temp));

}

void sendHeadData (void)
{
	uint32_t	dataArray[32];
	uint32_t	ret;
	int16_t		i;
	CAN_MSG		TxMsg;
	static	int16_t		lastsent = 0;


	if (SnapRecv())
		return;

	// format and send the current data items
	dataArray[0] = sva[SVRRPM];
	dataArray[1] = sva[SVSRPM];
	dataArray[2] = sva[SVACCEL];
	dataArray[3] = sva[SVVOLTS];
	dataArray[4] = sva[SVAMPS];
	dataArray[5] = sva[SVRAWF];
	dataArray[6] = sva[SVRAWV];
	dataArray[7] = sva[SVTEMP];
	dataArray[8] = sva[SVFAULT];
	dataArray[9] = sva[SVBATP];
	dataArray[10] = sva[SVSTATE];

	// send can-bus messages
	TxMsg.Frame = 0x00080000; /* 11-bit, no RTR, DLC is 8 bytes */
	if (lastsent >= 11) lastsent = 0;
	for (i=lastsent; i<12; i++)
		{
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

#define CANPIDD            30
#define CANPIDQ            31
#define CANPIDS            32

extern float Kp;
extern float Ki;
extern float Kd;
extern float slipFactor;
extern float dqFactor;

extern float decelFactor;


void sendHeadTune (void)
{
	int j;
	CAN_MSG		TxMsg;


	if (SnapRecv())
		return;

	TxMsg.Frame = 0x00080000; /* 11-bit, no RTR, DLC is 8 bytes */
	// send slipFactor and Kp
	TxMsg.MsgID = EXP_STD_ID + CANPIDD; /* Explicit Standard ID + canpidq*/
	TxMsg.DataA = ((int32_t)(slipFactor * 10) & 0x000003ff);
	TxMsg.DataB = ((int32_t)(Kp * 100) & 0x000003ff);
	for(j=0; j<50 && !CAN1_SendMessage( &TxMsg ); j++)
	{
		delayMicros(0, 200, NULL );
	}

	// send Ki and decelFactor
	TxMsg.MsgID = EXP_STD_ID + CANPIDQ; /* Explicit Standard ID + canpidq*/
	TxMsg.DataA = ((int32_t)(Ki * 100) & 0x000003ff);
	TxMsg.DataB = ((int32_t)(decelFactor * 100) & 0x000003ff);
	for(j=0; j<50 && !CAN1_SendMessage( &TxMsg ); j++)
	{
		delayMicros(0, 200, NULL );
	}

	// send Ki and decelFactor
	TxMsg.MsgID = EXP_STD_ID + CANPIDS; /* Explicit Standard ID + canpidq*/
	TxMsg.DataB = ((int32_t)(dqFactor * 100) & 0x000003ff);
	for(j=0; j<50 && !CAN1_SendMessage( &TxMsg ); j++)
	{
		delayMicros(0, 200, NULL );
	}

}

#ifdef SENDFOCTUNE
void sendHeadTune (void)
{
	int j;
	CAN_MSG		TxMsg;
	MC_DATA_Type * mc;

	if (SnapRecv())
		return;

	mc = focGetMC ();

	if (!mc->ctrl.busy)
	{
		// send pidQ	(torque)
		TxMsg.MsgID = EXP_STD_ID + CANPIDQ; /* Explicit Standard ID + canpidq*/
		TxMsg.DataA = (mc->var.foc.pidQ.kp.full & 0x000003ff);
		TxMsg.DataB = (mc->var.foc.pidQ.ki.full & 0x000003ff);
		for(j=0; j<50 && !CAN1_SendMessage( &TxMsg ); j++)
		{
			delayMicros(0, 200, NULL );
		}
	}

	if (!mc->ctrl.busy)
	{
		// send pidD	(rotor flux)
		TxMsg.MsgID = EXP_STD_ID + CANPIDD; /* Explicit Standard ID + canpidq*/
		TxMsg.DataA = (mc->var.foc.pidD.kp.full & 0x000003ff);
		TxMsg.DataB = (mc->var.foc.pidD.ki.full & 0x000003ff);
		for(j=0; j<50 && !CAN1_SendMessage( &TxMsg ); j++)
		{
			delayMicros(0, 200, NULL );
		}
	}

	if (!mc->ctrl.busy)
	{
		// send pidS	(speed)
		TxMsg.MsgID = EXP_STD_ID + CANPIDS; /* Explicit Standard ID + canpidq*/
		TxMsg.DataA = (mc->ctrl.pidSpeed.kp.full & 0x000003ff);
		TxMsg.DataB = (mc->ctrl.pidSpeed.ki.full & 0x000003ff);
		for(j=0; j<50 && !CAN1_SendMessage( &TxMsg ); j++)
		{
			delayMicros(0, 200, NULL );
		}
	}
}

#endif
