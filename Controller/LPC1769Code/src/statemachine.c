/****************************************************************************
 *   $Id:: statemachine.c                   $
 *
 *   Description:
 *     This file contains state machine modules.
 *
 ****************************************************************************/
#include "LPC17xx.h"
#include "type.h"
#include "timer.h"
#include "leds.h"
#include "button.h"
#include "status.h"
#include "statemachine.h"

uint32_t	currentState = STATE_IDLE;
uint32_t	desiredState = STATE_IDLE;

void sm_init (void)
{
	currentState = STATE_IDLE;
	desiredState = STATE_IDLE;
}

void sm_exe (void)
{
	if (currentState != desiredState)
	{
		// what is the next step?
	}

}

uint32_t sm_request (uint32_t newState)
{
	uint32_t	cond = 0;	// set to one on successful state change


	switch (newState)
	{
	case STATE_RUNNING	:
		// what state are we in now?
		switch (currentState)
		{
		case STATE_IDLE		:

		case STATE_STARTING	:
		case STATE_RUNNING	:
			// already there or on the way
			break;

		case STATE_STOPPING	:
		case STATE_STOPPED	:
			//// stopped to running,
			// set the desired state to running
			// check permissives
			// oil pump on
			// precharge
			// stator rpm to zero
			// waveform running
			// hv on
			//
			break;

		case STATE_FAULTED	:
			// need to clear the fault first
			break;
		}
		break;

	case STATE_STOPPED	:
		// what state are we in now?
		switch (currentState)
		{
		case STATE_IDLE		:

		case STATE_STARTING	:
		case STATE_RUNNING	:
			//// running to stopped,
			// set the desired state to stopping
			// check permissives
			// stator rpm ramp to zero
			// hv off
			// bleed hv ?
			// oil pump off
			// waveform disabled
			//

		case STATE_STOPPING	:
		case STATE_STOPPED	:
			// already there or on the way
			break;

		case STATE_FAULTED	:
			// need to clear the fault first
			break;
		}
		break;

	case STATE_FAULTED	:
		// what state are we in now?
		switch (currentState)
		{
		case STATE_IDLE		:
		case STATE_STOPPING	:
		case STATE_STOPPED	:
		case STATE_STARTING	:
		case STATE_RUNNING	:
			break;
		}
		break;

	}

	return cond;
}
