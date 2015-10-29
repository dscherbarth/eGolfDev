#include "LPC17xx.h"
#include "type.h"
#include "lcd.h"
#include "adc.h"
#include "string.h"
#include "stdio.h"
#include "test.h"
#include "timer.h"
#include "waveform.h"


void focpwmDisable( void );

void TestPhaseCurrent(phase_t	*p)
{
	int i;
	uint32_t pc1, pc2, pc3;


	// and no svm/foc
	focpwmDisable();

	// get average numbers
	set_mcpwm(0, 0, 0);	// zeroize
	pc1 = pc2 = pc3 = 0;
	for (i=0; i<TESTDUR; i++)
	{
		pc1 += getADC (PHASEAINDEX);
		pc2 += getADC (PHASECINDEX);
		pc3 += getADC (AMPSINDEX);			// buss current
		delayMicros(0, 1000, NULL );		// .001Sec delay
	}
	pc1 /= TESTDUR; pc2 /= TESTDUR;
	pc3 /= TESTDUR;
	p->pAz = pc1; p->pCz = pc2; p->Bz = pc3;

	// pA - pB, collect
	test_outputs(TESTVAL, TESTVAL, 0);
	pc1 = pc2 = 0;
	for (i=0; i<TESTDUR; i++)
	{
		pc1 += getADC (PHASEAINDEX);
		pc2 += getADC (PHASECINDEX);
		delayMicros(0, 1000, NULL );		// .001Sec delay
	}
	pc1 /= TESTDUR; pc2 /= TESTDUR;
	p->abA = pc1; p->abC = pc2;

	// settle
	test_outputs(0, 0, 0);	// zeroize
	delayMicros(0, 100000, NULL );		// .001Sec delay

	// PA - PC, collect
	test_outputs(TESTVAL, 0, TESTVAL);
	pc1 = pc2 = 0;
	for (i=0; i<TESTDUR; i++)
	{
		pc1 += getADC (PHASEAINDEX);
		pc2 += getADC (PHASECINDEX);
		delayMicros(0, 1000, NULL );		// .001Sec delay
	}
	pc1 /= TESTDUR; pc2 /= TESTDUR;
	p->acA = pc1; p->acC = pc2;

	// settle
	test_outputs(0, 0, 0);	// zeroize
	delayMicros(0, 100000, NULL );		// .001Sec delay

	// PB - PC, collect
	test_outputs(0, TESTVAL, TESTVAL);
	pc1 = pc2 = 0;
	for (i=0; i<TESTDUR; i++)
	{
		pc1 += getADC (PHASEAINDEX);
		pc2 += getADC (PHASECINDEX);
		delayMicros(0, 1000, NULL );		// .001Sec delay
	}
	pc1 /= TESTDUR; pc2 /= TESTDUR;
	p->bcA = pc1; p->bcC = pc2;

	// reset to zero state
	test_outputs(0, 0, 0);	// off

}

#define EXPECTED_abA	100
#define EXPECTED_abC	100
#define EXPECTED_bcA	100
#define EXPECTED_bcC	100
#define EXPECTED_acA	100
#define EXPECTED_acC	100
#define EXPECTED_THRESH	10		//threshold

int TestValidate (phase_t	*p)
{
	int ret = 1;		// start with ok

	if (	(p->abA > (EXPECTED_abA + EXPECTED_THRESH)) || (p->abA < (EXPECTED_abA - EXPECTED_THRESH)) ||
			(p->bcA > (EXPECTED_bcA + EXPECTED_THRESH)) || (p->bcA < (EXPECTED_bcA - EXPECTED_THRESH)) ||
			(p->acA > (EXPECTED_acA + EXPECTED_THRESH)) || (p->acA < (EXPECTED_acA - EXPECTED_THRESH)) ||
			(p->abC > (EXPECTED_abC + EXPECTED_THRESH)) || (p->abC < (EXPECTED_abC - EXPECTED_THRESH)) ||
			(p->bcC > (EXPECTED_bcC + EXPECTED_THRESH)) || (p->bcC < (EXPECTED_bcC - EXPECTED_THRESH)) ||
			(p->acC > (EXPECTED_acC + EXPECTED_THRESH)) || (p->acC < (EXPECTED_acC - EXPECTED_THRESH)) )
	{
		ret = 0;		// test failed
	}

	return ret;
}

void displayPTest (phase_t *p)
{
	char temp[25];

	// display
	sprintf (temp, "%04d  %04d %04d %04d", p->pAz, p->abA, p->acA, p->acA);
	lcd_cursor (0, 0);
	lcd_write (temp, 20);
	sprintf (temp, "%04d  %04d %04d %04d", p->Bz, p->abC, p->acC, p->acC);
	lcd_cursor (1, 0);
	lcd_write (temp, 21);

}
