//
// models.c
//
// all of the supported motor models required data are kept here
// includes:
//	poles, maxrpm, time constant, tuning constants, mag and accel tables, encoder parameters
// provide storage of this info and a way to select which one we are using
//
#include "lpc17xx.h"
#include "type.h"
#include "models.h"

/*
	;     Imag = Imag + (fLoopPeriod/fRotorTmConst)*(Id - Imag)
    ;
    ;  Slip speed in RPS:
    ;     VelSlipRPS = (1/fRotorTmConst) * Iq/Imag / (2*pi)
    ;
    ;  Rotor flux speed in RPS:
    ;     VelFluxRPS = iPoles * VelMechRPS + VelSlipRPS
    ;
    ;  Rotor flux angle (radians):
    ;     AngFlux = AngFlux + fLoopPeriod * 2 * pi * VelFluxRPS
    ;
    ;; qdImag = qdImag + qKcur * (qId - qdImag)      ;; magnetizing current
    ;; qVelSlip   = qKslip * qIq/qdImag
*/

// little motor tables
static param_table lm_mag[] = { 	{0,				500},
									{5,				500},
									{600,			400},
									{800,			300},
									{1000,			300},
									{1200,			200},
									{2000,			100},
									{-1,		-1}
};

static param_table lm_torque[] = { 	{0,				0},
									{100,			0},
									{150,			600},
									{2000,			18000},
									{-1,		-1}
};

#ifdef mmtab
// medium motor tables
static param_table mm_mag[] = { 	{0,				1250},
									{5,				1250},
									{600,			1000},
									{800,			800},
									{1000,			500},
									{1200,			300},
									{2000,			100},
									{-1,		-1}
};

static param_table mm_torque[] = { 	{0,				0},
									{100,			0},
									{150,			600},
									{2000,			18000},
									{-1,		-1}
};
#endif

#ifdef oldbooktab
static param_table mm_book_mag[] = { {0,			1050},
									{1280,			1050},
									{1536,			1024},
									{1792,			945},
									{2048,			901},
									{2304,			871},
									{2560,			851},
									{2816,			831},
									{3072,			801},
									{3328,			771},
									{3584,			760},
									{3840,			729},
									{4096,			698},
									{4352,			657},
									{4608,			624},
									{7000,			301},
									{-1,		-1}
};
#endif

static param_table mm_book_mag[] = { {0,			1050},
									{2400,			1050},
									{2880,			839},
									{3360,			699},
									{3840,			599},
									{4320,			524},
									{4800,			465},
									{5280,			419},
									{5760,			381},
									{6240,			349},
									{6720,			322},
									{7200,			299},
									{7680,			279},
									{8160,			262},
									{8640,			246},
									{10000,			246},
									{-1,		-1}
};

static param_table bm_time_const[] = { {0,			280},
									{10000,			280},
									{-1, 			-1}
};

static param_table mm_book_torque[] = { {0,				0},
										{228,			384},
										{292,			512},
										{356,			1024},
										{420,			2048},
										{484,			2560},
										{548,			3072},
										{612,			3584},
										{776,			4096},
										{740,			4608},
										{804,			5120},
										{868,			5632},
										{932,			6144},
										{996,			6656},
										{1060,			7168},
										{1124,			7680},
										{1188,			8192},
										{1252,			9216},
										{1316,			10240},
										{1380,			11264},
										{1444,			12288},
										{1508,			12928},
										{1572,			13056},
										{1636,			13184},
										{1700,			13312},
										{1764,			13440},
										{1828,			13568},
										{1892,			13632},
										{1956,			13696},
										{2020,			13760},
										{2048,			13824},
										{-1,		-1}
};
#ifdef bmtab
// big motor tables
static param_table bm_mag[] = { 	{0,				500},
									{5,				500},
									{600,			400},
									{800,			300},
									{1000,			300},
									{1200,			200},
									{2000,			100},
									{-1,		-1}
};

static param_table bm_torque[] = { 	{0,				0},
									{100,			0},
									{150,			600},
									{8000,			18000},
									{-1,		-1}
};
#endif

static motor_t	qhp = {
		"little 1/4hp motor",	// Dave's motor
		3,
		1200,
		0.04183,
		6.8176,
		1.0,
		2.0,
		3.0,		// fault limit current
		5,
		.8, 0.0, 0.0,
		.8, 0.0, 0.0,
		2000,
		lm_mag,
		lm_torque,
		bm_time_const
};

