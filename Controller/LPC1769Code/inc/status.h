#include "app_setup.h"


#define SVRRPM	0		// rotor rpm
#define SVSRPM	1		// stator rpm
#define	SVACCEL	2		// accel value
#define	SVTUNE	3		// tune value
#define SVTEMP	4		// temp in deg F
#define	SVPOS	5		// current rotor position
#define SVINDEX	6		// rotor index count
#define SVDIR	7		// rotor direction
#define SVMODE	8		// control mode
#define SVENAB	9		// enable/disable state
#define SVCDIR	10		// commanded direction
#define SVCAP	11
#define SVVOLTS	12		// current measured system voltage
#define SVAMPS	13		// current measured system am draw
#define	SVPROP	14		// proportional tuning const
#define	SVINTEG	15		// integral tuning constant
#define SVCONN	16		// connector state
#define	SVOILP	17		// oil pressure present
#define SVRAWV	18		// last commanded volt value
#define SVRAWF	19		// last commanded freq value
#define SVFAULT	20		// current fault value
#define SVBATP	21		// battery percent remaining
#define SVSTATE	22		// current run state
#define SVPHAC	23		// phase a current
#define SVPHCC	24		// phase c current

#define SV_MAX	25	// number of status values

#define PAGE_WELCOME	1	// initial welcome page
#define PAGE_OPERATE	2	// main operation page
#define PAGE_COMMAND	3	// page to select state change operations
#define PAGE_SELECT		4	// select new display
#define PAGE_POWER		5	// power/battery related page
#define PAGE_TUNE		6	// display and modify tuning parameters
#define PAGE_DIAG1		7	// diagnostic info
#define PAGE_DIAG2		8	// diagnostic info

#define CMDSTART 		1	// begin the start sequence
#define CMDSTOP 		2	// begin the stop sequence
#define CMDCLEAR 		3	// clear faults
#define CMDFORWARD 		4	// set direction to forward
#define CMDREVERSE 		5	// set direction to reverse

#define STARTING 1
#define RUNNING  2
#define STOPPING 3
#define STOPPED  4
#define FAULTED  5

#define HEADCMDSTART	1
#define HEADCMDSTOP		2
#define HEADCMDCLEAR	3
#define HEADCMDCHANGEDIR 4
#define HEADCMDTESTDTR 	5


void setStatVal (int valindex, int32_t value);
void SetPage (uint32_t page);
void selectPage ( void );
void stepPage(void);
void DisplayPage (void);
int buttonstateChanged (int buttonNumber);
void sendHeadData (void);
void sendHeadTune (void);

