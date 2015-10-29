/*****************************************************************************
 * $Id: mc_lib.c 6577 2011-06-06 12:00:00Z nxp16893 $
 *
 * Project: MC_lib
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
#include <stdint.h>
#include "mc_lib.h"
#include "mc_inc.h"
#include "app_setup.h"
#include "mcpwm.h"
#include "waveform.h"

MCPWM_CHANNEL_SETUP_T channelsetup[3];
uint8_t size;

uint8_t vMC_LIB_GetMCType(void)
{
#if defined MCLIB_TYPE_BLDC
	return 0;
#elif defined MCLIB_TYPE_STEPPER
	return 1;
#elif defined MCLIB_TYPE_SVPWM
	return 2;
#elif defined MCLIB_TYPE_FOC_SYNC
	return 3;
#elif defined MCLIB_TYPE_FOC_ASYNC
	return 4;
#endif
};

void vMC_LIB_PID_Reset(F6_10_PID_Type *pid)
{
	pid->input_prev.full = F6_10_CONST(0.0); 	// Previous input
	pid->err_sum.full = F6_10_CONST(0.0);		// Integrated error * ki
	pid->d_input.full = F6_10_CONST(0.0); 		// Derivative on measure
	
	/* Calculate new max value for err_sum */
	if(pid->ki.full>0)pid->err_sum_max.full = DIV_F22_10(pid->out_max, pid->ki);
}

void vMC_LIB_PID_F6_10_Copy(F6_10_PID_Type *pid_src, F6_10_PID_Type *pid_dst)
{
	uint8_t n;

	size = sizeof(*pid_src);

	for(n=0;n<size;n++)
	{
		((uint8_t *)pid_dst)[n] = ((uint8_t *)pid_src)[n];
	}
}

void f6_10MC_LIB_PIDSetKi(int16_t ki, F6_10_PID_Type *pid)
{
	/* err = err * (old_i / new_i) */
	
	F22_10 scale;
	F22_10 oldKi;
	F22_10 newKi;

	// ki is in the 0.x range. Since division of a small FP number is not accurate,
	// scale ki before division
	if(ki>0)
	{
		oldKi.full = (int32_t)pid->ki.full << 16; 
		newKi.full = (int32_t)ki << 16; 
		scale.full = DIV_F22_10(oldKi, newKi);	
	}
	else
	{
		scale.full = F22_10_CONST(0.0);	
	}

	pid->err_sum.full = MULT_F22_10(pid->err_sum, scale);
	
	//if(pid->err_sum.full>0)pid->err_sum.full = MULT_F22_10(pid->err_sum, pid->ki);
	//if(ki>0)pid->err_sum.full = DIV_F22_10s(pid->err_sum, ki);	 
	
	pid->ki.full = ki; 
	
	/* Calculate new max value for err_sum */
	if(ki>0)pid->err_sum_max.full = DIV_F22_10(pid->out_max, pid->ki);
}

int16_t i16MC_LIB_PID(int16_t input, F6_10_PID_Type *pid)
{
	int32_t output;
	
	pid->err.full = (int32_t)pid->sp.full - input;
	pid->err_sum.full += pid->err.full;

	/* Limit err_sum */
//	if(pid->err_sum.full > pid->err_sum_max.full)pid->err_sum.full = pid->err_sum_max.full;
//	if(pid->err_sum.full < -pid->err_sum_max.full)pid->err_sum.full = -pid->err_sum_max.full;
		
//	/* Compensate for derrivative kick by using dInput instead of dError */
//	pid->d_input.full = input - pid->input_prev.full;

	output = (MULT_F22_10(pid->err, pid->kp) * 1000) +
			 MULT_F22_10(pid->err_sum, pid->ki);		// the ki term is deprecated by 10000 because we are executing at 10Khz???
//			 MULT_F22_10(pid->d_input, pid->kd);
	output /= 1000;	// normalize

	if(output > pid->out_max.full)
	{
		/* Anti wind-up: undo integration if output>max */
		pid->err_sum.full -= (int32_t)pid->err.full;
		/* Clamp output */
		output = pid->out_max.full;
	} 
	if(output < pid->out_min.full)
	{
		/* Anti wind-up: undo integration if output=max */
		pid->err_sum.full -= (int32_t)pid->err.full;
		/* Clamp output */
		output = pid->out_min.full;	
	}	  

//	/* Save input */
//	pid->input_prev.full = input;
	
	return (int16_t)output;
}


int16_t f6_10MC_LIB_ScaledADC(uint16_t adcval, uint16_t offset, F6_10 gain)
{
	F6_10 fpTmp;
	fpTmp.full = (int16_t)adcval - offset;
	return MULT_F6_10(fpTmp, gain); 
}
#define QEI_MAXPOS 4096
int16_t f6_10MC_LIB_QEIToRad(CALIB_Type *calib, uint16_t pos)
{
	uint32_t tmpPos;
	tmpPos = pos;	
//	if(tmpPos<calib->QEIoffset)tmpPos+=QEI_MAXPOS;		// If value<offset then add ticks
//	tmpPos-=calib->QEIoffset;							// Correct value with offset

#if POLEPAIRS>1  
#define POLEPAIRS_MOD (QEI_MAXPOS/POLEPAIRS)
	tmpPos = (tmpPos%POLEPAIRS_MOD)*POLEPAIRS;
#endif
	// From QEI ticks to 2PI radians
	// 2*PI = 6.283185307 = FP full value 6434
	#define QEI_SCALE ((F6_10_CONST(2*PI)<<16)/(QEI_MAXPOS+1))
	return (int16_t)((tmpPos * QEI_SCALE) >> 16);
}

