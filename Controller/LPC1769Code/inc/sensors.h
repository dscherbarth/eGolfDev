#define SUPPLYVOLTAGELOWRANGE 10.0
#define SUPPLYVOLTAGEHIGHRANGE 370.0
#define SUPPLYVOLTAGE 		250.0		// soon read this directly from the isolated voltage transducer
#define APPARENTMAXVOLTS	125.0	// seems like this should be more than 75...


void updateSensors (void);
float getBusvolt (void);
float GetSupVolt ();
float getBuscurrent (void);
float getTemp (void);
float getTunevalue (void);
float getAccelvalue (void);
float getAccelRPM (void);
void saveOffsets(void);


