/*****************************************************************************
 * $Id: mc_inc.h 6577 2011-06-06 12:00:00Z nxp16893 $
 *
 * Project: MC_lib
 *
 * Description: Motor Control libraries	perohperal includes
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
#ifndef __MC_INC_H
#define	__MC_INC_H

#include <stdint.h>
#include "type.h"

/*****************************************************************************
 * Target selection 
 * 	Select the target MCU 
 *****************************************************************************/
//#define MCLIB_TARGET_LPC11xx 
//#define MCLIB_TARGET_LPC13xx 
#define MCLIB_TARGET_LPC17xx
//#define MCLIB_TARGET_LPC18XX
//#define MCLIB_TARGET_LPC43XX 

/*****************************************************************************
 * Option selection 
 * 	Select compile options 
 *****************************************************************************/
//#define MCLIB_OPT_FOCSENSORLESS
//#define MCLIB_OPT_SINGLESHUNT

/*****************************************************************************
 * Motor control type selection 
 * 	Select compile options 
 *****************************************************************************/
 //#define MCLIB_TYPE_BLDC
//#define MCLIB_TYPE_STEPPER
//#define MCLIB_TYPE_SVPWM
#define MCLIB_TYPE_FOC_SYNC
//#define MCLIB_TYPE_FOC_ASYNC

/*****************************************************************************
 * Motor control library version 
 * 	Revision history:
 * 	v1_0: Beta release. Supports LPC1800 FOC dual shunt only.
 *****************************************************************************/
#define MCLIB_V_MAJOR 1
#define MCLIB_V_MINOR 0

/*****************************************************************************
 * Imported functions 
 *****************************************************************************/
extern void vDelay_Ms(uint32_t ms);

/*****************************************************************************
 * Peripheral includes 
 *****************************************************************************/
#ifdef MCLIB_TARGET_LPC18XX

	/* Include LPC1800 definitions */	
	#include "lpc18xx.h"
	
	/* Include LPC1800 motor control peripheral libraries */
	#include "lpc18xx_qei.h"
	#include "lpc18xx_scu.h"
	#include "lpc18xx_uart.h"
	#include "lpc18xx_adc.h"
	#include "lpc18xx_dma.h"
	#include "sct_user.h"	

#elif MCLIB_TARGET_LPC43XX

	/* Include LPC4300 definitions */	
	#include "lpc43xx.h"

	/* Include LPC4300 motor control peripheral libraries */
	#include "lpc43xx_qei.h"
	#include "lpc43xx_scu.h"
	#include "lpc43xx_uart.h"
	#include "lpc43xx_adc.h"
	#include "lpc43xx_dma.h"

#endif
#endif /* end __MC_INC_H */

/*****************************************************************************
**                            End Of File
******************************************************************************/