uint8_t u8MC_LIB_GetSector(CALIB_Type *calib, SENS_Type type)
{
 	switch(type)
	{
		case(SENS_HALL):
			return 0;
		case(SENS_QEI):
			return 0;
		case(SENS_FLUX):
			return 0;
		case(SENS_SENSORLESS):
			return 0;
		default:
			return 0;
	}
}

/*****************************************************************************
** Function name:		s16MC_clamp
**
** Description:			clamp a variable to a limit
**
** Parameters:			variable and limit
** Returned value:		clamped variable 
**
*****************************************************************************/
int16_t s16MC_clamp(int32_t sl, int16_t clamp)
{
 	if(sl>clamp)return clamp;
	if(sl<-clamp)return -clamp;
	return (int16_t)sl;
}

/*****************************************************************************
** Function name:		s32MC_clamp
**
** Description:			clamp a variable to a limit
**
** Parameters:			variable and limit
** Returned value:		clamped variable 
**
*****************************************************************************/
int32_t s32MC_clamp(int32_t sl, int32_t clamp)
{
	if(sl>clamp)return clamp;
	if(sl<-clamp)return -clamp;
	return sl;
}

uint16_t AH, BH, CH;

/******************************************************************************
** Function name:		vSCT_SetPWM_F6_10
**
** Descriptions:		PWM match activate registers setup 
**
** Parameters:			pwm struct
** Returned value:		None
** 
******************************************************************************/
void vMC_LIB_SetPWM_F6_10(F6_10_PWM_Type *p)
{

	AH = ((PWM_CYCLE) * ((int32_t)(((p->A.full-512)<<14)/1024))>>14) + PWM_CYCLE/2;
	BH = ((PWM_CYCLE) * ((int32_t)(((p->B.full-512)<<14)/1024))>>14) + PWM_CYCLE/2;
	CH = ((PWM_CYCLE) * ((int32_t)(((p->C.full-512)<<14)/1024))>>14) + PWM_CYCLE/2;

	// save outputs

	// load values and write
	set_mcpwm (AH, BH, CH);

}

// void calcWindDown (steps)
// delta (cvalue - 512)/steps for each channel
// int (done) exeWindDownStep (step)
void vMC_LIB_SetPWMFreeRun(void)
{
}

void vMC_LIB_Disable(MC_DATA_Type *mc)
{
#if defined MCLIB_TYPE_BLDC
	/* tbd */
#elif defined MCLIB_TYPE_STEPPER
	/* tbd */
#elif defined MCLIB_TYPE_SVPWM
	/* tbd */
#elif defined MCLIB_TYPE_FOC_SYNC
	mc->ctrl.enabled = FALSE;
	vMC_LIB_SetPWMFreeRun();
#elif defined MCLIB_TYPE_FOC_ASYNC
	/* tbd */
#endif
}

void vMC_LIB_Enable(MC_DATA_Type *mc)			 	
{
#if defined MCLIB_TYPE_BLDC
	/* tbd */
#elif defined MCLIB_TYPE_STEPPER
	/* tbd */
#elif defined MCLIB_TYPE_SVPWM
	/* tbd */
#elif defined MCLIB_TYPE_FOC_SYNC
	vMC_LIB_PID_Reset(&mc->var.foc.pidD);
	vMC_LIB_PID_Reset(&mc->var.foc.pidQ);
	mc->ctrl.enabled = TRUE;
#elif defined MCLIB_TYPE_FOC_ASYNC
	/* tbd */
#endif	
}

void vMC_LIB_F6_10Filter(F6_10 value, F6_10 *pfltrd, F6_10 c_fltr)
{
	pfltrd->full += MULT_F6_10(c_fltr, value) - MULT_F6_10s(c_fltr, pfltrd->full);
}

void vf6_10MC_LIB_LimitVq(F22_10 Vlim_square, F6_10 *pVq, F6_10 *pVd, F6_10 *pIsum)
{
	F22_10 Vd_square;
	F22_10 Vq_max;

	Vd_square.full = pVd->full;
	Vd_square.full = MULT_F22_10(Vd_square, Vd_square);
	Vq_max.full = Vlim_square.full - Vd_square.full;
	Vq_max.full = sqrt_F22_10(Vq_max.full);

	if(pVq->full > Vq_max.full)pVq->full=(int16_t)Vq_max.full;
	if(pVq->full < -Vq_max.full)pVq->full=(int16_t)-Vq_max.full;
}

void vf6_10MC_LIB_CurrentLimitQsp(F6_10 *psp, F6_10 *pVq, F6_10 *pVd, F6_10 *pIsum)
{
	F6_10 Ierr; 
	F22_10 sp_max;
	
	sp_max.full = abs(psp->full);

	if(pIsum->full > F6_10_CONST(I_CUTOFF) && pIsum->full <  F6_10_CONST(I_MAX))
	{
		Ierr.full = pIsum->full - F6_10_CONST(I_CUTOFF);
		// sp_max = sp_max - (i_err * ( sp_max / (I_MAX-I_CUTOFF) ) );
		sp_max.full -= MULT_F22_10s(Ierr, MULT_F22_10s(sp_max, F22_10_CONST(1.0 / (I_MAX - I_CUTOFF))));
	}
	else if(pIsum->full >= F6_10_CONST(I_MAX))
	{
		sp_max.full = F22_10_CONST(0.0);
	}

	/* Limit *psp to sp_max */
	if(psp->full > sp_max.full)
	{
		psp->full=sp_max.full;
	}
	if(psp->full < -sp_max.full)
	{
		psp->full=-sp_max.full;
	}
}



/*****************************************************************************
**                            End Of File
******************************************************************************/
