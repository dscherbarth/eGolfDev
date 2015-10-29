#define TESTDUR		100		// milliseconds to keep the output pairs on
#define TESTVAL		55		// 5% (if buss voltage is 300 this is 15v, should be safe)

typedef struct phases {
	uint32_t		abA, bcA, acA;	//	phase A current measured while across ab, bc and ac winding
	uint32_t		abC, bcC, acC;	//	phase C current measured while across ab, bc and ac winding
	uint32_t		pAz, pCz, Bz;	//	phase A, C and buss current zero values
} phase_t;

void TestPhaseCurrent(phase_t	*p);
void displayPTest (phase_t *p);
int  TestValidate (phase_t *p);
