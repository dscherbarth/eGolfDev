#define FORWARD		1
#define REVERSE		2


void pwm_init(void);
void set_wave_direction(uint32_t dir);
uint32_t get_wave_direction(void);
void set_wave_direction(uint32_t dir);
void fvpwmDisable( void );
void fvpwmEnable( void );
void set_mcpwm (int a, int b, int c);


