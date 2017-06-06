/*****************************************************************************
 * $Id: mc_math.h 6577 2011-06-06 12:00:00Z nxp16893 $
 *
 * Project: MC_lib
 *
 * Description: Motor Control math libraries
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
#ifndef __MC_MATH_H
#define	__MC_MATH_H

#include "mc_type.h"

#define SQRT3			1.73205		// sqrt(3) 
#define SQRT3DIV2		0.866		// sqrt(3)/2 
#define INV_SQRT3		0.57735  	// 1/sqrt(3) 

#define RPM_TO_RADSEC(x) (x/9.42477796)

int32_t sqrt_F22_10(int32_t value);

#endif /* end __MC_MATH_H */

/*****************************************************************************
**                            End Of File
******************************************************************************/
