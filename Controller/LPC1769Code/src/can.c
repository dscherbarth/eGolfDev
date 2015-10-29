#include "lpc17xx.h"
#include "type.h"
#include "app_setup.h"
#include "can.h"
#include "lcd.h"
#include "snapshot.h"
#include "status.h"
#include "button.h"
#include "models.h"
#include "timer.h"


/* Receive Queue: one queue for each CAN port */
CAN_MSG MsgBuf_RX;
volatile uint32_t CAN1RxDone;

volatile uint32_t CANStatus;
volatile uint32_t CANIStatus;
volatile uint32_t CAN1RxCount = 0;
volatile uint32_t CAN1TxCount = 0;
uint32_t CAN1ErrCount = 0;
uint32_t dacRValue = -1;		// unset initially

/******************************************************************************
** Function name:		CAN_ISR_Rx1
**
** Descriptions:		CAN Rx1 interrupt handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void CAN_ISR_Rx( void )
{
  uint32_t * pDest;

  /* initialize destination pointer	*/
  pDest = (uint32_t *)&MsgBuf_RX;
  *pDest = LPC_CAN1->RFS;  /* Frame	*/

  pDest++;
  *pDest = LPC_CAN1->RID; /* ID	*/

  pDest++;
  *pDest = LPC_CAN1->RDA; /* Data A */

  pDest++;
  *pDest = LPC_CAN1->RDB; /* Data B	*/

  CAN1RxDone = TRUE;
  LPC_CAN1->CMR = 0x01 << 2; /* release receive buffer */
  return;
}

void handleHeadRequest (uint16_t headReq);
void closedUpdateTune (int value, int param);

int32_t dtab = 250;
int32_t reflect = 1;
int32_t dacaten = 5;
void sendCANData (uint32_t id, uint32_t data)
{
	CAN_MSG		TxMsg;
	int j;

	TxMsg.Frame = 0x00080000; /* 11-bit, no RTR, DLC is 8 bytes */
	TxMsg.MsgID = id;
	TxMsg.DataA = data;
	for(j=0; j<50 && !CAN1_SendMessage( &TxMsg ); j++)
		{
		delayMicros(0, 200, NULL );
		}
	delayMicros(0, 2000, NULL );

}
void sendtuneparms()
{
	float p, in, d, tr;

	MgetDPID (&p, &in, &d);			//
	sendCANData (EXP_STD_ID + 250, (int32_t)(p * 100));
	sendCANData (EXP_STD_ID + 250, (int32_t)(in * 1000));
	sendCANData (EXP_STD_ID + 250, (int32_t)(dtab));

	MgetQPID (&p, &in, &d);			//
	sendCANData (EXP_STD_ID + 250, (int32_t)(p * 100));
	sendCANData (EXP_STD_ID + 250, (int32_t)(in * 1000));
	d = MgetDgain ();
	sendCANData (EXP_STD_ID + 250, (int32_t)(d * 10));
	tr = MgetTimeConst ();
	sendCANData (EXP_STD_ID + 250, (int32_t)(tr * 1000));
	sendCANData (EXP_STD_ID + 250, (int32_t)(reflect));
	dacaten = MgetPostgain();
	sendCANData (EXP_STD_ID + 250, (int32_t)(dacaten));
}

