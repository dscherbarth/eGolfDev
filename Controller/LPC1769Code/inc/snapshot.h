struct snaprec {
	uint16_t	S1;
	uint16_t	S2;
	uint16_t	S3;
	uint16_t	S4;
	uint16_t	S5;
	uint16_t	S6;
};

#define SNAPMAX 2000	// .2 seconds at 10000/sec

void snapInit(void);
void takeSnapshot (uint16_t qset, uint16_t slip, uint16_t snapRes);
void addSRec (uint16_t I0, uint16_t I1, uint16_t Pos, uint16_t a, uint16_t b, uint16_t c);
int  moreSnap( void );
void sendSnap ( void );
void SnapFault ( void );

