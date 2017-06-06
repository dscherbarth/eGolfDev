/*****************************************************************************
 * $Id: mc_comms.h 6577 2011-06-06 12:00:00Z nxp16893 $
 *
 * Project: MC_lib
 *
 * Description: Motor Control communication libraries
 *
 * Copyright(C) 2011, NXP Semiconductor
 * All rights reserved.
 *
 *****************************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 *****************************************************************************/
#ifndef __MC_COMMS_H
#define	__MC_COMMS_H

#include "mc_type.h"

#define RX_STARTCHAR	1		// Start character for receiving command packets
#define RX_MAX_PACKETS	8
#define RX_MAXBUFFSIZE	32		// Max number of bytes in UART buff
#define TX_LIST_MAXSIZE	0xFF	// Max size of tx list 

#define CMD_ALIVE_FEED  	2	// Connection alive feed
#define CMD_WRITE_BIT		3	// Write a single bit 
#define CMD_WRITE_8			4	// Write an 8bit value
#define CMD_WRITE_16 		5	// Write an 16bit value
#define CMD_WRITE_32		6	// Write an 32bit value
#define CMD_SETTXLIST		7	// Set txlist items
#define CMD_SETTRIGGER		8	// Set trigger 
#define CMD_CTRLEN 			9	// Set MC enabled/disabled
#define CMD_SPDCEN 			10	// Set speed control enabled/disabled
#define CMD_PID_KI			11	// Set a new PID Ki value
#define CMD_SAMPLE_POINT	12	// Set an adc sample point 
#define CMD_QEI_INV     	13	// Set QEI direction 
#define CMD_QEI_AUTOCAL 	14	// Start QEI autocalibration routine

#define TRIGGER_NO_EDGE			0
#define TRIGGER_RISING_EDGE		1		
#define TRIGGER_FALLING_EDGE	2

typedef void (* COMMS_CALLBACKu8)(uint8_t value);
typedef void (* COMMS_CALLBACKu8u16)(uint8_t ch, uint16_t value);

/** @brief COMMS Initialization buffer structure */
typedef struct {
	COMMS_CALLBACKu8 Enable; 			/*!< Pointer to Enable callback function  */					
	COMMS_CALLBACKu8 SpeedEnable; 		/*!< Pointer to SpeedEnable callback function  */					
	COMMS_CALLBACKu8u16	SamplePoint;	/*!< Pointer to SpeedEnable callback function  */					
}COMMS_Callbacks_Type;

typedef struct _RX_PACKET
{
	uint8_t cmd;
	uint8_t data[RX_MAXBUFFSIZE];
}RX_PACKET;

typedef struct _SYNC_Type			// 24 bytes
{
	uint8_t syncVAL[4];				// 0:	sync bytes
	uint16_t mc_lib_version;		// 4: 	MC lib version
	uint16_t total_size;			// 6:	Total size of packet
	uint16_t sync_size;				// 8:	Size of sync structure
	uint16_t data_size;				// 10:	Size of data structure
	uint16_t scope_size;			// 12: 	Size of scope buffer
	uint16_t RESERVED0;				// 14: 	
	uint32_t deviceID[2];			// 16: 	double DWORD DeviceID
}SYNC_Type;

typedef struct _TRIGGER_Type
{
	uint8_t size;				// Number of trigger bytes to compare with
	uint8_t edge;				// Edge type of trigger
	uint8_t address;			// Address in FOC struct of value to be triggered
	uint8_t start;				// Flag if signal condition prior to trigger transsision is met 
	uint8_t done;				// Flag if signal condition after trigger transsision is met 
	uint32_t value;				// DWORD value to trigger to
}TRIGGER_Type;

typedef struct _MC_COMMS_Type {
	uint8_t tx_buf_full:1;		// Flag that indicates whether the transmit buffer is filled
	uint8_t tx_complete:1;		// Flag that indicates whether transmission is complete
	uint8_t empty:6;	
		
	TRIGGER_Type trig;
	RX_PACKET rx_packets[RX_MAX_PACKETS];

	uint16_t scope_list[TX_LIST_MAXSIZE];	// List containing the items to be put into tx_buff
	uint8_t scope_list_cnt;					// Number of items in scope list
	uint16_t scope_list_index;				// Index of the scope buffer

	COMMS_Callbacks_Type callbacks;

	SYNC_Type sync;						// Contains the sync bytes 
	CTRL_Type ctrl;						// Control/status variables
	CALIB_Type calib;					// Calibration parameters
	
#if defined MCLIB_TYPE_BLDC
	uint8_t data[sizeof(BLDC_Type)];	// BLDC specific variables/parameters
#elif defined MCLIB_TYPE_STEPPER			
	/* tbd */
#elif defined MCLIB_TYPE_SVPWM
	/* tbd */
#elif defined MCLIB_TYPE_FOC_SYNC
	uint8_t data[sizeof(FOC_Type)];		// FOC specific variables/parameters
#elif defined MCLIB_TYPE_FOC_ASYNC
	/* tbd */
#endif

	uint8_t scope[];
}MC_COMMS_Type;

void vMC_COMMS_Init(MC_COMMS_Type *comms, MC_DATA_Type *mc, uint32_t timeout);
void vMC_COMMS_TimerFeed(uint32_t dt);
uint8_t u8MC_COMMS_IsConnected(void);
void vMC_COMMS_ConnReset(void);
void vMC_COMMS_CreateTxBuff(MC_DATA_Type *mc);
void vMC_COMMS_ProcessPacket(uint8_t index, MC_DATA_Type *mc);
void vMC_COMMS_ProcessChar(uint8_t ch);
uint8_t u8MC_COMMS_GetRxPacketCnt(void);
void vMC_COMMS_SetNextPacketIndex(void);
uint8_t u8MC_COMMS_GetNextPacketIndex(void);
uint8_t u8MC_COMMS_IsConnected(void);
void vMC_COMMS_AddTxBuffItems(MC_DATA_Type *mc);
void vMC_COMMS_Transmit(MC_DATA_Type *mc);
void vMC_COMMS_SetTransmissionComplete(void);

#endif /* end __MC_COMMS_H */

/*****************************************************************************
**                            End Of File
******************************************************************************/
