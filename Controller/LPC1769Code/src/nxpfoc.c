/*****************************************************************************
 * $Id: mc_foc->c 6577 2011-06-06 12:00:00Z nxp16893 $
 *
 * Project: MC_lib
 *
 * Description:  Field Oriented Motor Control library
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
#include "stdint.h"
#include "app_setup.h"
#include "mc_foc.h"
#include "adc.h"
#include "qei.h"
#include "gpio.h"
#include "fault.h"
#include "status.h"
#include "snapshot.h"
#include "waveform.h"
#include "control.h"
#include "command.h"
#include "Sensors.h"
#include "models.h"
#include "timer.h"


extern uint32_t	dacRValue;

MC_DATA_Type mc;					// Definition of structure that holds all


MC_DATA_Type* pmc;			// Local copy of pointer to global MC data structure
FOC_Type* pfoc;				// Local copy of pointer to global FOC data structure

F6_10	polespi;
F6_10	fsconst;
uint32_t	cpr = 0;	// counts per rev
float flconst, fltemp;

/*****************************************************************************
** Function name:		FOC_Init
**
** Description:			Initialize FOC parameters to default
**
** Parameters:			None
** Returned value:		None
*****************************************************************************/
void vMC_FOC_Init(void)
{
	float	p, i, d;


	pmc = &mc;								// Save pointer to global MC data
	pfoc = &mc.var.foc;					// Save pointer to global FOC data

	/* Initialize peripherals */
	pmc->ctrl.enabled = FALSE;

	/* Initialize FOC PID values */
//	pfoc->pidQ.out_max.full = INT16_MAX;
//	pfoc->pidQ.out_min.full = INT16_MIN;
	pfoc->pidQ.out_max.full = 2000;		// based on postgain of 2
	pfoc->pidQ.out_min.full = -2000;
	vMC_LIB_PID_Reset(&pfoc->pidQ);

//	pfoc->pidD.out_max.full = INT16_MAX;
//	pfoc->pidD.out_min.full = INT16_MIN;
	pfoc->pidD.out_max.full = 2000;
	pfoc->pidD.out_min.full = -2000;
	vMC_LIB_PID_Reset(&pfoc->pidD);

	MgetQPID (&p, &i, &d);			// init from the model
	pmc->var.foc.pidQ.kp.full = F6_10_CONST(p);
	f6_10MC_LIB_PIDSetKi(F6_10_CONST(i), &pmc->var.foc.pidQ);
	pmc->var.foc.pidQ.kd.full = F6_10_CONST(d);

	MgetDPID (&p, &i, &d);			// init from the model
	pmc->var.foc.pidD.kp.full = F6_10_CONST(p);
	f6_10MC_LIB_PIDSetKi(F6_10_CONST(i), &pmc->var.foc.pidD);
	pmc->var.foc.pidD.kd.full = F6_10_CONST(d);

	// reset the position counter
	LPC_QEI->CON = 0x1;

}

int	QandDinited = 0;
static F6_10	Slipalpha;
void focpwmDisable( void )
{


	if (pmc->ctrl.enabled)
	{
		// pwm outputs to zero
		set_mcpwm(0, 0, 0);

		// re-do the foc params
		QandDinited = 0;

		// stop the foc pwm writes
		pmc->ctrl.enabled = 0;

	}
}

void focpwmFaultDisable( void )
{

	// outputs off
	set_mcpwm(0, 0, 0);

	// re-do the foc params
	QandDinited = 0;

	// stop the foc pwm writes
	pmc->ctrl.enabled = 0;
}

F6_10 sinAlpha, cosAlpha;	// Sine and cosine component of alpha
F6_10	Ia, Ib, Ic;
F6_10	vr1, vr2, vr3;

extern int32_t reflect;
extern int32_t dacaten;

uint32_t	zpA, zpC;
uint32_t	deltapos, currpos, lastpos;
void focpwmEnable( void )
{
	int i;

	if (!pmc->ctrl.enabled)
	{
		// ramp up the outputs (this is tied to the current pwm 900/10kHz cfg)
		for (i=0; i<(PWM_CYCLE/2); i++)
		{
			set_mcpwm(i, i, i);
			delayMicros(0, 200, NULL );
		}	// total .225 seconds

		// get zero current values
		zpA = zpC = 0;
		for (i=0; i<128; i++)
		{
			delayMicros(0, 500, NULL );
			zpA += getADC (PHASEAINDEX);
			zpC += getADC (PHASECINDEX);
		}
		zpA = zpA >> 7;
		zpC = zpC >> 7;

		// re-do the foc params
		QandDinited = 0;

		pmc->ctrl.enabled = 1;
	}
}

