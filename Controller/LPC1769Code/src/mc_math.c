/*****************************************************************************
 * $Id: mc_math.c 6577 2011-06-06 12:00:00Z nxp16893 $
 *
 * Project: MC_math
 *
 * Description: Motor Control math libraries
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
#include "mc_math.h"

/*****************************************************************************
** Function name:		sqrt_F22_10
**
** Description:			Integer squareroot of the full part of a F22_10 fixed 
**						point calculation variable 
**
** 						The integer square root of a fixed point number divided 
** 						by the square root of 2 to the F power where F is the 
** 						number of bits of fraction in the fixed point number is 
** 						the square root of the original fixed point number. 
** 						If you have an even number of bits of fraction you can 
** 						convert the integer square root to the fixed point square 
** 						root with a shift. Otherwise you have to do one divide to 
** 						recover the square root. 
**
** 						The sqrt(a * b) = sqrt(a) * sqrt(b) and
** 						fixed point numbers are scaled integers
** 						you can take the square root of a fixed 
** 						point number by taking the square root of
** 						the integer representation of the number
** 						and then dividing by the square root of the
** 						scale factor.
**
** 						For fixed point formats with an even number
** 						of fractional bits the division can be done
** 						with a simple shift operation.
**
** Parameters:			.full part of F22_10 fixed point variable
** Returned value:		.full part of F22_10 fixed point variable 
**
*****************************************************************************/	
#define step(shift) \
	if((0x40000000l >> shift) + root <= value)          \
	{                                                   \
	    value -= (0x40000000l >> shift) + root;         \
	    root = (root >> 1) | (0x40000000l >> shift);    \
	}                                                   \
	else                                                \
	{                                                   \
	    root = root >> 1;                               \
	}
int32_t sqrt_F22_10(int32_t value)
{
    long root = 0;

    step( 0);
    step( 2);
    step( 4);
    step( 6);
    step( 8);
    step(10);
    step(12);
    step(14);
    step(16);
    step(18);
    step(20);
    step(22);
    step(24);
    step(26);
    step(28);
    step(30);

    // round to the nearest integer, cuts max error in half
	if(root < value)++root;
    root <<= 5;	   	// 10/2 bits fraction
	return root-2;
}
/*****************************************************************************
**                            End Of File
******************************************************************************/