uint32_t regenVal = 0;
static float dpval, dival, ddval = 0.0, qpval, qival, qdval, trval;
void HandleCANRx(CAN_MSG *MsgBuf_RX)
{
	short temp;
	float tempf;

	switch(MsgBuf_RX->MsgID)
	{
	case 356: // this is an action command (start/stop/clear fault)
		handleHeadRequest (MsgBuf_RX->DataA & 0x000000ff);
		break;

	case 357:	// this is a request to change what is being reflected to the DAC
		dacRValue = (MsgBuf_RX->DataA & 0x000000ff);
		break;

	case 390:	// command to fetch tuning params
		sendtuneparms ();
		break;

	case 380:	// start snapshot
		takeSnapshot(0, 0, (MsgBuf_RX->DataA & 0x000000ff));
		break;

	case 381:	// fetch snapshot;
		if (moreSnap())
			{
			sendSnap();
			}
		break;

	case 382:	// accel rpm position requested

//		setJog((float) 4321.0);
		temp = (short)MsgBuf_RX->DataA;
		setTorque(temp);
		tempf = (float) temp;
		setJog(tempf * .685);
		break;

	case 383:	// accel torque set

		temp = (short)MsgBuf_RX->DataA;
		setTorque(temp);
		break;

	case 507:	// dptune
		dpval = (float)((short)MsgBuf_RX->DataA) / 100.0;
		break;
	case 508:	// ditune
		dival = (float)((short)MsgBuf_RX->DataA) / 1000.0;
		break;
	case 509:	// dtable
		dtab = (short)MsgBuf_RX->DataA;
		break;
	case 510:	// qptune
		qpval = (float)((short)MsgBuf_RX->DataA) / 100.0;
		break;
	case 511:	// qitune
		qival = (float)((short)MsgBuf_RX->DataA) / 1000.0;
		break;
	case 512:	// qdtune
		qdval = (float)((short)MsgBuf_RX->DataA);
		MsetDgain (qdval / 10);
		break;
	case 513:	// trtune
		trval = (float)((short)MsgBuf_RX->DataA) / 1000.0;
		break;
	case 514:	// reflect
		reflect = (int32_t)((short)MsgBuf_RX->DataA);
		break;
	case 515:	// dacaten
		dacaten = (int32_t)((short)MsgBuf_RX->DataA);
		MsetPostgain (dacaten);
		ddval = 0;
		MsetDPID (&dpval, &dival, &ddval);
		qdval = 0;
		MsetQPID (&qpval, &qival, &qdval);
		MsetTimeConst (trval);
		break;

	case 601:	// regen
		regenVal = (int32_t)((short)MsgBuf_RX->DataA) - 50;
		break;
	}
}

/*****************************************************************************
** Function name:		CAN_Handler
**
** Descriptions:		CAN interrupt handler
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
void CAN_IRQHandler(void)
{
	CANIStatus = LPC_CAN1->ICR;
	if (CANIStatus & 0x80)
	{
		// bus error, try to turn it back on..
		LPC_CAN1->MOD = 1;    /* Reset CAN */
		LPC_CAN1->GSR = 0;    /* Reset error counter when CANxMOD is in reset	*/
		LPC_CAN1->MOD = 0x0;  /* CAN in normal operation mode */
	}
	else
	{
		  CANStatus = LPC_CANCR->RxSR;
		  if ( CANStatus & (1 << 8) )
		  {
			CAN1RxCount++;
			CAN_ISR_Rx();
			HandleCANRx(&MsgBuf_RX);
		  }
		  if ( LPC_CAN1->GSR & (1 << 6 ) )
		  {
			/* The error count includes both TX and RX */
			CAN1ErrCount = LPC_CAN1->GSR >> 16;
		  }
	}
  return;
}

/******************************************************************************
** Function name:		CAN_Init
**
** Descriptions:		Initialize CAN, install CAN interrupt handler
**
** parameters:			bitrate
** Returned value:		true or false, false if initialization failed.
**
******************************************************************************/
uint32_t CAN_Init( uint32_t can_btr )
{
  CAN1RxDone = FALSE;

  LPC_SC->PCONP |= (1<<13);  /* Enable CAN1 clock */

  LPC_PINCON->PINSEL0 &= ~0x0000000F;  /* CAN1 is p0.0 and p0.1	*/
  LPC_PINCON->PINSEL0 |= 0x00000005;

  LPC_CAN1->MOD = 1;    /* Reset CAN */
  LPC_CAN1->IER = 0;    /* Disable Receive Interrupt */
  LPC_CAN1->GSR = 0;    /* Reset error counter when CANxMOD is in reset	*/

  LPC_CAN1->BTR = can_btr;
  LPC_CAN1->MOD = 0x0;  /* CAN in normal operation mode */

  NVIC_EnableIRQ(CAN_IRQn);

  LPC_CAN1->IER = 0x81; /* Enable receive interrupts and bus error interrupts*/
  return( TRUE );
}

