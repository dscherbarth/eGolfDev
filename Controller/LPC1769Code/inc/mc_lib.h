/*****************************************************************************
 * $Id: mc_lib.h 6577 2011-06-06 12:00:00Z nxp16893 $
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
#ifndef __MC_LIB_H
#define	__MC_LIB_H

#include "mc_type.h"
#include "mc_math.h"

/* Function prototypes */
void f6_10MC_LIB_PIDSetKi(int16_t ki, F6_10_PID_Type *pid);
void vMC_LIB_F6_10Filter(F6_10 value, F6_10 *pfltrd, F6_10 c_fltr);
void vf6_10MC_LIB_LimitVq(F22_10 Vlim_square, F6_10 *pVq, F6_10 *pVd, F6_10 *pIsum);
void vf6_10MC_LIB_CurrentLimitQsp(F6_10 *psp, F6_10 *pVq, F6_10 *pVd, F6_10 *pIsum);

uint8_t vMC_LIB_GetMCType(void);
void vMC_LIB_PID_Reset(F6_10_PID_Type *pid);
void vMC_LIB_PID_F6_10_Copy(F6_10_PID_Type *pid_src, F6_10_PID_Type *pid_dst);

int16_t i16MC_LIB_PID(int16_t input, F6_10_PID_Type *pid);

int16_t f6_10MC_LIB_ScaledADC(uint16_t adcval, uint16_t offset, F6_10 gain);
int16_t f6_10MC_LIB_QEIToRad(CALIB_Type *calib, uint16_t pos);
uint8_t u8MC_LIB_GetSector(CALIB_Type *calib, SENS_Type type);
int16_t s16MC_clamp(int32_t sl, int16_t clamp);
int32_t s32MC_clamp(int32_t sl, int32_t clamp);
void vMC_LIB_SetPWMFreeRun(void);
void vMC_LIB_SetPWM_F6_10(F6_10_PWM_Type *p);
void vMC_LIB_Disable(MC_DATA_Type *mc);
void vMC_LIB_Enable(MC_DATA_Type *mc);

static __inline void vMC_LIB_F6_10ClarkePark(F6_10 Ia, F6_10 Ib, F6_10 *pId, F6_10 *pIq, F6_10 sinVal, F6_10 cosVal)
{
	int16_t tmp;

	tmp = MULT_F6_10s(Ia,F6_10_CONST(INV_SQRT3)) + (MULT_F6_10s(Ib,F6_10_CONST(INV_SQRT3)) * 2);
	pIq->full = MULT_F6_10(cosVal, Ia) + MULT_F6_10s(sinVal, tmp);
	pId->full = MULT_F6_10(sinVal, Ia) - MULT_F6_10s(cosVal, tmp);
}

/* Static inline functions */
static __inline void vMC_LIB_F6_10Park(F6_10 Ialpha, F6_10 Ibeta, F6_10 *pId, F6_10 *pIq, F6_10 sinVal, F6_10 cosVal)
{
	pIq->full = MULT_F6_10(cosVal, Ibeta) - MULT_F6_10(sinVal, Ialpha);  
	pId->full = MULT_F6_10(cosVal, Ialpha) + MULT_F6_10(sinVal, Ibeta);  
}

static __inline void vMC_LIB_F6_10InversePark(F6_10 Vd, F6_10 Vq, F6_10 *pValpha, F6_10 *pVbeta, F6_10 sinVal, F6_10 cosVal)
{
	pValpha->full = (int16_t)(MULT_F22_10(Vd, cosVal) - MULT_F22_10(Vq, sinVal)); 
	pVbeta->full  = (int16_t)(MULT_F22_10(Vd, sinVal) + MULT_F22_10(Vq, cosVal)); 
}

static __inline void PJPvMC_LIB_F6_10InversePark(F6_10 Vd, F6_10 Vq, F6_10 *pValpha, F6_10 *pVbeta, F6_10 sinVal, F6_10 cosVal)
{
	pValpha->full = (int16_t)(MULT_F22_10(Vd, sinVal) + MULT_F22_10(Vq, cosVal));
	pVbeta->full  = (int16_t)(MULT_F22_10(Vd, cosVal) - MULT_F22_10(Vq, sinVal));
}

static __inline void vMC_LIB_F6_10Clarke(F6_10 Ia, F6_10 Ib, F6_10 *pIalpha, F6_10 *pIbeta)
{
	pIalpha->full = Ia.full;
	pIbeta->full = Ia.full + MULT_F6_10s(Ib,F6_10_CONST(2.0));
	pIbeta->full = MULT_F6_10s((*pIbeta), F6_10_CONST(INV_SQRT3)); 		 	
}

