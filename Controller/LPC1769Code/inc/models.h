//
// models.h
//
#define LITTLE	1	// 1/4hp test motor
#define MEDIUM	2	// 1 1/2hp test motor
#define BIG		3	// egolf real thing

typedef struct _paramtable {
	uint32_t	input;
	uint32_t	result;
} param_table;


typedef struct  tag_params
{
	char 	name[30];
	int 	poles;		// really pairs
	int		maxrpm;		// for limiting
	float	tr_factor;	// (1/2*pi*tr)
	float	PhIgain;	// phase current gain factor (needs to shoot for max I == 32 unless we change fixed cfg)
	float	PhIgain2;	// phase current gain factor 2
	float	maxCurrent;	// for current limiter (should be lower than fault limit)
	float	faultCurrent;	// absolute limit
	int		postinvclarkegain;	// shift down by after invclarke to get 0 - 1 range
	float	pid_d_p;	// tuning for d p term
	float	pid_d_i;	// tuning for d i term
	float	pid_d_d;	// tuning for d d term
	float	pid_q_p;	// tuning for q p term
	float	pid_q_i;	// tuning for q i term
	float	pid_q_d;	// tuning for q d term
	int		encoder_cpr;	// counts per rev on the encoder
	param_table		*mag_table;
	param_table		*tq_table;

} motor_t;

void 		ModelSelect (int motor);
int 		MgetEncCPR (void);
int 		MgetPoles (void);
float 		MgetTimeConst (void);
void		MsetTimeConst (float tc);
uint32_t 	MgetMagVal (uint32_t rpm);
uint32_t 	MgetAccSq (uint32_t torque);
int MgetRPMLimit (void);
void MgetDPID (float *p, float *i, float *d);
void MgetQPID (float *p, float *i, float *d);
void MsetDPID (float *p, float *i, float *d);
void MsetQPID (float *p, float *i, float *d);
void Isvalset (int val);
float MgetDgain (void);
void MsetDgain (float Dg);
float MgetIgain (void);
void MsetIgain (float Igain);
float MgetIgain2 (void);
int MgetPostgain (void);
void MsetPostgain (int pgain);
float MgetBusILimit (void);
float MgetBusIFLimit (void);