/******************************************************************************
** Function name:		CAN_SetACCF_Lookup
**
** Descriptions:		Initialize CAN, install CAN interrupt handler
**
** parameters:			bitrate
** Returned value:		true or false, false if initialization failed.
**
******************************************************************************/
void CAN_SetACCF_Lookup( void )
{
  uint32_t address = 0;
  uint32_t i;
  uint32_t ID_high, ID_low;

  /* Set explicit standard Frame */
  LPC_CANAF->SFF_sa = address;
  for ( i = 0; i < ACCF_IDEN_NUM; i += 2 )
  {
	ID_low = (i << 29) | (EXP_STD_ID << 16);
	ID_high = ((i+1) << 13) | (EXP_STD_ID << 0);
	*((volatile uint32_t *)(LPC_CANAF_RAM_BASE + address)) = ID_low | ID_high;
	address += 4;
  }

  /* Set group standard Frame */
  LPC_CANAF->SFF_GRP_sa = address;
  for ( i = 0; i < ACCF_IDEN_NUM; i += 2 )
  {
	ID_low = (i << 29) | (GRP_STD_ID << 16);
	ID_high = ((i+1) << 13) | (GRP_STD_ID << 0);
	*((volatile uint32_t *)(LPC_CANAF_RAM_BASE + address)) = ID_low | ID_high;
	address += 4;
  }

  /* Set explicit extended Frame */
  LPC_CANAF->EFF_sa = address;
  for ( i = 0; i < ACCF_IDEN_NUM; i++  )
  {
	ID_low = (i << 29) | (EXP_EXT_ID << 0);
	*((volatile uint32_t *)(LPC_CANAF_RAM_BASE + address)) = ID_low;
	address += 4;
  }

  /* Set group extended Frame */
  LPC_CANAF->EFF_GRP_sa = address;
  for ( i = 0; i < ACCF_IDEN_NUM; i++  )
  {
	ID_low = (i << 29) | (GRP_EXT_ID << 0);
	*((volatile uint32_t *)(LPC_CANAF_RAM_BASE + address)) = ID_low;
	address += 4;
  }

  /* Set End of Table */
  LPC_CANAF->ENDofTable = address;
  return;
}

/******************************************************************************
** Function name:		CAN_SetACCF
**
** Descriptions:		Set acceptance filter and SRAM associated with
**
** parameters:			ACMF mode
** Returned value:		None
**
**
******************************************************************************/
void CAN_SetACCF( uint32_t ACCFMode )
{
  switch ( ACCFMode )
  {
	case ACCF_OFF:
	  LPC_CANAF->AFMR = ACCFMode;
	  LPC_CAN1->MOD = 1;	// Reset CAN
	  LPC_CAN1->IER = 0;	// Disable Receive Interrupt
	  LPC_CAN1->GSR = 0;	// Reset error counter when CANxMOD is in reset
	break;

	case ACCF_BYPASS:
	  LPC_CANAF->AFMR = ACCFMode;
	break;

	case ACCF_ON:
	case ACCF_FULLCAN:
	  LPC_CANAF->AFMR = ACCF_OFF;
	  CAN_SetACCF_Lookup();
	  LPC_CANAF->AFMR = ACCFMode;
	break;

	default:
	break;
  }
  return;
}

static int intrans = 0;
/******************************************************************************
** Function name:		CAN1_SendMessage
**
** Descriptions:		Send message block to CAN1
**
** parameters:			pointer to the CAN message
** Returned value:		true or false, if message buffer is available,
**						message can be sent successfully, return TRUE,
**						otherwise, return FALSE.
**
******************************************************************************/
uint32_t CAN1_SendMessage( CAN_MSG *pTxBuf )
{
  uint32_t CANStatus;

  if (!intrans)
  {
//	  intrans = 1;
	  CAN1TxCount++;
	  CANStatus = LPC_CAN1->SR;
	  if ( CANStatus & 0x00000004 )
	  {
		LPC_CAN1->TFI1 = pTxBuf->Frame & 0xC00F0000;
		LPC_CAN1->TID1 = pTxBuf->MsgID;
		LPC_CAN1->TDA1 = pTxBuf->DataA;
		LPC_CAN1->TDB1 = pTxBuf->DataB;
		LPC_CAN1->CMR = 0x21;
		intrans = 0;
		return ( 1 );
	  }
	  else if ( CANStatus & 0x00000400 )
	  {
		LPC_CAN1->TFI2 = pTxBuf->Frame & 0xC00F0000;
		LPC_CAN1->TID2 = pTxBuf->MsgID;
		LPC_CAN1->TDA2 = pTxBuf->DataA;
		LPC_CAN1->TDB2 = pTxBuf->DataB;
		LPC_CAN1->CMR = 0x41;
		intrans = 0;
		return ( 1 );
	  }
	  else if ( CANStatus & 0x00040000 )
	  {
		LPC_CAN1->TFI3 = pTxBuf->Frame & 0xC00F0000;
		LPC_CAN1->TID3 = pTxBuf->MsgID;
		LPC_CAN1->TDA3 = pTxBuf->DataA;
		LPC_CAN1->TDB3 = pTxBuf->DataB;
		LPC_CAN1->CMR = 0x81;
		intrans = 0;
		return ( 1 );
	  }
      intrans = 0;
  }
  return ( 0 );
}



