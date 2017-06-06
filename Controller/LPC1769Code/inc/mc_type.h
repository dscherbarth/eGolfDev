/*****************************************************************************
 * $Id: mc_type.h 6577 2011-06-06 12:00:00Z nxp16893 $
 *
 * Project: MC_lib
 *
 * Description: Motor Control libraries	type definitions
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
#ifndef __MC_TYPE_H
#define	__MC_TYPE_H

#include <stdint.h>
#include "mc_f6_10.h"					

#ifndef NULL
#define NULL    ((void *)0)
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (1)
#endif

/*****************************************************************************
 * Static MC parameter definitions; do not change 
 *****************************************************************************/
#define MODE_QEI_RAMP	0		// Ramp for QEI operation
#define MODE_SL_RAMP	1		// Ramp for sensorless operation
#define MODE_QEI		2		// QEI operation
#define MODE_HALL		3		// HALL operation
#define MODE_SL			4		// Sensorless operation

#define PWM_MIN_F6_10	F6_10_CONST((1-PWM_MAX))// (1-max)% PWM phase not overlapping = 1 us 														
#define PWM_LOOPTIME		(1.0/PWM_FREQUENCY) 			// PWM Period = 1.0 / PWMFREQUENCY
#define SENSORLESS_SPEEDLOOPTIME 	(1.0/SENSORLESS_SPEEDLOOPFREQ) 		// Speed Control Period
#define PWM_SPEEDRATE (unsigned int)(SENSORLESS_SPEEDLOOPTIME/PWM_LOOPTIME)	// PWM loops per velocity calculation

typedef struct F6_10_PWM
{
	F6_10 A;		// F6_10 pwm duty cycle value normalised to 1
	F6_10 B;
	F6_10 C;
}F6_10_PWM_Type;

typedef struct F6_10_PID 
{
	F6_10 sp;				// Setpoint
	F6_10 kp;				// Proportional component
	F6_10 ki;				// Integral component
	F6_10 kd;				// Derrivative component
	F6_10 input_prev;		// Previous input
	F6_10 err;				// Error
	F22_10 err_sum;			// Integrated error 
	F22_10 err_sum_max;		// Maximum integrated error
	F22_10 d_input; 		// Derivative on measurement: derivated error is equal to negative 
							// derivative of input, except when the setpoint is changing
	F6_10 out_max;			// Maximum output
	F6_10 out_min;			// Minimum output
}F6_10_PID_Type;

typedef enum SENS		
{
	SENS_HALL		= 0,
	SENS_QEI 		= 1,
	SENS_FLUX 		= 2,
	SENS_SENSORLESS = 3,
}SENS_Type;

typedef struct CTRL
{
	/* Generic control/status variables */
	uint8_t type;				// type; BLDC/FOC/SVPWM/Stepper
	uint8_t temp;				// Temperature of driver stage
	uint8_t sector;				// Current SVPWM sector
	uint8_t sensor_mode;		// Currently used sensor for position	
	
	int32_t RPM;				// Rounds per minute
	F6_10_PID_Type pidSpeed;	// PID values for speed
		
	/* Status */
	uint8_t enabled:1;			// bit0: Motor control enabled
	uint8_t speedEna:1;			// bit1: Speed PID enabled
	uint8_t busy:1;				// bit2: Processor not IDLE
	uint8_t brake_thresh:1;		// bit3: Braking threshold reached
	uint8_t brake_forced:1;		// bit4: Braking forced by hardware 
	uint8_t curr_trip:1;		// bit5: Current trip
	uint8_t RESERVED0:2;		// bit6..7
	uint8_t RESERVED1[3];		// 3 bytes
}CTRL_Type;
	
typedef struct CALIB
{
	/* Sensor calibration values */
	uint16_t IphADCpoint[3];	// Current measurement position
	uint16_t IphADCoffset[3];	// Offset for phase current measurement value
	F6_10 IphADCgain[3];		// Gain for phase current measurement value		

	uint16_t IbusADCoffset;		// ADC offset for bus current
	uint16_t VbusADCoffset;		// ADC offset for bus voltage
	F6_10 IbusADCgain;			// Gain for bus current
	F6_10 VbusADCgain;			// Gain for bus voltage

	uint16_t QEIoffset;			// QEI ticks offset
	uint8_t QEIcalib:1;			// bit0: QEI calibrated
	uint8_t QEIauto_complete:1;	// bit1: QEI auto calibration completed
	uint8_t QEIinvert:1;		// bit2: QEI inverted
	uint8_t QEIuseinx:1;		// bit3: use QEI index pulse
	uint8_t RESERVED0:4;		// bit4..7
	uint8_t RESERVED1[3];		// 3 bytes
}CALIB_Type;						

