#define BUTTON1	8		// top
#define BUTTON2	10
#define BUTTON3	11
#define BUTTON4	12		// bottom

void buttonInit (void);
int buttonIsPressed (int buttonNumber);
int buttonstateChanged (int buttonNumber);
void handleButtons( void );
void setJog (float val);
uint32_t getTorque(void);
void setTorque(uint32_t storque);