static __inline void vMC_LIB_F6_10InverseClarke(F6_10 Valpha, F6_10 Vbeta, F6_10 *pVa, F6_10 *pVb, F6_10 *pVc)
{
	int16_t tmp;	

	pVa->full = Vbeta.full;

	pVb->full = -DIV_F6_10s(Vbeta, F6_10_CONST(2.0));
	pVc->full = pVb->full;
	tmp = MULT_F6_10s(Valpha, F6_10_CONST(SQRT3DIV2)); 	
	pVb->full += tmp;
	pVc->full -= tmp;  
}

static __inline void PJPvMC_LIB_F6_10InverseClarke(F6_10 Valpha, F6_10 Vbeta, F6_10 *pVa, F6_10 *pVb, F6_10 *pVc)
{
	int16_t tmp;

	pVa->full = Valpha.full;
	/* pVb = -Vbeta/2 + sqrt(3)/2 * Valpha;
	   pVc = -Vbeta/2 - sqrt(3)/2 * Valpha; */
	pVb->full = -DIV_F6_10s(Valpha, F6_10_CONST(2.0));
	pVc->full = pVb->full;
	tmp = MULT_F6_10s(Vbeta, F6_10_CONST(SQRT3DIV2));
	pVb->full -= tmp;
	pVc->full += tmp;
}

static __inline void vMC_LIB_F6_10SVPWM(F6_10 pVa, F6_10 pVb, F6_10 pVc, F6_10_PWM_Type *p_pwm, uint8_t *sector)
{	
	if(pVa.full >= F6_10_CONST(0.0))
	{
		if(pVb.full >= F6_10_CONST(0.0))		// Sector 0: 0-60 degrees
		{	
			*sector = 0;	
			p_pwm->C.full = (F6_10_CONST(1.0)-pVa.full-pVb.full)>>1;   // Divide by 2 equals >> 1	
			p_pwm->B.full = p_pwm->C.full+pVa.full;
			p_pwm->A.full = p_pwm->B.full+pVb.full;
		}
		else
		{
			if(pVc.full >= F6_10_CONST(0.0))	// Sector 2: 120-180 degrees
			{
				*sector = 2;	
				p_pwm->A.full = (F6_10_CONST(1.0)-pVc.full-pVa.full)>>1;
				p_pwm->C.full = p_pwm->A.full+pVc.full;
				p_pwm->B.full = p_pwm->C.full+pVa.full;
			}
			else								// Sector 1: 60-120 degrees
			{
			   	*sector = 1;	
				p_pwm->C.full = (F6_10_CONST(1.0)+pVc.full+pVb.full)>>1;
				p_pwm->A.full = p_pwm->C.full-pVc.full;
				p_pwm->B.full = p_pwm->A.full-pVb.full;
			}
		}
	}
	else
	{	
		if(pVb.full >= F6_10_CONST(0.0) )
		{
			if(pVc.full >= F6_10_CONST(0.0))	// Sector 4: 240-300 degrees
			{
				*sector = 4;				
				p_pwm->B.full = (F6_10_CONST(1.0)-pVb.full-pVc.full)>>1; 
				p_pwm->A.full = p_pwm->B.full+pVb.full;	  
				p_pwm->C.full = p_pwm->A.full+pVc.full;
			}
			else								// Sector 5: 300-0 degrees
			{
				*sector = 5;	
				p_pwm->B.full = (F6_10_CONST(1.0)+pVa.full+pVc.full)>>1;
				p_pwm->C.full = p_pwm->B.full-pVa.full;
				p_pwm->A.full = p_pwm->C.full-pVc.full;
			}
		}
		else		 							// Sector 3: 180-240 degrees
		{
			*sector = 3;
			p_pwm->A.full = (F6_10_CONST(1.0)+pVa.full+pVb.full)>>1;
			p_pwm->B.full = p_pwm->A.full-pVb.full;
			p_pwm->C.full = p_pwm->B.full-pVa.full;
		}
	}
}

static __inline int16_t f6_10MC_LIB_PID(F6_10 input, F6_10_PID_Type *pid)
{
	return i16MC_LIB_PID(input.full, pid);
}

#endif /* end __MC_LIB_H */

/*****************************************************************************
**                            End Of File
******************************************************************************/
