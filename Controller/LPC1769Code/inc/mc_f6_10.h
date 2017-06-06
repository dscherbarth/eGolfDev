/*****************************************************************************
 * $Id: mc_f6_10.h 6577 2011-06-06 12:00:00Z nxp16893 $
 *
 * Project: MC_lib
 *
 * Description: Motor Control fixed point F6_10 definitions
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
#ifndef __MC_F6_10_H
#define	__MC_F6_10_H

// Fixed Point 6 bits signed integer, 10 bits (=1024) unsigned fraction
// Range -31,9990234375 .. 32
// Precision: 0,0009765625
typedef union F6_10tag 
{
	int16_t full;
	struct partF6_10tag 
	{
		unsigned short fraction: 10;	// Y = 10
		int16_t integer: 6;		// X = 6
	}part;
}F6_10;
#define MULT_F6_10(A,B) 		(((int32_t)A.full*B.full+512)>>10)
#define MULT_F6_10s(A,B) 		(((int32_t)A.full*B+512)>>10)
#define MULT_F6_10ss(A,B) 		(((int32_t)A*B+512)>>10)
#define DIV_F6_10(A,B) 			(((((int32_t)A.full<<11)/B.full)+1)/2)
#define DIV_F6_10s(A,B)			(((((int32_t)A.full<<11)/B)+1)/2)
#define F6_10_CONST(f)			(f<0 ?  (((int16_t)(f)<<10) - (((int16_t)((-f+0.00048828125)*1024))&0x3FF)) : (((int16_t)(f)<<10) + (((int16_t)((f+0.00048828125)*1024))&0x3FF)))
#define F6_10_TOFLOAT(F)		((float) F.part.fraction / 1024 + (float) F.part.integer) 

// Fixed Point 22 bits signed integer, 10 bits (=1024) unsigned fraction
// Range -2097151,99902344 .. 2097152
// Precision: 0,0009765625
typedef union F22_10tag 
{
	int32_t full;
	struct partF22_10tag 
	{
		uint32_t fraction: 10;	// Y = 10
		int32_t integer: 22;		// X = 22
	}part;
}F22_10;
#define MULT_F22_10(A,B) 	(((int64_t)A.full*B.full+512)>>10)
#define MULT_F22_10s(A,B) 	(((int64_t)A.full*B+512)>>10)
#define DIV_F22_10(A,B) 	(((((int64_t)A.full<<11)/B.full)+1)/2)
#define DIV_F22_10s(A,B) 	(((((int64_t)A.full<<11)/B)+1)/2)
//#define F22_10_CONST(f) 			(f<0 ? (((int32_t)(f)<<10) - (((int32_t)((f-0.00048828125)*1024))&0x3FF)) : (((int32_t)(f)<<10) + (((int32_t)((f+0.00048828125)*1024))&0x3FF)))
#define F22_10_CONST(f)		(f<0 ? (((int32_t)(f)<<10) - (((int32_t)((-f+0.00048828125)*1024))&0x3FF)) : (((int32_t)(f)<<10) + (((int32_t)((f+0.00048828125)*1024))&0x3FF)))
//#define F6_10_TOFLOAT(F)			((float) F.part.fraction / 1024 + (float) F.part.integer) 

// FIXED POINT 16 bits integer 16 bits fraction
// Range -32768 - 32767
// Granularity 0.0000152588
typedef union F16_16tag 
{
	int32_t full;
	struct partF16_16tag 
	{
		uint32_t fraction: 16;	// Y = 16
		int32_t integer: 	16;	// X = 16
	}part;
}F16_16;
#define MULT_F16_16(A,B) 	(((int64_t)A.full*B.full+32768)>>16)
#define MULT_F16_16s(A,B) 	(((int64_t)A.full*B+32768)>>16)
#define DIV_F16_16(A,B) 	(((((int64_t)A.full<<17)/B.full)+1)/2)
#define DIV_F16_16s(A,B) 	(((((int64_t)A.full<<17)/B)+1)/2)
//#define F16_16_CONST(f) 	(f<0 ? (((int32_t)(f)<<16) - (((int32_t)((f-0.0000076294)*65536))&0xFFFF)) : (((int32_t)(f)<<16) + (((int32_t)((f+0.0000076294)*65536))&0xFFFF)))
#define F16_16_CONST(f)		(f<0 ? ( ((int32_t)(f)<<16) - (((int32_t)((-f+0.0000076294)*65536))&0xFFFF)) : (((int32_t)(f)<<22) + (((int32_t)((f+0.0000076294)*65536))&0xFFFF)))

int16_t F6_10_sin(int16_t angle);
int16_t F6_10_cos(int16_t angle);

#endif /* end __MC_F6_10_H */

/*****************************************************************************
**                            End Of File
******************************************************************************/
