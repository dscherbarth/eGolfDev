//*****************************************************************************
//   +--+       
//   | ++----+   
//   +-++    |  
//     |     |  
//   +-+--+  |   
//   | +--+--+  
//   +----+    Copyright (c) 2010 Code Red Technologies Ltd.
//
// leds.c provided functions to access LED2 (P0_22) on the
// LPCXpresso1768 processor board.
//
//
// Software License Agreement
// 
// The software is owned by Code Red Technologies and/or its suppliers, and is 
// protected under applicable copyright laws.  All rights are reserved.  Any 
// use in violation of the foregoing restrictions may subject the user to criminal 
// sanctions under applicable laws, as well as to civil liability for the breach 
// of the terms and conditions of this license.
// 
// THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
// OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
// USE OF THIS SOFTWARE FOR COMMERCIAL DEVELOPMENT AND/OR EDUCATION IS SUBJECT
// TO A CURRENT END USER LICENSE AGREEMENT (COMMERCIAL OR EDUCATIONAL) WITH
// CODE RED TECHNOLOGIES LTD. 

#include "LPC17xx.h"
#include "leds.h"

void init_leds (void)
{
	uint32_t	state;


	state = LPC_GPIO2->FIOPIN;

	// leds need to be set to one to turn off
	LPC_GPIO2->FIOSET = ~state & ((1 << 7) | (1 << 4) | (1 << 5) | (1 << 6));
}

void led_on (uint32_t ledNum)
{
	uint32_t	state;


	state = LPC_GPIO2->FIOPIN;
	LPC_GPIO2->FIOCLR = ((state) & (1 << (ledNum + 3)));
}

void led_off (uint32_t ledNum)
{
	uint32_t	state;


	state = LPC_GPIO2->FIOPIN;
	LPC_GPIO2->FIOSET = ((~state) & (1 << (ledNum + 3)));
}

void led_invert (uint32_t ledNum)
{
	volatile uint32_t ledstate = 0;

	// Read current state of GPIO P0_0..31, which includes LED2
//	ledstate = LPC_GPIO2->FIOPIN;

	// Turn off LED2 if it is on
	// (ANDing to ensure we only affect the LED output)
	LPC_GPIO2->FIOCLR = ledstate & (1 << (ledNum + 3));

	// Turn on LED2 if it is off
	// (ANDing to ensure we only affect the LED output)
	LPC_GPIO2->FIOSET = ((~ledstate) & (1 << (ledNum + 3)));

}

// Function to initialise GPIO to access LED2
void led2_init (void)
{
	// Set P0_22 to 00 - GPIO
	LPC_PINCON->PINSEL1	&= (~(3 << 12));
	// Set GPIO - P0_22 - to be output
	LPC_GPIO0->FIODIR |= (1 << 22);
}


// Function to turn LED2 on
void led2_on (void)
{
	LPC_GPIO0->FIOSET = (1 << 22);
}

// Function to turn LED2 off
void led2_off (void)
{
	LPC_GPIO0->FIOCLR = (1 << 22);
}

// Function to invert current state of LED2
void led2_invert (void)
{
	uint32_t ledstate;

	// Read current state of GPIO P0_0..31, which includes LED2
	ledstate = LPC_GPIO0->FIOPIN;
	// Turn off LED2 if it is on
	// (ANDing to ensure we only affect the LED output)
	LPC_GPIO0->FIOCLR = ledstate & (1 << 22);
	// Turn on LED2 if it is off
	// (ANDing to ensure we only affect the LED output)
	LPC_GPIO0->FIOSET = ((~ledstate) & (1 << 22));
}

void gpio7_off (void)
{
	LPC_GPIO2->FIOCLR = (1 << 7);
}

// Function to invert current state of LED2
void gpio7_invert (void)
{
	uint32_t gpio7state, test=0;
return;
	// Read current state of GPIO P0_0..31, which includes LED2
	gpio7state = LPC_GPIO2->FIOPIN;

	// Turn off LED2 if it is on
	// (ANDing to ensure we only affect the LED output)
	LPC_GPIO2->FIOCLR = gpio7state & (1 << 7);

	// Turn on LED2 if it is off
	// (ANDing to ensure we only affect the LED output)
	LPC_GPIO2->FIOSET = ((~gpio7state) & (1 << 7));

	if (test == 1)
	{
		LPC_GPIO2->FIODIR &= ~(1 << 7);
		LPC_GPIO2->FIODIR |= (1 << 7);

	}
}

