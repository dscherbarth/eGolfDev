/****************************************************************************
 *   $Id:: wdt.h 5803 2010-12-03 00:30:03Z usb00423                         $
 *   Project: NXP LPC17xx WDT example
 *
 *   Description:
 *     This file contains WDT code header definition.
 *
 ****************************************************************************
****************************************************************************/
#ifndef __WDT_H
#define __WDT_H

#define WDEN		(0x1<<0)
#define WDRESET		(0x1<<1)
#define WDTOF		(0x1<<2)
#define WDINT		(0x1<<3)

#define WDT_FEED_VALUE	0x001FFFFF	// about 2 seconds


extern uint32_t WDTInit( void );
extern void WDTFeed( void );

#endif /* end __WDT_H */
/*****************************************************************************
**                            End Of File
******************************************************************************/
