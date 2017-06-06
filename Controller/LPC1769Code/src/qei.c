#include "LPC17xx.h"
#include "type.h"
#include "qei.h"
#include "status.h"
#include "models.h"

#define QEIPPR	4096 //resolver to qei big motor
//#define QEIPPR	2000	// little and medium motor
#define QEILOAD	4000000			// 25 percent of PCLK
#define PCLK	18000000			// 18 Mhz

void qei_init(void)
{
	// turn on the qei peripheral
	LPC_SC->PCONP |= (1 << 18);

	// set the maximum position
	LPC_QEI->MAXPOS = MgetEncCPR ();

	// init the timer for velocity in RPM
	LPC_QEI->LOAD = QEILOAD;

	// filter (experimentally arrived at)
//	LPC_QEI->FILTER = 50;
	LPC_QEI->FILTER = 80;

	// config for a and b edges and respi
//	LPC_QEI->CON = 0x2;		// might need to re regularly reset...
	LPC_QEI->CONF = 0x4;	// cap mode 1

	// configuration of the input pins is done in mcpwm.c
}

uint32_t qei_readPos(void)
{
	return LPC_QEI->POS;
}

uint32_t qei_readRPM(void)
{
	uint32_t	RPM;
	static float trpm = 0;


	trpm = (((float)PCLK * (float)LPC_QEI->CAP * 60.0) / ((float)LPC_QEI->LOAD * (float)MgetEncCPR ()));
	RPM = trpm;

	return RPM;
}

uint32_t qei_read32RPM(void)
{
	uint32_t	RPM;


	RPM = (((uint64_t)PCLK * (uint64_t)LPC_QEI->CAP * (uint64_t)60) / ((uint64_t)LPC_QEI->LOAD * (uint64_t)MgetEncCPR ()));

	return RPM;
}

uint32_t qei_readDir(void)
{
	return LPC_QEI->STAT;	// only 1 bit in the status register for direction
}

