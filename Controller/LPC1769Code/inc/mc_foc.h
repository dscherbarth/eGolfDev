/*****************************************************************************
 * $Id: mc_foc.h 6577 2011-06-06 12:00:00Z nxp16893 $
 *
 * Project: MC_lib
 *
 * Description: Field Oriented Motor Control library
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
#ifndef __MC_FOC_H
#define	__MC_FOC_H

#include "mc_type.h"

#define i32RPM_TO_F22_10RADS(n)	(n*(2.0*PI*1024)/60)


/* Max RPM = 8192 = 2^13 */
/* Setpoint is fixed point with a max full value of 32768 */
/* Resolution = 8192 / 32768 = 0.25 RPM */
/* Speed setpoint 'full' value for  must therefore be RPMx4 */
#define i32RPM_TO_F6_10PID(n) (n<<2)

/* Sensorless algorithm parameters */
#define RPM0 500
#define RPM1 1000
#define RPM2 1500
#define RPM3 2000
#define RPM4 2500
#define RPM5 3000
#define RPM6 3500
#define RPM7 4000
#define RPM8 4500
#define RPM9 5000

#define OFFSET0	1.47	
#define OFFSET1	1.57	
#define OFFSET2	1.61	
#define OFFSET3	1.63
#define OFFSET4	1.64	
#define OFFSET5	1.65	
#define OFFSET6	1.66	
#define OFFSET7	1.67	
#define OFFSET8	1.67	
#define OFFSET9	1.67

#define OMEGA0	(float) 2.0*PI*RPM0*SENSORLESS_SPEEDLOOPTIME/60.0 
#define OMEGA1	(float) 2.0*PI*RPM1*SENSORLESS_SPEEDLOOPTIME/60.0
#define OMEGA2	(float) 2.0*PI*RPM2*SENSORLESS_SPEEDLOOPTIME/60.0
#define OMEGA3	(float) 2.0*PI*RPM3*SENSORLESS_SPEEDLOOPTIME/60.0
#define OMEGA4	(float) 2.0*PI*RPM4*SENSORLESS_SPEEDLOOPTIME/60.0
#define OMEGA5	(float) 2.0*PI*RPM5*SENSORLESS_SPEEDLOOPTIME/60.0
#define OMEGA6	(float) 2.0*PI*RPM6*SENSORLESS_SPEEDLOOPTIME/60.0
#define OMEGA7	(float) 2.0*PI*RPM7*SENSORLESS_SPEEDLOOPTIME/60.0
#define OMEGA8	(float) 2.0*PI*RPM8*SENSORLESS_SPEEDLOOPTIME/60.0
#define OMEGA9	(float) 2.0*PI*RPM9*SENSORLESS_SPEEDLOOPTIME/60.0

#define SLOPE0 (float)((OFFSET1 - OFFSET0) / (OMEGA1 -OMEGA0))
#define SLOPE1 (float)((OFFSET2 - OFFSET1) / (OMEGA2 -OMEGA1))
#define SLOPE2 (float)((OFFSET3 - OFFSET2) / (OMEGA3 -OMEGA2))
#define SLOPE3 (float)((OFFSET4 - OFFSET3) / (OMEGA4 -OMEGA3))
#define SLOPE4 (float)((OFFSET5 - OFFSET4) / (OMEGA5 -OMEGA4))
#define SLOPE5 (float)((OFFSET6 - OFFSET5) / (OMEGA6 -OMEGA5))
#define SLOPE6 (float)((OFFSET7 - OFFSET6) / (OMEGA7 -OMEGA6))
#define SLOPE7 (float)((OFFSET8 - OFFSET7) / (OMEGA8 -OMEGA7))
#define SLOPE8 (float)((OFFSET9 - OFFSET8) / (OMEGA9 -OMEGA8))

void vMC_FOC_SetDefault(FOC_STORAGE_Type *storedFoc);
void vMC_FOC_ImportStorage(FOC_STORAGE_Type *storedFoc, FOC_Type *foc);
void vMC_FOC_Init(void);
void vMC_FOC_Loop(void);
void f6_10MC_Commutate(int16_t duty, int16_t alpha);
MC_DATA_Type * focGetMC (void);
void focUpdateTune (int value, int param);
void focpwmDisable( void );
void focpwmEnable( void );
void focSPset (uint32_t SPset);
void focQset(float Qset );



#endif /* end __MC_FOC_H */

/*****************************************************************************
**                            End Of File
******************************************************************************/
