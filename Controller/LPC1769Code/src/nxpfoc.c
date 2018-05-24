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
#include "snapshot.h"
#include "waveform.h"
#include "control.h"
#include "command.h"
#include "Sensors.h"
#include "models.h"
#include "timer.h"

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
	pfoc->pidQ.out_max.full = 6000;		// based on postgain of 2 (experimental)
	pfoc->pidQ.out_min.full = -6000;
	vMC_LIB_PID_Reset(&pfoc->pidQ);

	pfoc->pidD.out_max.full = 2000;		// (experimental)
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
F6_10	igain;
int		postgain = 5;
uint32_t	limitState = 0;
int limiting = 0;
int climiting = 0;

F6_10	pi;
void QDinit()
{
	if (!QandDinited)
	{
		cpr = MgetEncCPR ();

		pi.full = F6_10_CONST(3.14159265358979);
		polespi.full = MULT_F6_10s(pi, (1 << 10));

		igain.full = F6_10_CONST(MgetIgain());
		postgain = MgetPostgain();

		deltapos = 0; currpos = 0; lastpos = 0;
		Slipalpha.full = 0;

		fltemp = (2 * 3.14159265);
		fltemp *= MgetTimeConst();
		flconst = (1.0 / fltemp) / ((POLEPAIRS * PWM_FREQUENCY) / cpr); // 1/((2 * pi) * Tr) / (10000 / 2000)
		fsconst.full = F6_10_CONST(flconst);

		QandDinited = 1;
	}
}

uint32_t QLimit (F6_10 QSetPoint)
{
	F6_10 SaveQSetPoint = QSetPoint;
	float	dif;


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
	if (getTemp () > 170)
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
	if (QSetPoint.full < 0 && getBusvolt () > 368)
	{
		QSetPoint.full = 0;
	}

	return QSetPoint.full;
}

static F6_10	cmdsp;
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
	static F6_10	fsmag;


	QSetPoint.full = Q ; 	// delivered << by 10 bits
	DSetPoint.full = D; 	// delivered << by 10 bits

	SaveQSetPoint.full = pfoc->pidQ.sp.full;
	SaveDSetPoint.full = pfoc->pidD.sp.full;

	QDinit();

	QSetPoint.full = QLimit(QSetPoint);

	// load up the setpoints
	if (!limiting & !climiting)
	{
		SaveQSetPoint.full = QSetPoint.full;
	}
	else
	{
		SaveQSetPoint.full = QSetPoint.full = 0;
	}
	if(QSetPoint.full != 0)
	{
		SaveDSetPoint.full = DSetPoint.full;
	}
	else
	{
		SaveDSetPoint.full = 0;
	}

	// calculate slip
	ftemp.full = DIV_F6_10(SaveQSetPoint, SaveDSetPoint);	// regen will make negative slip
	fs.full = MULT_F6_10(ftemp, fsconst);	// accounts for time constant (tr) and 1/2*PI

	fsmag.full = F6_10_CONST(1.25);

	fs.full = MULT_F6_10(fs, fsmag);	// factor

	// invert torque if regening
	if (QSetPoint.full < 0)
		{
		SaveQSetPoint.full = -SaveQSetPoint.full;
		}

	// scale adjust for Qsp (this really just affects pedal position)
	SaveQSetPoint.full /= 2;

	// experimental absolute q limit
	if (SaveQSetPoint.full > 5000)
	{
		SaveQSetPoint.full = 5000;
	}

	cmdsp.full = SaveQSetPoint.full;	// allow high speed loop to average
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
#define CFMAX 1900		// should be 490 Amps
#define CFLTNUM		8 	// number of times this must be true
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
	if (pmc->ctrl.enabled)
	{
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


//	MMIVecMax.full = 1944;		// 95 % of 2047
	MMIVecMax.full = 5444;	// test !!!!
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

void RetPWM (uint16_t * a, uint16_t * b, uint16_t * c );

F6_10	tpd, tpq;
F6_10	PJPId, PJPIq;
extern	uint32_t			gRPM;
static	uint32_t			avg = 0;

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

	// make the oil and contactor economizer pwms
	exeOilPWM ();
	exeSolPWM ();

//	device_on(FAN);		// timing

	// produce an average qsp
	avg = (avg * 11 + cmdsp.full) / 12;
	pfoc->pidQ.sp.full = avg;

	pmc->ctrl.busy = TRUE;

	CurrentLimit();

	/* Get phase currents */
	Ia.full = f6_10MC_LIB_ScaledADC(getADC (PHASEAINDEX), zpA, igain);
	Ic.full = f6_10MC_LIB_ScaledADC(getADC (PHASECINDEX), zpC, igain);
	Ib.full = -(Ia.full + Ic.full);

	/* Clarke transformation */
//	vMC_LIB_F6_10Clarke(Dir?Ib:Ic, Dir?Ic:Ib, &pfoc->Ialpha, &pfoc->Ibeta);
	vMC_LIB_F6_10Clarke(Ib, Ic, &pfoc->Ialpha, &pfoc->Ibeta);

	getSinCos ();

	/* Park transformation */
	vMC_LIB_F6_10Park(pfoc->Ialpha, pfoc->Ibeta, &pfoc->Id, &pfoc->Iq, sinAlpha, cosAlpha);

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

	if(pmc->ctrl.enabled)vMC_LIB_SetPWM_F6_10(&pfoc->pwm);

	// get the pwm values we just used
	RetPWM (&ta, &tb, &tc);
	uint16_t trpm = (uint16_t)qei_read32RPM();

	// add snapshot values to the ring
	addSRec12 (pfoc->pidQ.sp.full, pfoc->Iq.full, pfoc->Vq.full,
			   fs.full, trpm,
			   pfoc->Id.full, pfoc->Vd.full,
			   Ia.full, pfoc->pwm.A.full,
			   sinAlpha.full, (uint16_t)currpos,
			   ta);
//	addSRec12 ((uint16_t)1,(uint16_t)2,(uint16_t)3,(uint16_t)4,(uint16_t)5,(uint16_t)6,(uint16_t)7,(uint16_t)8,(uint16_t)9,(uint16_t)10,(uint16_t)11,(uint16_t)12);

	pmc->ctrl.busy = FALSE;

//	device_off(FAN);		// timing

	return;
}


/*****************************************************************************
**                            End Of File
******************************************************************************/
