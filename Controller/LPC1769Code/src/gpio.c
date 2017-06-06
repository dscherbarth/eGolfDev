#include "LPC17xx.h"
#include "type.h"
#include "gpio.h"

/*****************************************************************************
** Function name:		gpio_init
**
** Descriptions:		initialize gpio2 for buttons and leds
**
** parameters:			none
** Returned value:		nothing
**
*****************************************************************************/
void gpio_init(void)
{

	uint32_t	state;

	// initialize pgio2 11 and 12 to be digital inputs
	LPC_PINCON->PINSEL4 &= ~0x03c70000;	/* p2.5, 2.6, 2.7, 2.11 and 2.12 as gpio */

	LPC_GPIO2->FIODIR |= (1 << 4);
	LPC_GPIO2->FIODIR |= (1 << 5);
	LPC_GPIO2->FIODIR |= (1 << 6);
	LPC_GPIO2->FIODIR |= (1 << 7);

	// leds need to be set to one to turn off
	state = LPC_GPIO2->FIOPIN;
	LPC_GPIO2->FIOSET = ~state & ((1 << 7) | (1 << 4) | (1 << 5) | (1 << 6));


}


void device_init(void)
{

	// write a one to config as output
	LPC_GPIO2->FIODIR |= ((1 << OILPUMP) | ( 1 << PRECHARGE) | ( 1 << HVSOLENOID) | ( 1 << FAN) | (1 << FAULTRESET));

	// active low devices need to be set to one to turn off
	LPC_GPIO2->FIOSET = ((1 << OILPUMP) | ( 1 << PRECHARGE) | ( 1 << HVSOLENOID) | ( 1 << FAN) | (1 << FAULTRESET));

}

// For active low devices
void inline device_on (uint32_t deviceName)
{
	/*
	"When FIOSET or FIOCLR are used, only pin/bit(s) written with 1 will be changed,
	while those written as 0 will remain unaffected."
	*/

	LPC_GPIO2->FIOCLR = (1 << deviceName);
}

// For active low devices
void inline device_off(uint32_t deviceName)
{
	/*
	"When FIOSET or FIOCLR are used, only pin/bit(s) written with 1 will be changed,
	while those written as 0 will remain unaffected."
	*/

	LPC_GPIO2->FIOSET = (1 << deviceName);
}

int device_test (uint32_t deviceName)
{


	// gpio bit clear indicates button is pressed
	if (deviceName == DRVPILOT)
	{
		return ((LPC_GPIO3->FIOPIN & (1 << deviceName)) );
	}
	else
	{
		LPC_GPIO2->FIOMASK = 0x0;

		return ((LPC_GPIO2->FIOPIN & (1 << deviceName)) );
	}
}

// Solenoid pwm (0 - 9)
//
int SolOnCount, SolOffCount;
int wkSolOnCount = 0, wkSolOffCount = 10;
int SolState = 0;	// initially off
void SetSolPWM (int Solperc)
{
	SolOnCount = Solperc;
	if (SolOnCount < 0) SolOnCount = 0;
	if (SolOnCount > 10) SolOnCount = 10;
	SolOffCount = 10 - SolOnCount;
}

void exeSolPWM (void)
{
	if (SolOnCount == 0)
	{
		// special case
		device_off (HVSOLENOID);
		SolState = 0;
	}
	else if (SolOnCount == 10)
	{
		// special case
		device_on (HVSOLENOID);
		SolState = 1;
	}
	else
	{
		if (SolState == 0)
		{
			// was off
			if (wkSolOffCount <= 0)
			{
				device_on (HVSOLENOID);
				SolState = 1;
				wkSolOffCount = SolOffCount;
			}
			else
			{
				wkSolOffCount--;
			}
		}
		else
		{
			// on
			if (wkSolOnCount <= 0)
			{
				device_off (HVSOLENOID);
				SolState = 0;
				wkSolOnCount = SolOnCount;
			}
			else
			{
				wkSolOnCount--;
			}
		}
	}
}

// oil pump pwm (0 - 9)
//
int oilOnCount, oilOffCount;
int wkoilOnCount = 0, wkoilOffCount = 10;
int oilState = 0;	// initially off
void SetOilPWM (int oilperc)
{
	oilOnCount = oilperc;
	if (oilOnCount < 0) oilOnCount = 0;
	if (oilOnCount > 10) oilOnCount = 10;
	oilOffCount = 10 - oilOnCount;
}

void exeOilPWM (void)
{
	if (oilOnCount == 0)
	{
		// special case
		device_off (OILPUMP);
		oilState = 0;
	}
	else
	{
		if (oilState == 0)
		{
			// off
			if (wkoilOffCount <= 0)
			{
				device_on (OILPUMP);
				oilState = 1;
				wkoilOffCount = oilOffCount;
			}
			else
			{
				wkoilOffCount--;
			}
		}
		else
		{
			// on
			if (wkoilOnCount <= 0)
			{
				device_off (OILPUMP);
				oilState = 0;
				wkoilOnCount = oilOnCount;
			}
			else
			{
				wkoilOnCount--;
			}
		}
	}
}