F6_10	fs;		// calculated target slip
F6_10	igain, igain2;
int		postgain = 5;
uint32_t qerror = 0, derror = 0;
uint32_t	limitState = 0;
int limiting = 0;
int climiting = 0;

int GetQsetpointLimit (void)
{
	int rpm;

	rpm = qei_read32RPM();

	if (rpm < 100)
	{
		return 3700;
	}
	else if (rpm < 1000)
	{
		return 4000;
	}
	else if (rpm <3000)
	{
		return 4200;
	}
	else
	{
		return 4300;
	}
}

//
// MCEsetQandD	Set torque and f based magnet values and set the slip value
//
//	inputs:
//		Q			torque
//		D			rpm based magnet value
//
//	outputs:
//		nothing
//
void MCEsetQandD (int Q, uint32_t D)
{
	static F6_10	ftemp, QSetPoint, DSetPoint;
	static F6_10	SaveQSetPoint, SaveDSetPoint;
	int Lim;

	float	dif;
	F6_10	pi;

	QSetPoint.full = Q; 	// delivered << by 10 bits
	DSetPoint.full = D; 	// delivered << by 10 bits

	SaveQSetPoint.full = pfoc->pidQ.sp.full;
	SaveDSetPoint.full = pfoc->pidD.sp.full;

	if (!QandDinited)
	{
		cpr = MgetEncCPR ();

		fltemp = (2 * 3.14159265);
		fltemp *= MgetTimeConst();
		flconst = (1.0 / fltemp) / ((POLEPAIRS * PWM_FREQUENCY) / cpr); // 1/((2 * pi) * Tr) / (10000 / 2000)
		fsconst.full = F6_10_CONST(flconst);

		pi.full = F6_10_CONST(3.14159265358979);
		polespi.full = MULT_F6_10s(pi, (1 << 10));

		igain.full = F6_10_CONST(MgetIgain());
		igain2.full = F6_10_CONST(MgetIgain2());
		postgain = MgetPostgain();

		deltapos = 0; currpos = 0; lastpos = 0;
		Slipalpha.full = 0;

		QandDinited = 1;
	}

	// overspeed protection
	limiting = 0;
	if (qei_read32RPM() > MgetRPMLimit ())
	{
		SaveQSetPoint.full = SaveQSetPoint.full - ((qei_read32RPM() - MgetRPMLimit ()) * 50);	// reduce based on error
		if (QSetPoint.full < 0)
			{
			SaveQSetPoint.full = 0;
			}
		limiting = 1;
		limitState = 1;
	}

	// overcurrent protection
	if (getBuscurrent () > MgetBusILimit ())
	{
		dif = (getBuscurrent () - MgetBusILimit ());
		dif *= dif;
		SaveQSetPoint.full = SaveQSetPoint.full - dif;	// reduce based on error ^ 2
		if (QSetPoint.full < 0)
			{
			SaveQSetPoint.full = 0;
			}
		limiting = 1;
		limitState = 2;

	}

	// over temperature
	if (getTemp () > 150)
	{
		dif = (getTemp () - 150) * 10;
		dif *= dif;
		SaveQSetPoint.full = SaveQSetPoint.full - dif;	// reduce based on error ^ 2
		if (QSetPoint.full < 0)
			{
			SaveQSetPoint.full = 0;
			}
		limiting = 1;
		limitState = 3;
	}

	// make sure we have battery headroom if we are regening
	if (QSetPoint.full < 0 && getBusvolt () > 315)
	{
		QSetPoint.full = 0;
	}

	// load up the setpoints
	if (!limiting & !climiting)
	{
		SaveQSetPoint.full = QSetPoint.full;
	}
	SaveDSetPoint.full = DSetPoint.full;

	ftemp.full = DIV_F6_10(SaveQSetPoint, SaveDSetPoint);	// regen will make negative slip
	fs.full = MULT_F6_10(ftemp, fsconst);	// accounts for time constant (tr) and 1/2*PI

//	if (QSetPoint.full < 0)
//		{
//		SaveQSetPoint.full = -QSetPoint.full; // slip might be negative but torque setpoint needs to be pos
//		}

	// scale adjust for Qsp (this really just affects pedal position)
	SaveQSetPoint.full /= 2;	// number might reach 6500 // should be *.289

	// setpoint limit (clipping
	Lim = GetQsetpointLimit ();
	if (SaveQSetPoint.full < 0)
	{
		if (SaveQSetPoint.full < -Lim) SaveQSetPoint.full = -Lim;
	}
	else
	{
		if (SaveQSetPoint.full > Lim) SaveQSetPoint.full = Lim;
	}

	setStatVal (SVPHAC, SaveDSetPoint.full);
	setStatVal (SVPHCC, SaveQSetPoint.full);

	pfoc->pidQ.sp.full = SaveQSetPoint.full;
	pfoc->pidD.sp.full = SaveDSetPoint.full;
}


