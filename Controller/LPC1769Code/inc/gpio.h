#ifndef __GPIO_H
#define __GPIO_H

// gpio allocations
//	Name 	I/O		Assignment			Pin		#Define ID
//	p2.0	out		oil pump on-off		J6-42	OILPUMP
//	p2.1	in		oil pressure sw in	J6-43	OILPRESS
//	p2.2	out		pre-charge on-off	J6-44	PRECHARGE
//	p2.3	out		hv solenoid on-off	J6-45	HVSOLENOID
// 	p2.4	out		led 1				J6-46
//	p2.5	out		led	2				J6-47
//	p2.6	out		led	3				J6-48
//	p2.7	out		led	4				J6-49
//	p2.8	in		top button			J6-50	BUTTON1
//	p2.9	out		cooling fan on-off	PAD-19	FAN
//	p2.10	in		mid top button		J6-51	BUTTON2
//	p2.11	in		mid bottom button	J6-52	BUTTON3
//	p2.12	in		bottom button		J6-53	BUTTON4
//	p2.13	in		current limit fault J6-27	FAULT

//	p0.19								PAD-17
//
//	p1.21	in		MCABORT/			PAD-4
//
// 	p3.25	in		driver board pilot	PAD-13	DRVPILOT
//	p3.26	in		drive fast abort (jumper from pad 4 to pad 14)	PAD-14
//
//  p4.28								PAD-15
// (ext int should be made available for trans vss sensor) J6-51

#define OILPUMP	0
#define	OILPRESS 1
#define	PRECHARGE 2
#define	HVSOLENOID 3
#define FAULTRESET 4
#define	FAN	9
#define	FAULTDET 13
#define	DRVPILOT 25

void device_init(void);
void device_on (uint32_t deviceName);
void device_off(uint32_t deviceName);
int device_test (uint32_t deviceName);
void gpio_init(void);

void SetSolPWM (int oilperc);
void exeSolPWM (void);
void SetOilPWM (int oilperc);
void exeOilPWM (void);

#endif
/*****************************************************************************
**                            End Of File
******************************************************************************/
