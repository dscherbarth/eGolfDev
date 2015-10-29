#define ACCEL_AINDEX	0	// change to 2 to use the car accelerator
#define MAXMODE 5

#define MODEFOC		1
#define MODESVM		2
#define MODESVMCLOS	3
#define MODEMCE		4

void exeControl(void);
void readAnalogControlInputs (void);
void start (void);
void stop (void);
void start_hv_only (void);
void stop_hv_only (void);
float getJog(void);
uint32_t getMode (void);