typedef struct FOC 
{
	F6_10_PWM_Type pwm;		// Structure containing start, length and sequence of pwm signals
	uint16_t ADCphase[3];	// ADC phase values
	uint16_t ADCIbus;		// ADC value Ibus
	uint16_t ADCVbus;		// ADC value Ibus
 	F6_10 pwm_brake;		// PWM value for braking
	F6_10 I[3];				// Reconstructed Ia, Ib & Ic
	F6_10 Ibus;				// Bus current 	
	F6_10 Vbus;				// Bus voltage
 	uint16_t QEIpos;		// QEI position
	F6_10 Ialpha;			// Calculated Ia in dynamic reference frame
	F6_10 Ibeta;			// Calculated Ib in dynamic reference frame
	F6_10 Id;				// Calculated Id in static reference frame
	F6_10 Iq;				// Calculated Iq in static reference frame
	F6_10 Vd;				// Regulated PI output d in static reference frame
	F6_10 Vq;				// Regulated PI output q in static reference frame
	F6_10 Valpha;			// Regulated PI output alpha in dynamic reference frame
	F6_10 Vbeta;			// Regulated PI output beta in dynamic reference frame
	F6_10 Vr1;				// Vref1 from inverse park
	F6_10 Vr2;				// Vref2 from inverse park
	F6_10 Vr3;				// Vref3 from inverse park
	F6_10_PID_Type pidQ;		// PID values for Q component
	F6_10_PID_Type pidD;		// PID values for D component
	F6_10 alphaSens;  			// Sensor angle
	F6_10 alphaEst;				// Estimated angle
	F22_10 omega;				// Motor speed in rad/sec. 1 rad/sec = 9.549 rpm 
								// Range = +/- 32768 * 9.549 rpm.
								// Resolution = 9.549 / 2^10 = 0,0093 rpm
	F16_16  F16_16_Valpha;   	// Voltage on alpha axis
	F16_16  F16_16_Vbeta;   	// Input: Stationary beta-axis stator voltage 
	F16_16  F16_16_Ialpha; 		// Current on alpha axis 
	F16_16  F16_16_Ibeta;  		// Current on beta axis  
	F16_16  bemf_alpha;   		// BEMF voltage on alpha axis 
	F16_16  bemf_alpha_flt;		// Filtered BEMF voltage on alpha axis
	F16_16  bemf_beta;  		// BEMF voltage on beta axis 
	F16_16  bemf_beta_flt;		// Filtered BEMF voltage on beta axis 
	F16_16  z_alpha;      		// SMC correction factor z on alpha axis 
	F16_16  z_beta;      		// SMC correction factor z on beta axis
	F16_16  Ialpha_est;   		// Estimated current on alpha axis
	F16_16  Ibeta_est;    		// Estimated current on beta axis 
	F16_16  Ialpha_err; 		// Current estimation error on alpha axis
	F16_16  Ibeta_err;  		// Current estimation error on beta axis               
	F16_16  K_smc;     			// Sliding Mode Controller gain 
	F16_16  smc_max_err; 		// Maximum error of SMC 
	F16_16  G;    				// Virtual motor algorithm constant
	F16_16  F;    				// Virtual motor algorithm constant
	F16_16  Cflt_smc;       	// Slide mode controller filter coefficientficient 
	F16_16  Cflt_omega;  		// Filter coefficientficient for Omega filtered calc
	F16_16  omega_est; 			// Estimated rotor speed
	F16_16  omega_flt; 			// Filtered estimated rotor speed
	F16_16  theta;				// Estimated rotor angle 
	F16_16  theta_offset;		// Theta compensation 
}FOC_Type;

typedef struct FOC_STORAGE	 	// Variables of FOC that are saved in datastorage
{
	F6_10_PID_Type pidQ;		// PID values for Q component
	F6_10_PID_Type pidD;		// PID values for D component
	F6_10_PID_Type pidSpeed;	// PID values for speed
	   	
	F16_16 G;    				// Virtual motor algorithm constant
	F16_16 F;    				// Virtual motor algorithm constant
	F16_16 K_smc;     			// Sliding control gain 
	F16_16 smc_max_err; 		// Maximum current error for linear SMC 
	F16_16 Cflt_omega;  		// Filter coefficient for Omega  
}FOC_STORAGE_Type;

typedef struct BLDC 
{
	uint32_t empty;
}BLDC_Type;

typedef struct BLDC_STORAGE 
{
	uint32_t empty;
}BLDC_STORAGE_Type;

typedef struct MC_DATA
{
	uint16_t size;			// Size of this structure

	CTRL_Type ctrl;			// Control variables
	CALIB_Type calib;		// Sensor calibration values

	/* Motor Control type specific variables */
	union varTag
	{
		uint8_t vars[1];
		BLDC_Type bldc;
		FOC_Type foc;	
	}var;
}MC_DATA_Type;

#define abs(value)	(value<0 ? -value : value)

#endif /* end __MC_TYPE_H */

/*****************************************************************************
**                            End Of File
******************************************************************************/
