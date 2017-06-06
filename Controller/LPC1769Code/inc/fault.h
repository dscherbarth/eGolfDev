#define EINT0_EDGE	0x00000001
#define EINT0		0x00000001
#define EINT3_EDGE	0x00000008
#define EINT3		0x00000008

#define FAULT_OVERTEMP		1
#define FAULT_OVERCURR		2
#define FAULT_OVERVOLT		3
#define FAULT_UNDERVOLT		4
#define FAULT_OILPRESS		5
#define FAULT_PRECHARGE		6
#define FAULT_DESATURATE	7
#define FAULT_PHASETESTFAIL	8
#define FAULT_RELAYPOS		9
#define FAULT_LOOPCURRA		10
#define FAULT_LOOPCURRB		11
#define FAULT_LOOPCURRC		12

#define MAXTEMP		185		// shutdown max temp
#define MAXVOLT		380		// shutdown max bus voltage
#define LOWVOLT		269		// 77 cells at 3.5 v
#define MINVOLT		215		// 77 @ 2.8
#define MINAMPS		-2		// re-charging current limit

void faultInit (void);
int faulted (void);
void fault_reset (void);
void checkFault (int latch);
void faultReasonLoop (int reason);