//
// getSinCos apply slip to current theta and calculate sin and cos
//
F6_10	ongoing_slip;
int		int_slip;
int dir_fact = 1;
void getSinCos (void)
{
	uint32_t	dtptemp;


	currpos = qei_readPos();
	if (qei_readDir () == 0)	// direction is forward (increasing position)
	{
		if (currpos < lastpos)
		{
			deltapos = (currpos + cpr) - lastpos;
		}
		else
		{
			deltapos = currpos - lastpos;
		}
	}
	else						// direction is reverse (decreasing position)
	{
		if (lastpos < currpos)
		{
			deltapos = -((lastpos + cpr) - currpos);
		}
		else
		{
			deltapos = -(lastpos - currpos);
		}
	}
	lastpos = currpos;

	ongoing_slip.full += (dir_fact * fs.full);				// don't lose the fractional part
	int_slip = ongoing_slip.full /1024;
	ongoing_slip.full -= (int_slip * 1024);

	dtptemp = MULT_F6_10s(polespi, deltapos);	// accounts for stator frequency

	// add position to theta
	Slipalpha.full += dtptemp;

	// add slip to theta
	Slipalpha.full += int_slip;

	if (Slipalpha.full < 0)
	{
		Slipalpha.full += 6432;
	}

	Slipalpha.full %= 6432;
	if (Slipalpha.full < 0) Slipalpha.full = 6432;  // back to 2pi at 0 if reversing

	if (dir_fact == 1)
	{
		sinAlpha.full = F6_10_sin(Slipalpha.full);
		cosAlpha.full = F6_10_cos(Slipalpha.full);
	}
	else
	{
		sinAlpha.full = F6_10_sin(6432 - Slipalpha.full);
		cosAlpha.full = F6_10_cos(6432 - Slipalpha.full);
	}
}

//
// toggleDir	// change the direction (caller must make sure this happens at zero rpm)
//
uint32_t	Dir = 1;
void toggleDir (void)
{
	if (Dir == 0)
	{
		Dir = 1;
		dir_fact = 1;
	}
	else
	{
		Dir = 0;
		dir_fact = -1;
	}
}