static motor_t	oohhp = {
		"medium 1-1/2hp motor",	// Bob's motor
		2,
		1800,
		0.100,
		30.0,
		4.0,
		3.5,		// might need to be higher to reach max torque but I want to protect Bob's motor
		5.0,
		2,
		.11, 0.0, 0.0,			// D
		.15, 0.0, 0.0,			// Q
		2000,
		mm_book_mag,
		mm_book_torque,
		bm_time_const
};

static motor_t	bighp = {
		"(big) 250hp motor",	// Eric's motor
		2,
		6500,		// testing
//		8000,
		0.270,
		3.0,
		1.0,
		250.0,		// testing
		350.0,		// testing
//		300.0,		// yikes!!
//		400.0,		// double yikes!
		2,			// post gain (atten)
		1.25, .55, 0.0,
		1.15, .73, 0.0,
		4096,
		mm_book_mag,
		mm_book_torque,
		bm_time_const
};

static motor_t	*selMod = 0;

//
// ModelSelect
//
void ModelSelect (int motor)
{
	switch (motor)
	{
	case LITTLE :
		selMod = &qhp;
		break;

	case MEDIUM :
		selMod = &oohhp;
		break;

	case BIG :
		selMod = &bighp;
		break;
	}
}

extern int32_t dtab;
//
// accessors
//
/******************************************************************************
** Function name:		MgetMagVal
**
** Descriptions:		use mag req table based on freq
**
** parameters:			desired frequency in rpm
** Returned value:		mag val
**
******************************************************************************/
uint32_t MgetMagVal (uint32_t rpm)
{
	int	magIndex;
	int magval;
	float sperc, tv;


	// protect against un-selected model
	if (!selMod)
	{
		return 1;		// limit div/0 possibility
	}

	// not found! zero index should be safe
	if (rpm < selMod->mag_table[0].input)
		{
		return (selMod->mag_table[0].result);
		}

	// search the table
	for (magIndex = 0; selMod->mag_table[magIndex].input != -1; magIndex++)
		{
			if (rpm >= selMod->mag_table[magIndex].input && rpm < selMod->mag_table[magIndex+1].input)
			{
				// found the segment, calculate the mag to return
				magval = selMod->mag_table[magIndex].result;
				sperc = (float)(rpm - selMod->mag_table[magIndex].input) / (float)(selMod->mag_table[magIndex+1].input - selMod->mag_table[magIndex].input);

				tv = ((float)(selMod->mag_table[magIndex+1].result) - (float)(selMod->mag_table[magIndex].result));
				tv = tv * sperc;
				tv = tv + magval;
				magval = (int) tv;
				return (magval);
			}
		}

	// off the end of the table, clamp the result at the end of the table
	if (rpm > selMod->mag_table[magIndex-1].input)
	{
		return (selMod->mag_table[magIndex-1].result);
	}

	return 1;

}

uint32_t MgetTimeC (uint32_t rpm)
{
	int	tcIndex;
	int tcval;
	float sperc, tv;


	// protect against un-selected model
	if (!selMod)
	{
		return 1;		// limit div/0 possibility
	}

	// not found! zero index should be safe
	if (rpm < selMod->tc_table[0].input)
		{
		return (selMod->tc_table[0].result);
		}

	// search the table
	for (tcIndex = 0; selMod->tc_table[tcIndex].input != -1; tcIndex++)
		{
			if (rpm >= selMod->tc_table[tcIndex].input && rpm < selMod->tc_table[tcIndex+1].input)
			{
				// found the segment, calculate the mag to return
				tcval = selMod->tc_table[tcIndex].result;
				sperc = (float)(rpm - selMod->tc_table[tcIndex].input) / (float)(selMod->tc_table[tcIndex+1].input - selMod->tc_table[tcIndex].input);

				tv = ((float)(selMod->tc_table[tcIndex+1].result) - (float)(selMod->tc_table[tcIndex].result));
				tv = tv * sperc;
				tv = tv + tcval;
				tcval = (int) tv;
				return (tcval);
			}
		}

	// off the end of the table, clamp the result at the end of the table
	if (rpm > selMod->tc_table[tcIndex-1].input)
	{
		return (selMod->tc_table[tcIndex-1].result);
	}

	return 1;

}
int fakeoutisval = 0;
void Isvalset (int val)
{
	fakeoutisval = val;
}

