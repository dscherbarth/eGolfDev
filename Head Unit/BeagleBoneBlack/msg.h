#define MSGID		276

#define MSGWAVEOFF	0
#define MSGCHKPRESS	1
#define MSGZEROVOLT	2
#define MSGPRECHRG	3
#define MSGHVON		4
#define MSGZEROAMP	5
#define MSGOILON	6
#define MSGINVON	7
#define MSGZEROTHR	8
#define MSGWAVEON	9
#define MSGOILFLT	10
#define MSGOILSWFLT	11
#define MSGFLUXCHG	12
#define MSGGWAVAIL	13
#define MSGRAMPDN	14
#define MSGINVOFF	15
#define MSGOILOFF	16
#define MSGHVOFF	17
#define MSGPRECHGOF	18
#define MSGBLANK	19

void msg_init (GtkWidget *topwin, PangoFontDescription *dfdata);
void clear_msg (void);
void disp_msg (char *msg);
void disp_msg_num (int msgnum);
void fault_init (GtkWidget *topwin, PangoFontDescription *dfdata);
void clear_fault (void);
void disp_fault (char *msg);
void limit_init (GtkWidget *topwin, PangoFontDescription *dfdata);
void clear_limit (void);
void disp_limit (char *msg);
