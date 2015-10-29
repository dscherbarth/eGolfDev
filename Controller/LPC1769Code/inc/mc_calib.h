/*****************************************************************************
 * $Id: mc_calib.h 6577 2011-06-06 12:00:00Z nxp16893 $
 *
 * Project: MC_lib
 *
 * Description: Motor Control calibration libraries
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
#ifndef __MC_CALIB_H
#define	__MC_CALIB_H

void vCALIB_InitQEICalib(F6_10 duty, uint32_t delay);
void vCALIB_QEICalib(CALIB_Type *calib);

#endif /* end __MC_CALIB_H */

/*****************************************************************************
**                            End Of File
******************************************************************************/