/******************************************************************************
** Function name:		MgetAccSq
**
** Descriptions:		use Acc sq table based on torque
**
** parameters:			desired torque in ft-lbs
** Returned value:		IsSq
**
******************************************************************************/
uint32_t MgetAccSq (uint32_t torque)
{
	int	tqIndex;
	int Accval;
	float sperc, tv;


	if (fakeoutisval)
	{
		return fakeoutisval;
	}

	// protect against un-selected model
	if (!selMod)
	{
		return 0;
	}

	// not found! zero index should be safe
	if (torque < selMod->tq_table[0].input)
		{
		return (selMod->tq_table[0].result);
		}

	// search the table
	for (tqIndex = 0; selMod->tq_table[tqIndex].input != -1; tqIndex++)
		{
			if (torque >= selMod->tq_table[tqIndex].input && torque < selMod->tq_table[tqIndex+1].input)
			{
				// found the segment, calculate the mag to return
				Accval = selMod->tq_table[tqIndex].result;
				sperc = (float)(torque - selMod->tq_table[tqIndex].input) / (float)(selMod->tq_table[tqIndex+1].input - selMod->tq_table[tqIndex].input);
				tv = ((float)(selMod->tq_table[tqIndex+1].result) - (float)(selMod->tq_table[tqIndex].result));
				tv = tv * sperc;
				tv = Accval + tv;
				Accval = (int)tv;
				return Accval;
//				return Accval * 2;
			}
		}

	// off the end of the table, clamp the result at the end of the table
	if (torque > selMod->tq_table[tqIndex-1].input)
	{
		return ((selMod->tq_table[tqIndex-1].result) * 2);
	}

	// not found! zero should be safe
	return 0;

}

int MgetPoles (void)
{
	return (selMod? selMod->poles:0);
}

int MgetRPMLimit (void)
{
	return (selMod? selMod->maxrpm:0);
}

extern	uint32_t			gRPM;
float MgetTimeConst (void)
{
//	return ((float)MgetTimeC(gRPM) / 1000);
	return (selMod? selMod->tr_factor:0.0);
}

void MsetTimeConst (float tc)
{
	if (!selMod)
	{
		return;
	}
	selMod->tr_factor = tc;
	selMod->tc_table[1].result = (uint32_t)(tc * 1000);
}

void MgetDPID (float *p, float *i, float *d)
{
	if (!selMod)
	{
		return;
	}
	*p = selMod->pid_d_p;
	*i = selMod->pid_d_i;
	*d = selMod->pid_d_d;
}

void MsetDPID (float *p, float *i, float *d)
{
	if (!selMod)
	{
		return;
	}
	selMod->pid_d_p = *p;
	selMod->pid_d_i = *i;
	selMod->pid_d_d = *d;
}

void MgetQPID (float *p, float *i, float *d)
{
	if (!selMod)
	{
		return;
	}
	*p = selMod->pid_q_p;
	*i = selMod->pid_q_i;
	*d = selMod->pid_q_d;
}
void MsetQPID (float *p, float *i, float *d)
{
	if (!selMod)
	{
		return;
	}
	selMod->pid_q_p = *p;
	selMod->pid_q_i = *i;
	selMod->pid_q_d = *d;
}

int MgetEncCPR (void)
{
	return (selMod? selMod->encoder_cpr:0);
}
float Dgain = 2;
float MgetDgain (void)
{
	return (Dgain);
}
void MsetDgain (float Dg)
{
	Dgain = Dg;
}
float MgetIgain (void)
{
	return (selMod? selMod->PhIgain:1.0);
}
void MsetIgain (float Igain)
{
	selMod->PhIgain = Igain;
}
float MgetIgain2 (void)
{
	return (selMod? selMod->PhIgain2:1.0);
}
int MgetPostgain (void)
{
	return (selMod? selMod->postinvclarkegain:1.0);
}
void MsetPostgain (int pgain)
{
	selMod->postinvclarkegain = pgain;
}
float MgetBusILimit (void)
{
	return (selMod? selMod->maxCurrent:1.0);
}
float MgetBusIFLimit (void)
{
	return (selMod? selMod->faultCurrent:1.0);
}
