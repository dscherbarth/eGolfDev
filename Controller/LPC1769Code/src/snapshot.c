#include "LPC17xx.h"
#include "type.h"
#include "can.h"
#include "string.h"
#include "stdio.h"
#include "snapshot.h"
#include "timer.h"

// save and transfer snapshot data on command
//
//	States: Free Running, Demand@Freq, FaultStop, Fetch Waiting
//
//	Transitions: Init, Snap Req, Fault, Fetch Complete
//
//	Interface: Init, RequestSnapAtFreq, SnapFault

static struct snaprec sr[SNAPMAX];		//
int srPos = 0;							// position of next snap write
int	srCollecting = 0;					// set if we are collecting

uint16_t srQset;
uint16_t srSlip;
uint16_t srPost = 0;

#define MAXRESIND	5
int resVal[MAXRESIND] = {0, 1, 9, 19, 39};

static uint16_t sDelay = 0, sDelayCnt = 0;

// set snapshot resolution
void setSres (uint16_t delaySteps)
{
	// set the number of delay steps for each collection so we can fit more time into SNAPMAX collections
	sDelayCnt = sDelay = delaySteps;
}

// Public
void SnapFault ( void )
{
	srCollecting = 0;
}

// Public
// initialize the snapshot data structures and start collecting
void snapInit(void)
{
	int i;


	// reset the shapshot buffer
	for(i=0; i<SNAPMAX; i++)
	{
		sr[i].S1 = 0;
		sr[i].S2 = 0;
		sr[i].S3 = 0;
		sr[i].S4 = 0;
		sr[i].S5 = 0;
		sr[i].S6 = 0;
	}
	srPos = 0;
	srPost = 0;
	srCollecting = 1;
	setSres (0);
}

// Public
int snapSent = 0;
void takeSnapshot (uint16_t qset, uint16_t slip, uint16_t snapRes)
{
	srQset = qset;
	srSlip = slip;

	// if we are stopped due to a fault, don't restart until this snapshot has been picked up
	// start collecting
	srCollecting = 0;
	delayMicros(0, 200, NULL );
	sDelayCnt = 0;
	snapSent = 0;
	srPos = 0;
	if (snapRes == 10)
	{
		setSres (0);
		srPost = 0;		// waiting for fault
	}
	else if (snapRes >= 0 && snapRes < MAXRESIND)
	{
		setSres (resVal[snapRes]);
		srPost = 2000;		// fill the entire buffer
	}
	srCollecting = 1;
}

// Public
void addSRec12 (uint16_t S1, uint16_t S2, uint16_t S3, uint16_t S4, uint16_t S5, uint16_t S6, uint16_t S7, uint16_t S8, uint16_t S9, uint16_t S10, uint16_t S11, uint16_t S12)
{
	if(srCollecting)
	{
		if (sDelayCnt)
		{
			// honor the resolution setting on snapshot collection
			sDelayCnt--;
			return;
		}
		else
		{
			// reset the delay counter
			sDelayCnt = sDelay;
		}

		sr[srPos].S1 = S1;
		sr[srPos].S2 = S2;
		sr[srPos].S3 = S3;
		sr[srPos].S4 = S4;
		sr[srPos].S5 = S5;
		sr[srPos].S6 = S6;
		srPos++;
		sr[srPos].S1 = S7;
		sr[srPos].S2 = S8;
		sr[srPos].S3 = S9;
		sr[srPos].S4 = S10;
		sr[srPos].S5 = S11;
		sr[srPos].S6 = S12;
		srPos++;

		// handle end point detection
		if(srPost)
		{
			srPost--;
			srPost--;
			if (srPost <= 0)
			{
				// stop collecting and wait for fetch
				srCollecting = 0;
			}
		}

		// roll the position index if nec.
		if (srPos >= SNAPMAX)
		{
			srPos = 0;
		}
	}
}

void addSRec (uint16_t S1, uint16_t S2, uint16_t S3, uint16_t S4, uint16_t S5, uint16_t S6)
{
	if(srCollecting)
	{
		if (sDelayCnt)
		{
			// honor the resolution setting on snapshot collection
			sDelayCnt--;
			return;
		}
		else
		{
			// reset the delay counter
			sDelayCnt = sDelay;
		}

		sr[srPos].S1 = S1;
		sr[srPos].S2 = S2;
		sr[srPos].S3 = S3;
		sr[srPos].S4 = S4;
		sr[srPos].S5 = S5;
		sr[srPos].S6 = S6;
		srPos++;

		// handle end point detection
		if(srPost)
		{
			srPost--;
			if (!srPost)
			{
				// stop collecting and wait for fetch
				srCollecting = 0;
			}
		}

		// roll the position index if nec.
		if (srPos >= SNAPMAX)
		{
			srPos = 0;
		}
	}
}

// Public
int moreSnap()
{
	if (snapSent < SNAPMAX + 2)
	{
		return 1;
	}

	// fetching complete, re-set
	snapInit();
	snapSent = 0;
	return 0;
}

// Public
void sendSnap (void)
{
	CAN_MSG		TxMsg;
	int	j, snapIndex;


	// make sure the snapshot is complete
	if (!srCollecting)
	{
		TxMsg.Frame = 0x00080000; /* 11-bit, no RTR, DLC is 8 bytes */
		if (snapSent == 0)
		{
			// send header via canbus
			TxMsg.MsgID = EXP_STD_ID + 150; /* snapshot header*/
			TxMsg.DataA = srQset;
			TxMsg.DataB = srSlip;
			for(j=0; j<500 && !CAN1_SendMessage( &TxMsg ); j++)
				{
				delayMicros(0, 200, NULL );
				}
			snapSent++;
		}
		else if (snapSent < SNAPMAX + 1)
		{
			snapIndex = (srPos + (snapSent-1)) % SNAPMAX;
			TxMsg.MsgID = EXP_STD_ID + 151; /* snapshot data1*/
			TxMsg.DataA = (uint32_t)((uint32_t)sr[snapIndex].S1 + (uint32_t)(sr[snapIndex].S2 << 16));
			TxMsg.DataB = (uint32_t)sr[snapIndex].S3;
			for(j=0; j<500 && !CAN1_SendMessage( &TxMsg ); j++)
				{
				delayMicros(0, 500, NULL );
				}
			TxMsg.MsgID = EXP_STD_ID + 152; /* snapshot data1*/
			TxMsg.DataA = (uint32_t)((uint32_t)sr[snapIndex].S4 + (uint32_t)(sr[snapIndex].S5 << 16));
			TxMsg.DataB = (uint32_t)sr[snapIndex].S6;
			for(j=0; j<500 && !CAN1_SendMessage( &TxMsg ); j++)
				{
				delayMicros(0, 500, NULL );
				}
			snapSent++;
		}
		else if (snapSent >= SNAPMAX + 1)
		{
			TxMsg.MsgID = EXP_STD_ID + 153; /* snapshot close*/
			for(j=0; j<500 && !CAN1_SendMessage( &TxMsg ); j++)
				{
				delayMicros(0, 500, NULL );
				}
			snapInit();		// let collection restart
			snapSent = 0;
		}
	}
}

