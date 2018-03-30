// 
// buttons.h
//
#define STARTING 1
#define RUNNING  2
#define STOPPING 3
#define STOPPED  4
#define FAULTED  5

#define HEADCMDSTART	1
#define HEADCMDSTOP		2
#define HEADCMDCLEAR	3
#define HEADCMDCHANGEDIR 4
#define HEADCMDTEST 5

#define GETDISCHFILE 		1
#define SETDISCHFILE 		2
#define CLEARDISCHFILE 		3
#define DISCHFILENOACTION 	4

void create_buttons (GtkWidget *window, GtkWidget *fixed);
void set_cmd_button_state (int state, int fault);
void hide_buttons ();
int trend_active (void);
int get_disch_action(int what);