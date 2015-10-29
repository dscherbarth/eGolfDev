/*****************************************************************************
 * $Id: app_setup.h 6577 2011-06-06 12:00:00Z nxp16893 $
 *
 * Project: Dual shunt FOC
 *
 * Description: Motor Control libraries
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
 #ifndef __APP_H__
#define __APP_H__

#include <stdio.h>
#include <math.h>
//#include "LPC18xx.h"
#include "type.h"

/* Include CMSIS DSPlib */
#include "arm_math.h"

/* Include Motor Control libraries */
#include "mc_inc.h"		

#if MCLIB_V_MAJOR != 1 || MCLIB_V_MINOR !=0
#error Application developped for MCLIB v0_1 
#endif

#include "mc_type.h"			
//#include "mc_bldc.h"
#include "mc_foc.h"		   		
#include "mc_comms.h"			
#include "mc_datastorage.h"		

#include "mc_lib.h"		   		
#include "mc_math.h"			
#include "mc_f6_10.h"			
#include "mc_calib.h"			

/* Include DSP library */
//#include "cr_dsplib.h"

/* Include Peripheral libraries */
//#include "lpc18xx_qei.h"
//#include "lpc18xx_scu.h"
//#include "lpc18xx_uart.h"
//#include "lpc18xx_adc.h"
//#include "lpc18xx_dma.h"

/* Include SCT files */
//#include "sct_fsm.h"
//#include "sct_user.h"

/*****************************************************************************
 * Board specific defines 
 *****************************************************************************/
#define SW2	!((LPC_GPIO3->PIN>>6)&1)			// P6.10
#define SW3	!((LPC_GPIO2->PIN>>0)&1)			// P4.0

/*****************************************************************************
 * User definable parameters 
 *****************************************************************************/
#define POLEPAIRS			2
//#define PWM_FREQUENCY		10000 				// PWM frequency [Hz]
//#define PWM_CYCLE			900					// period of pwm
#define PWM_FREQUENCY		8000 				// PWM frequency [Hz]
#define PWM_CYCLE			1125					// period of pwm
#define PWM_ADC_POINT		2933  				// 60 : 0.5 us  ADC 1 clk = 0.22 us
#define PWM_DEADTIME		20					// 250 nss : 20kHz PWM	PWM_DEADTIME=(-1.666667*PWM_CYCLE*PWM_CYCLE*us+30000*PWM_CYCLE*us)/1000000
#define PWM_MAX				0.96				// % max PWM 

#define QEI_COUNTS_PER_TURN	4096 				// Number of QEI counts per turn
#define QEI_USE_INDEX		FALSE
#define QEI_OFFSET			0
#define QEI_INVERT			0
#define QEI_CALIB_DUTY		F6_10_CONST(0.05)	// 5% dutycycle
#define QEI_CALIB_DELAY 	3000				// msec to wait for steady calibration value

#define SENSORLESS_SPEEDLOOPFREQ	1000 		// Speed loop frequency [Hz]

#define CONNECTION_TIMEOUT	1500 				// [msec] Gets a feed from GUI every sec.

#define STARTUP_TORQUE	F6_10_CONST(25.0)
#define I_MAX 25.0	 							// Maximum current limit
#define I_CUTOFF 24.0 							// Cutoff for current limit
#define C_FLT_IBUS 0.1 			  				// Filter coefficient for IBUS
#define I_BUS_OFFSET 3							// Ibus ADC offset
#define I_BUS_GAIN F6_10_CONST(16.3);			// Ibus gain
#define V_BUS_OFFSET 0							// Vbus ADC offset
#define V_BUS_GAIN F6_10_CONST(1.0)				// Vbus gain

#define K_SMC			0.85					// Slide Mode Controller gain 
#define SMC_MAX_ERR    	0.005					// Maximum error of SMC 
																 
#define PHASE_RESISTANCE	((float)0.153)		// Phase resistance in Ohms
#define PHASE_INDUCTANCE	((float)0.0001658) 	// Phase inductance in Henrys.

																					 
// To determine the current offset and scale an external 
// sourcemeter is used, and the adc value's are read in 
// the scope window of the MC GUI.
// Average adc value 569 = 0.0 A
// Average adc value 801 = 2.19 A
// This corresponds to 9.5mA per adc tick
// The scale factor thus becomes: 1024 * 9.5mA = 9,7
#define ADC_PHA_OFFSET 514 
#define ADC_PHB_OFFSET 514 
#define ADC_PHC_OFFSET 515 

#define ADC_PHASE_GAIN	F6_10_CONST(9.7)

#define ADC_PHA_PORT	0
#define ADC_PHA_CH		3
#define ADC_PHB_PORT	0
#define ADC_PHB_CH		4
#define ADC_PHC_PORT	1
#define ADC_PHC_CH 		5
#define ADC_IBUS_PORT	1
#define ADC_IBUS_CH 	6

extern void vAppInit(void);
extern void vAppSetDefault(MC_STORAGE_Type *stored);
extern void vAppResetCurrTrip(void);


#endif /* end __APP_H */

/*****************************************************************************
**                            End Of File
******************************************************************************/