static int iafnum = 0, ibfnum= 0 , icfnum = 0;
static int ialnum = 0, iblnum= 0 , iclnum = 0;
/*****************************************************************************
** Function name:		CurrentLimit
**
** Description:			Per FOC loop current limit test
**
** Parameters:			None
** Returned value:		None
**
** test measured Ia and Ic and calculated Ib against a hard limit
**
*****************************************************************************/
void CurrentLimit (void)
{
	int ia, ib, ic;
#define CFMAX 1500		// should be 450 Amps
#define CLMAX 1000		// should be 300 Amps
#define CFLTNUM		3 	// number of times this must be true
#define CLIMNUM		1 	// number of times this must be true
	int stopflag = 0;


	ia = getADC (PHASEAINDEX) - zpA;
	ic = getADC (PHASECINDEX) - zpC;
	ib = -(ia + ic);
	if (ia < 0) ia = -ia;
	if (ib < 0) ib = -ib;
	if (ic < 0) ic = -ic;
	if (ia < CFMAX) iafnum = 0;
	if (ib < CFMAX) ibfnum = 0;
	if (ic < CFMAX) icfnum = 0;
	if (ia < CLMAX) ialnum = 0;
	if (ib < CLMAX) iblnum = 0;
	if (ic < CLMAX) iclnum = 0;
	climiting = 0;
	if (pmc->ctrl.enabled)
	{
		// test for setpoint limit action
		if ( ia > CLMAX && ialnum++ > CLIMNUM)
		{
//			climiting = 1;
//			limitState = 5;
//			pfoc->pidQ.sp.full -= (pfoc->pidQ.sp.full >> 3);
		}
		else if ( ib > CLMAX && iblnum++ > CLIMNUM)
		{
//			climiting = 1;
//			limitState = 5;
//			pfoc->pidQ.sp.full -= (pfoc->pidQ.sp.full >> 3);
		}
		else if ( ic > CLMAX && iclnum++ > CLIMNUM)
		{
//			climiting = 1;
//			limitState = 5;
//			pfoc->pidQ.sp.full -= (pfoc->pidQ.sp.full >> 3);
		}

		// test faults
		if ( ia > CFMAX && iafnum++ > CFLTNUM)
		{
			faultReasonLoop(FAULT_LOOPCURRA);
			stopflag = 1;
		}
		else if ( ib > CFMAX && ibfnum++ > CFLTNUM)
		{
			faultReasonLoop(FAULT_LOOPCURRB);
			stopflag = 1;
		}
		else if ( ic > CFMAX && icfnum++ > CFLTNUM)
		{
			faultReasonLoop(FAULT_LOOPCURRC);
			stopflag = 1;
		}
	}

	if (stopflag)
	{
		// stop the foc pwm writes
		pmc->ctrl.enabled = 0;

		// current fault disable pwm and set faulted runStatus
		set_mcpwm(0, 0, 0);

		// re-do the foc params
		QandDinited = 0;


		faultRunState ();
	}

}

// input Vd, Vq are the results of PID, output Vd, Vq are limited if nec
void LimitVqVd(F6_10 *pVq, F6_10 *pVd)
{
	F22_10 MMIVecMax, Vector, VdSq, VqSq;
	F6_10 MMILim, Vq, Vd;


	MMIVecMax.full = 1944;		// 95 % of 2047
	VdSq.full = pVd->full; VdSq.full = MULT_F22_10(VdSq, VdSq);
	VqSq.full = pVq->full; VqSq.full = MULT_F22_10(VqSq, VqSq);
	Vector.full = VdSq.full + VqSq.full;
	Vector.full = sqrt_F22_10(Vector.full);
	MMIVecMax.full = DIV_F22_10(MMIVecMax, Vector);

	if (MMIVecMax.full < 1024)	// factor test is if less than 1
	{
		// apply circle limit
		MMILim.full = MMIVecMax.full;
		Vd.full = pVd->full;	pVd->full =  MULT_F6_10 (Vd, MMILim);
		Vq.full = pVq->full;	pVq->full =  MULT_F6_10 (Vq, MMILim);
	}
}

