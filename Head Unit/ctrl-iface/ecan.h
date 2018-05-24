//
// ecan.h
//
#define RRPM 	256	// rotor rpm id
#define SRPM 	257	// rotor rpm id
#define ACCEL	258	// accel position id
#define VOLTS	259	// bus voltage id
#define AMPS	260	// bus current id
#define FREQ	261	// rotor rpm id
#define AMPL	262	// waveform amplitude id
#define TEMP	263	// controller temp id
#define PHAC	264	// phase A current id
#define PHCC	265	// phase C current id
#define FAULT	266	// FAULT condition id
#define BATP	267	// battery percent id
#define STATE	268	// controller state id
#define LIMIT	270	// LIMIT condition id

#define STARTSNAP	380		// ask to take a snapshot now
#define GETSNAP		381		// fetch snapshot data
#define SNAPHEADER	406
#define SNAPDATA1	407
#define SNAPDATA2	408
#define SNAPCOMP	409
#define TUNEDATA	506
#define BATTDATA	525
#define BATTDATA2	526
#define HTRDATA		545
#define REGENSET	601

#define CMDSEND		356		// can msgid to command the controller

#define SETTORQ		383		// request a set torque value (Qsp)

void can_cmd (int cmd, int data0);
void init_can (void);
void read_can_value (int *data);
