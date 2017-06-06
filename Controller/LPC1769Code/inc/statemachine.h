#define STATE_IDLE			1
#define STATE_STARTING		2
#define STATE_RUNNING		3
#define STATE_STOPPING		4
#define	STATE_STOPPED		5
#define STATE_FAULTED		6
#define STATE_FAULTCLEAR	7
#define STATE_REVERSING		8	// ???

void sm_init (void);
void sm_exe (void);
