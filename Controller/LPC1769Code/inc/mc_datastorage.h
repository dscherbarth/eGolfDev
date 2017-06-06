/*****************************************************************************
 * $Id: mc_datastorage.h 6577 2011-06-06 12:00:00Z nxp16893 $
 *
 * Project: MC_lib
 *
 * Description: Brushless DC Motor Control libraries
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
#ifndef __MC_DATASTORAGE_H
#define	__MC_DATASTORAGE_H

#include "mc_type.h"

typedef struct _STORAGE_Type 
{
	uint8_t saved;				// Indicates wheter values have been saved in EEPROM
	uint16_t size;				// Size of this structure

	CTRL_Type ctrl;				// Control variables
	CALIB_Type calib;			// Sensor calibration values

	/* Motor Control type specific variables */
	union _var
	{
		uint8_t vars[1];
		BLDC_STORAGE_Type bldc;
		FOC_STORAGE_Type foc;	
	}var;
}MC_STORAGE_Type;

uint8_t u8MS_DS_LoadParam(MC_STORAGE_Type *stored);
void vMC_DS_ImportStorage(MC_STORAGE_Type *stored, MC_DATA_Type *mc);

#endif /* end __MC_DATASTORAGE_H */

/*****************************************************************************
**                            End Of File
******************************************************************************/
