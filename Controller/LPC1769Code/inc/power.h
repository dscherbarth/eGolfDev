void precharge (void);
void hvpoweron (void);
void hvpoweroff (void);

void addwatthours (float volts, float amps, float seconds);
void setwatthourmax (float maxwatthour);
float batterypercent (void);
void zerovolts (float volts);
void zeroamps (float amps);
void zerowatthours (void);
uint32_t getampoffset (void);