F6_10	tpd, tpq;
F6_10	PJPId, PJPIq;
/*****************************************************************************
** Function name:		vMC_FOC_Loop
**
** Description:			FOC loop containing the FOC algorithm
**
** Parameters:			None
** Returned value:		None
**
** timing numbers embedded below are measured with a 72MHz main clock
**
*****************************************************************************/
void vMC_FOC_Loop(void)
{
	uint16_t ta, tb, tc;

	// make the oil pwm
	exeOilPWM ();
	exeSolPWM ();

//	device_on(FAN);		// timing

	pmc->ctrl.busy = TRUE;

	CurrentLimit();

	/* Get phase currents */
	Ia.full = f6_10MC_LIB_ScaledADC(getADC (PHASEAINDEX), zpA, igain);
	Ic.full = f6_10MC_LIB_ScaledADC(getADC (PHASECINDEX), zpC, igain);
	Ib.full = -(Ia.full + Ic.full);

	/* Clarke transformation */
//	vMC_LIB_F6_10Clarke(Dir?Ic:Ia, Dir?Ia:Ic, &pfoc->Ialpha, &pfoc->Ibeta);
	vMC_LIB_F6_10Clarke(Ib, Ic, &pfoc->Ialpha, &pfoc->Ibeta);

	getSinCos ();

	/* Park transformation */
	vMC_LIB_F6_10Park(pfoc->Ialpha,
				 	  pfoc->Ibeta,
				 	  &pfoc->Id,
				 	  &pfoc->Iq,
				 	  sinAlpha,
				 	  cosAlpha);

	// control using NXP clarke and park Ib-Ic
	pfoc->Vq.full = f6_10MC_LIB_PID(pfoc->Iq, &pfoc->pidQ);
	pfoc->Vd.full = f6_10MC_LIB_PID(pfoc->Id, &pfoc->pidD);

	// make sure we won't exceed calculation (circle limits)
	LimitVqVd(&pfoc->Vq, &pfoc->Vd);

	/* Inverse park : translate static Vq and Vd to dynamic Valpha and Vbeta */
	vMC_LIB_F6_10InversePark(pfoc->Vd, pfoc->Vq, &pfoc->Valpha, &pfoc->Vbeta, sinAlpha, cosAlpha);

	/* Inverse Clarke */
	vMC_LIB_F6_10InverseClarke(pfoc->Valpha, pfoc->Vbeta, &vr1, &vr2, &vr3);

	/* Scale from 32 to 1; (used to be) divide by 32 (>>5) */
	vr1.full >>= postgain;
	vr2.full >>= postgain;
	vr3.full >>= postgain;

	/* Calculate pwm with SVPWM algorithm */
	vMC_LIB_F6_10SVPWM(Dir?vr2:vr1, vr3, Dir?vr1:vr2, &pfoc->pwm, &pmc->ctrl.sector);

//	addSRec (getADC (PHASECINDEX), pfoc->pwm.A.full, Slipalpha.full, pfoc->pidQ.err.full, pfoc->Vd.full, pfoc->Vq.full);
//	addSRec (Ia.full, Ic.full, pfoc->Id.full, pfoc->Iq.full, pfoc->Vd.full, pfoc->Vq.full);
//	addSRec (Ia.full, Ic.full, pfoc->pwm.A.full, pfoc->Iq.full, pfoc->Id.full, Slipalpha.full / 10);
//	addSRec (Ia.full, Ic.full, pfoc->pwm.A.full, pfoc->Valpha.full, pfoc->Vbeta.full, Slipalpha.full / 10);
//	addSRec (Ia.full, Ic.full, pfoc->pidQ.err.full, pfoc->pidD.err.full, (getBusIVal () - getampoffset())*33, Slipalpha.full / 10);
//	addSRec (pfoc->Id.full, pfoc->Iq.full, pfoc->pidQ.sp.full, Ia.full, Ic.full, Slipalpha.full);
//	addSRec (Ia.full, Ic.full, PJPId.full, PJPIq.full, pfoc->pidQ.sp.full, Slipalpha.full);
//	addSRec (pfoc->Vd.full, pfoc->Vq.full, PJPId.full, PJPIq.full, pfoc->pidQ.sp.full, Slipalpha.full);
//	addSRec (pfoc->pidQ.sp.full, pfoc->Iq.full, pfoc->Vq.full, pfoc->pidD.sp.full, pfoc->Id.full, pfoc->Vd.full);
//	addSRec (pfoc->pidQ.sp.full, pfoc->Iq.full, pfoc->Vq.full, Ia.full, Ib.full, Ic.full);
//	addSRec (512-vr1.full, 512-vr2.full, 512-vr3.full, 512-pfoc->pwm.A.full,512-pfoc->pwm.B.full, Slipalpha.full / 10);
//	addSRec (Ia.full, Ic.full, pfoc->pwm.A.full, sinAlpha.full, cosAlpha.full, Slipalpha.full / 10);
//	addSRec ((qei_readPos()/10), qei_readDir() * 1000, pfoc->pwm.A.full, pfoc->pwm.B.full, pfoc->pwm.C.full, Slipalpha.full / 10);


	if(pmc->ctrl.enabled)vMC_LIB_SetPWM_F6_10(&pfoc->pwm);

	// get the pwm values we just used
	RetPWM (&ta, &tb, &tc);

	addSRec (pfoc->pidQ.sp.full, pfoc->Iq.full, pfoc->Vq.full, Ia.full, Ib.full, Ic.full);
//	addSRec (pfoc->pwm.A.full,pfoc->pwm.B.full,pfoc->pwm.C.full,ta, tb, tc);
	addSRec (pfoc->Id.full, pfoc->Vd.full, ta, qei_read32RPM(), sinAlpha.full, cosAlpha.full);

//	dacR ();

	pmc->ctrl.busy = FALSE;

//	device_off(FAN);		// timing

	return;
}


/*****************************************************************************
**                            End Of File
******************************************************************************/
