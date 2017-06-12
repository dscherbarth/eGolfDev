//
// eGolf ewin.c
//

#include <gtk/gtk.h>
#include <gtkdatabox.h>
#include <gtkdatabox_points.h>
#include <gtkdatabox_ruler.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <time.h>
#include <errno.h>
 
#include <linux/can.h>
#include <linux/can/raw.h>

#include "ecan.h"
#include "ewin.h"
#include "msg.h"
#include "buttons.h"
#include "scope.h"

#include <stdio.h>
#include <stdlib.h>

extern gfloat x[], y[];
extern gfloat yv[];
extern GtkDataboxGraph *graph1;
extern GtkDataboxGraph *graph2;
extern GtkWidget *box;


static int data_current = FALSE;
static float bp=0.15, lastbp=99.0;
float dischargebp = 0.0;
extern int getdisch, setdisch;
int cleardisch = 0;

void *snap_get (void *ptr);

// label names and data fields
#define TAGSZ	20
#define BUFSZ	30

struct live_data {
	char tag[TAGSZ];
	char valbuf[BUFSZ];
	int val;
	int canid;
	int xpos;
	int ypos;
	GtkWidget *taglab;
	GtkWidget *vallab;
};

GtkWidget *load_cell_h;
GtkWidget *load_cell_b;
GtkWidget *recharge_b;

GtkWidget *snapres;
GtkWidget *torqueres;

struct live_data	l_d[] = {
	{"rrpm", 	"", 0, RRPM, 	100, 90,	NULL, NULL },
	{"srpm", 	"", 0, SRPM, 	200, 90,	NULL, NULL },
	{"volts", 	"", 0, VOLTS,	300, 90,	NULL, NULL },
	{"amps", 	"", 0, AMPS,	400, 90,	NULL, NULL },
	{"temp", 	"", 0, TEMP,	500, 90,	NULL, NULL },
	{"phAc", 	"", 0, PHAC,	600, 90,	NULL, NULL },
	{"phCc", 	"", 0, PHCC,	700, 90,	NULL, NULL },
	{"Batt", 	"", 0, BATP,	800, 90,	NULL, NULL },
	{"", 		"", 0, 0, 		0, 0, 		NULL, NULL }

	
};

#define MAXCSVTREND	10000
short int trendrpm  [MAXCSVTREND+1];
short int trendqv   [MAXCSVTREND+1];
short int trendvolts[MAXCSVTREND+1];
short int trendamps [MAXCSVTREND+1];
short int trendtemp [MAXCSVTREND+1];
time_t 	trendtime   [MAXCSVTREND+1];;
int trendpos = 0;
int trendcollecting = 0;
short int rpm, amps, volts;

void SaveTrendData(struct can_frame *frame)
{
	// if this is a trend point add it to the current trend record
	switch (frame->can_id)
	{
	case RRPM :
		rpm = trendrpm[trendpos] = frame->data[0] + (frame->data[1] << 8);
		if (rpm < 10) rpm = 10;
		break;
	case AMPS :
		amps = trendamps[trendpos] = frame->data[0] + (frame->data[1] << 8);
		break;
	case VOLTS :
		volts = trendvolts[trendpos] = frame->data[0] + (frame->data[1] << 8);
		break;
	case TEMP :
		trendtemp[trendpos] = frame->data[0] + (frame->data[1] << 8);
		break;
	case PHCC :
		trendqv[trendpos] = frame->data[0] + (frame->data[1] << 8);
		trendtime[trendpos] = time (NULL);
		StepTrend();

		break;
	}

}

void TrendStart (void)
{
	trendcollecting = 1;
}

void StepTrend (void)
{
	// step to the next trend record
	if (trendcollecting && (trendpos < MAXCSVTREND))
		{
		trendpos++;
		}
}

#define TTESTSPEEDTHRESHOLD 700	// arbitrary test endpoint
#define TTESTSIZE 240		// max test duration is 60 seconds

static int ttestrunning = 0;
static int ttestindex = 0;
static int ttestaccpos [ TTESTSIZE ];
static int ttestspeed [ TTESTSIZE ];

void setTorqueResult (float result)
{
	char temp[30];

//	gdk_threads_enter();
	if (result == -1.0)
	{
		sprintf (temp, "*****");	// indicate test running
	}
	else if (result == -2.0)
	{
		sprintf (temp, "#####");	// indicate test canceled
	}
	else if (result == -3.0)
	{
		sprintf (temp, "<%d>", ttestindex); // test progress
	}
	else
	{
		sprintf (temp, "%6.2f", result);
	}
	gtk_label_set (GTK_LABEL (torqueres), temp);	
//	gdk_threads_leave();
}

void startTorquetest (void)
{
	ttestrunning = 1;
	ttestindex = 0;
	setTorqueResult (-1.0);
	
}

void checkFininshTorquetest (int accpos, int speed)
{
	int accposint, speedint, i;
	float tresult = 0;
	
	
	// add new data to the list
	if (ttestrunning)
	{
		if (ttestindex < TTESTSIZE - 1)
		{
			ttestaccpos[ttestindex] = accpos;
			ttestspeed[ttestindex] = speed;
			ttestindex++;
			setTorqueResult (-3.0);
		}
		else
		{
			// data overrun, cancel test
			setTorqueResult (-2.0);
			
			// stop the test
			ttestrunning = 0;
		}
		
		// if we have passed the threshhold calculate result and display
		if (speed > TTESTSPEEDTHRESHOLD)
		{
			accposint = speedint = 0;
			for (i=0; i<ttestindex; i++)
			{
				accposint += ttestaccpos [i];
				speedint += ttestspeed [i];
			}
			if (accposint == 0) accposint = 1;	// no divide by zero
			tresult = (float)speedint / (float)accposint;
			setTorqueResult (tresult);
			
			// stop the test
			ttestrunning = 0;
		}
	}
}

void TrendWrite(void)
{
	FILE	*trendf;
	char tstr[200], fname[200], data[200];
	time_t t;
	struct tm *tmp;
	int i;


	// write the contents of the trend data to a .csv file
	// discover what the next trend data file will be
	t = time (NULL);
	tmp = localtime (&t);
	strftime (tstr, 200, "%F_%H:%M:%S", tmp);

	sprintf (fname, "TREND_%s.csv", tstr);

	// open the file and write headers
	trendf = fopen(fname, "w+");
	if (trendf != NULL)
	{

		sprintf (data, "RPM, AMPS, VOLTS, TEMP, QV\n");
		fwrite(data, strlen(data), 1, trendf);

		// write the data records
		for(i=0; i<trendpos; i++)
		{
			tmp = localtime (&trendtime[i]);
			strftime (tstr, 200, "%H:%M:%S", tmp);
			sprintf (data, "%d, %d, %d, %d, %d, %s\n", trendrpm[i], trendamps[i], trendvolts[i], trendtemp[i], trendqv[i], tstr);
			fwrite(data, strlen(data), 1, trendf);
		}	
		// close the file
		fclose (trendf);
	}

	// reset trend pos
	trendpos = 0;
	trendcollecting = 0;
}

GtkWidget	*prog;

GtkWidget	*prog_kw;
GtkWidget	*prog_kw_lag;
GtkWidget	*prog_kw_regen;
GtkWidget	*prog_kw_regen_lag;

void *update_window (void *ptr);
char *fmsg[] = {"                    ", "OVER TEMP", "OVER CURRENT", "OVER VOLT", "UNDER VOLT", "OIL PRESSURE", "PRECHARGE", "CURR FAULT","PHASE TEST FAIL", "RELAYPOS", "LOOP CURR A", "LOOP CURR B", "LOOP CURR C"};
char *lmsg[] = {"                    ", "Current", "RPM", "Temperature", "HW Current", "SW Current", "                "};
void handle_tune_data (int);
static tuneindex = 0;
int state;

void live_data_update (struct can_frame *frame)
{
	int i;
	short	dv;
	static char temp[50];
	static int limitv = 0;
	static int faultv = 0;
	
	
	switch (frame->can_id)
	{
		case TUNEDATA :
			gdk_threads_enter();
			dv = frame->data[0] + (frame->data[1] << 8);
			handle_tune_data (dv);
			gdk_threads_leave();
			tuneindex++;
			break;
			
		case SNAPHEADER :
		case SNAPDATA1 :
		case SNAPDATA2 :
		case SNAPCOMP :
			handle_snap_frame (frame);
			break;
			
		case MSGID :
			// this is a request to display a message
			gdk_threads_enter();
			disp_msg_num (frame->data[0]);
			gdk_threads_leave();
			break;
			
		case STATE :
			gdk_threads_enter();
			state = frame->data[0];
			set_cmd_button_state (frame->data[0], faultv);
			gdk_threads_leave();
			break;
			
		default:
			SaveTrendData(frame);
			for(i=0; i<MAXLD; i++)
			{
				if (l_d[i].canid == 0)
				{
					// at the end
					break;
				}
				
				if (frame->can_id == BATP)
				{
					data_current = TRUE;

					// update the battery condition
					dv = frame->data[0] + (frame->data[1] << 8);
					if (dv < 0) dv = 0; if (dv > 100) dv = 100;
					bp = ((double)dv) / 100.0;
				
					if (bp != lastbp)
					{
						// change, update the display
						lastbp = bp;
					}
				}
				
				// handle faults
				if (frame->can_id == FAULT)
				{
					gdk_threads_enter();
					if (frame->data[0] > 10 || frame->data[0] < 0)
					{
						disp_fault ("Unknown Fault");
					}
					else
					{
						disp_fault (fmsg[frame->data[0]]);
					}
					faultv = frame->data[0];
					gdk_threads_leave();
				}
				
				if (frame->can_id == LIMIT)
				{
					gdk_threads_enter();
					if (frame->data[0] > 5 || frame->data[0] < 0)
					{
						disp_fault ("Unknown Limit");
					}
					else
					{
						disp_fault (lmsg[frame->data[0]]);
					}
					limitv = frame->data[0];
					gdk_threads_leave();
				}
				
				if (frame->can_id == l_d[i].canid)
				{
					data_current = TRUE;

					// this is the one

					dv = frame->data[0] + (frame->data[1] << 8);
					l_d[i].val = dv;
					sprintf (temp, "%04d", dv);

					// only update the window on a change
					if (strcmp (temp, l_d[i].valbuf) != 0)
					{
						strcpy (l_d[i].valbuf, temp);
					}
					break;
				}
			}
	}
}

GtkWidget *tunelab1;
GtkWidget *tunelab2;
GtkWidget *tunelab3;
GtkWidget *tunelab4;
GtkWidget *tunelab5;
GtkWidget *tunelab6;
GtkWidget *tunelab7;
GtkWidget *tunelab8;
GtkWidget *tunelab9;
GtkWidget *tunelab10;
GtkWidget *tunelab11;
GtkWidget *tunelab12;

GtkWidget *tuneval1;
GtkWidget *tuneval2;
GtkWidget *tuneval3;
GtkWidget *tuneval4;
GtkWidget *tuneval5;
GtkWidget *tuneval6;
GtkWidget *tuneval7;
GtkWidget *tuneval8;
GtkWidget *tuneval9;

GtkWidget *tune1up;
GtkWidget *tune1down;
GtkWidget *tune2up;
GtkWidget *tune2down;
GtkWidget *tune3up;
GtkWidget *tune3down;
GtkWidget *tune4up;
GtkWidget *tune4down;
GtkWidget *tune5up;
GtkWidget *tune5down;
GtkWidget *tune6up;
GtkWidget *tune6down;
GtkWidget *tune7up;
GtkWidget *tune7down;
GtkWidget *tune8up;
GtkWidget *tune8down;
GtkWidget *tune9up;
GtkWidget *tune9down;

GtkWidget *tuneset;
GtkWidget *tuneget;
GtkWidget *tunetest;

// placeholder tune values
int tunedp = 0, tunedi = 0, tunedd = 0;
int tuneqp = 0, tuneqi = 0, tuneqd = 0;
int tunetr = 0; tunerf = 0;	tuneda = 0;

void display_tune (int display)
{
	if (display)
	{
		gtk_widget_show ((GtkWidget *)tunelab1);
		gtk_widget_show ((GtkWidget *)tunelab2);
		gtk_widget_show ((GtkWidget *)tunelab3);
		gtk_widget_show ((GtkWidget *)tunelab4);
		gtk_widget_show ((GtkWidget *)tunelab5);
		gtk_widget_show ((GtkWidget *)tunelab6);
		gtk_widget_show ((GtkWidget *)tunelab7);
		gtk_widget_show ((GtkWidget *)tunelab8);
		gtk_widget_show ((GtkWidget *)tunelab9);
		gtk_widget_show ((GtkWidget *)tunelab10);
		gtk_widget_show ((GtkWidget *)tunelab11);
		gtk_widget_show ((GtkWidget *)tunelab12);
		
		gtk_widget_show ((GtkWidget *)tuneval1);
		gtk_widget_show ((GtkWidget *)tuneval2);
		gtk_widget_show ((GtkWidget *)tuneval3);
		gtk_widget_show ((GtkWidget *)tuneval4);
		gtk_widget_show ((GtkWidget *)tuneval5);
		gtk_widget_show ((GtkWidget *)tuneval6);
		gtk_widget_show ((GtkWidget *)tuneval7);
		gtk_widget_show ((GtkWidget *)tuneval8);
		gtk_widget_show ((GtkWidget *)tuneval9);

		gtk_widget_show ((GtkWidget *)tune1up);
		gtk_widget_show ((GtkWidget *)tune1down);
		gtk_widget_show ((GtkWidget *)tune2up);
		gtk_widget_show ((GtkWidget *)tune2down);
		gtk_widget_show ((GtkWidget *)tune3up);
		gtk_widget_show ((GtkWidget *)tune3down);
		gtk_widget_show ((GtkWidget *)tune4up);
		gtk_widget_show ((GtkWidget *)tune4down);
		gtk_widget_show ((GtkWidget *)tune5up);
		gtk_widget_show ((GtkWidget *)tune5down);
		gtk_widget_show ((GtkWidget *)tune6up);
		gtk_widget_show ((GtkWidget *)tune6down);
		gtk_widget_show ((GtkWidget *)tune7up);
		gtk_widget_show ((GtkWidget *)tune7down);
		gtk_widget_show ((GtkWidget *)tune8up);
		gtk_widget_show ((GtkWidget *)tune8down);
		gtk_widget_show ((GtkWidget *)tune9up);
		gtk_widget_show ((GtkWidget *)tune9down);

		gtk_widget_show ((GtkWidget *)tuneset);
		gtk_widget_show ((GtkWidget *)tuneget);
	}
	else
	{
		gtk_widget_hide ((GtkWidget *)tunelab1);
		gtk_widget_hide ((GtkWidget *)tunelab2);
		gtk_widget_hide ((GtkWidget *)tunelab3);
		gtk_widget_hide ((GtkWidget *)tunelab4);
		gtk_widget_hide ((GtkWidget *)tunelab5);
		gtk_widget_hide ((GtkWidget *)tunelab6);
		gtk_widget_hide ((GtkWidget *)tunelab7);
		gtk_widget_hide ((GtkWidget *)tunelab8);
		gtk_widget_hide ((GtkWidget *)tunelab9);
		gtk_widget_hide ((GtkWidget *)tunelab10);
		gtk_widget_hide ((GtkWidget *)tunelab11);
		gtk_widget_hide ((GtkWidget *)tunelab12);
		
		gtk_widget_hide ((GtkWidget *)tuneval1);
		gtk_widget_hide ((GtkWidget *)tuneval2);
		gtk_widget_hide ((GtkWidget *)tuneval3);
		gtk_widget_hide ((GtkWidget *)tuneval4);
		gtk_widget_hide ((GtkWidget *)tuneval5);
		gtk_widget_hide ((GtkWidget *)tuneval6);
		gtk_widget_hide ((GtkWidget *)tuneval7);
		gtk_widget_hide ((GtkWidget *)tuneval8);
		gtk_widget_hide ((GtkWidget *)tuneval9);
		
		gtk_widget_hide ((GtkWidget *)tune1up);
		gtk_widget_hide ((GtkWidget *)tune1down);
		gtk_widget_hide ((GtkWidget *)tune2up);
		gtk_widget_hide ((GtkWidget *)tune2down);
		gtk_widget_hide ((GtkWidget *)tune3up);
		gtk_widget_hide ((GtkWidget *)tune3down);
		gtk_widget_hide ((GtkWidget *)tune4up);
		gtk_widget_hide ((GtkWidget *)tune4down);
		gtk_widget_hide ((GtkWidget *)tune5up);
		gtk_widget_hide ((GtkWidget *)tune5down);
		gtk_widget_hide ((GtkWidget *)tune6up);
		gtk_widget_hide ((GtkWidget *)tune6down);
		gtk_widget_hide ((GtkWidget *)tune7up);
		gtk_widget_hide ((GtkWidget *)tune7down);
		gtk_widget_hide ((GtkWidget *)tune8up);
		gtk_widget_hide ((GtkWidget *)tune8down);
		gtk_widget_hide ((GtkWidget *)tune9up);
		gtk_widget_hide ((GtkWidget *)tune9down);

		gtk_widget_hide ((GtkWidget *)tuneset);
		gtk_widget_hide ((GtkWidget *)tuneget);

	}
	
}

static PangoFontDescription *dfsmall;
static PangoFontDescription *dfdata;
static GtkWidget *window;
static GtkWidget *fixed;

void tunedisp(void)
{
	char temp[30];

	
	sprintf (temp, "%04d", tunedp);
	gtk_label_set (GTK_LABEL (tuneval1), temp);	
	sprintf (temp, "%04d", tunedi);
	gtk_label_set (GTK_LABEL (tuneval2), temp);	
	sprintf (temp, "%04d", tunedd);
	gtk_label_set (GTK_LABEL (tuneval3), temp);	
	sprintf (temp, "%04d", tuneqp);
	gtk_label_set (GTK_LABEL (tuneval4), temp);	
	sprintf (temp, "%04d", tuneqi);
	gtk_label_set (GTK_LABEL (tuneval5), temp);	
	sprintf (temp, "%04d", tuneqd);
	gtk_label_set (GTK_LABEL (tuneval6), temp);	
	sprintf (temp, "%04d", tunetr);
	gtk_label_set (GTK_LABEL (tuneval7), temp);	
	sprintf (temp, "%04d", tunerf);
	gtk_label_set (GTK_LABEL (tuneval8), temp);	
	sprintf (temp, "%04d", tuneda);
	gtk_label_set (GTK_LABEL (tuneval9), temp);	

}

void handle_tune_data (int tunev)
{	
	switch (tuneindex)
	{
	case 0:
		tunedp = tunev;
		break;
		
	case 1:
		tunedi = tunev;
		break;
		
	case 2:
		tunedd = tunev;
		break;
		
	case 3:
		tuneqp = tunev;
		break;
		
	case 4:
		tuneqi = tunev;
		break;
		
	case 5:
		tuneqd = tunev;
		break;
		
	case 6:
		tunetr = tunev;
		break;

	case 7:
		tunerf = tunev;
		break;

	case 8:
		tuneda = tunev;
		break;
	}
	tunedisp();
}


void tunetest_action (void)
{
	
	
	// fetch tune values and update the control values
	can_cmd (CMDSEND, HEADCMDTEST);

}
static int snap_res_val = 0;
int get_snapres (void)
{
	return snap_res_val;
}

void toggle_snapres(void)
{
	snap_res_val++;
	if (snap_res_val > 4) snap_res_val = 0;
	switch (snap_res_val)
	{
	case 0:
		gtk_label_set (GTK_LABEL (snapres), ".25 S ");	
		break;
	case 1:
		gtk_label_set (GTK_LABEL (snapres), ".5 S  ");	
		break;
	case 2:
		gtk_label_set (GTK_LABEL (snapres), "2.5 S ");	
		break;
	case 3:
		gtk_label_set (GTK_LABEL (snapres), "5 S   ");	
		break;
	case 4:
		gtk_label_set (GTK_LABEL (snapres), "10 S  ");	
		break;
	}
}

void tuneget_action (void)
{
	
	
	// fetch tune values and update the control values
	can_cmd (390, 0);	// make the request

	gtk_label_set (GTK_LABEL (tuneval1), "YYYY");	

	// wait for responses
	tuneindex = 0;

}

float	max = 0.0;
void load_cell_action (void)
{
	max = 0.0;
}

void recharge_action (void)
{
	cleardisch = 1;
}

void tuneset_action (void)
{
	// fetch tune values from controls and update the nxp
	can_cmd (507, tunedp);
	usleep(100000);
	can_cmd (508, tunedi);
	usleep(100000);
	can_cmd (509, tunedd);
	usleep(100000);
	can_cmd (510, tuneqp);
	usleep(100000);
	can_cmd (511, tuneqi);
	usleep(100000);
	can_cmd (512, tuneqd);
	usleep(100000);
	can_cmd (513, tunetr);
	usleep(100000);
	can_cmd (514, tunerf);
	usleep(100000);
	can_cmd (515, tuneda);
	usleep(100000);
}

#define VALINC 10

void tune1up_action (void) {tunedp += VALINC; if (tunedp > 1000) tunedp = 1000;tunedisp();}
void tune1down_action (void) {tunedp -= VALINC; if (tunedp < 0) tunedp = 0;tunedisp();}
void tune2up_action (void) {tunedi += VALINC; if (tunedi > 1000) tunedi = 1000;tunedisp();}
void tune2down_action (void) {tunedi -= VALINC; if (tunedi < 0) tunedi = 0;tunedisp();}
void tune3up_action (void) {tunedd += 100; if (tunedd > 10000) tunedd = 10000;tunedisp();}
void tune3down_action (void) {tunedd -= 100; if (tunedd < 0) tunedd = 0;tunedisp();}
void tune4up_action (void) {tuneqp += VALINC; if (tuneqp > 1000) tuneqp = 1000;tunedisp();}
void tune4down_action (void) {tuneqp -= VALINC; if (tuneqp < 0) tuneqp = 0;tunedisp();}
void tune5up_action (void) {tuneqi += VALINC; if (tuneqi > 1000) tuneqi = 1000;tunedisp();}
void tune5down_action (void) {tuneqi -= VALINC; if (tuneqi < 0) tuneqi = 0;tunedisp();}
void tune6up_action (void) {tuneqd += VALINC; if (tuneqd > 100000) tuneqd = 100000;tunedisp();}
void tune6down_action (void) {tuneqd -= VALINC; if (tuneqd < 0) tuneqd = 0;tunedisp();}
void tune7up_action (void){tunetr += VALINC; if (tunetr > 1000) tunetr = 1000;tunedisp();}
void tune7down_action (void) {tunetr -= VALINC; if (tunetr < 0) tunetr = 0;tunedisp();}
void tune8up_action (void){tunerf += 1; if (tunerf > 15) tunetr = 15;tunedisp();}
void tune8down_action (void) {tunerf -= 1; if (tunerf < 0) tunetr = 0;tunedisp();}
void tune9up_action (void){tuneda += 1; if (tuneda > 20) tuneda = 20;tunedisp();}
void tune9down_action (void) {tuneda -= 1; if (tuneda < 0) tuneda = 0;tunedisp();}

void init_tune (GtkWidget *topwin)
{
		// create the tune labels
		tunelab1 = gtk_label_new ("TUNE");
		tunelab2 = gtk_label_new ("D");
		tunelab3 = gtk_label_new ("Q");
		tunelab4 = gtk_label_new ("Prop");
		tunelab5 = gtk_label_new ("Int");
		tunelab6 = gtk_label_new ("DTable");
		tunelab7 = gtk_label_new ("Prop");
		tunelab8 = gtk_label_new ("Int");
		tunelab9 = gtk_label_new ("PFactor");
		tunelab10 = gtk_label_new ("Tr");
		tunelab11 = gtk_label_new ("Current Thresh");
		tunelab12 = gtk_label_new ("IGain");
		gtk_widget_modify_font(tunelab1, dfdata);  
		gtk_widget_modify_font(tunelab2, dfdata);  
		gtk_widget_modify_font(tunelab3, dfdata);  
		gtk_widget_modify_font(tunelab4, dfsmall);  
		gtk_widget_modify_font(tunelab5, dfsmall);  
		gtk_widget_modify_font(tunelab6, dfsmall);  
		gtk_widget_modify_font(tunelab7, dfsmall);  
		gtk_widget_modify_font(tunelab8, dfsmall);  
		gtk_widget_modify_font(tunelab9, dfsmall);  
		gtk_widget_modify_font(tunelab10, dfdata);  
		gtk_widget_modify_font(tunelab11, dfsmall);  
		gtk_widget_modify_font(tunelab12, dfsmall);  
		gtk_fixed_put(GTK_FIXED(topwin), tunelab1, 350, 150);
		gtk_fixed_put(GTK_FIXED(topwin), tunelab2, 200, 200);
		gtk_fixed_put(GTK_FIXED(topwin), tunelab3, 450, 200);
		gtk_fixed_put(GTK_FIXED(topwin), tunelab4, 200, 250);
		gtk_fixed_put(GTK_FIXED(topwin), tunelab5, 200, 300);
		gtk_fixed_put(GTK_FIXED(topwin), tunelab6, 200, 350);
		gtk_fixed_put(GTK_FIXED(topwin), tunelab7, 450, 250);
		gtk_fixed_put(GTK_FIXED(topwin), tunelab8, 450, 300);
		gtk_fixed_put(GTK_FIXED(topwin), tunelab9, 450, 350);
		gtk_fixed_put(GTK_FIXED(topwin), tunelab10, 350, 400);
		gtk_fixed_put(GTK_FIXED(topwin), tunelab11, 200, 450);
		gtk_fixed_put(GTK_FIXED(topwin), tunelab12, 450, 450);
		
		// create the live data field
		tuneval1 = gtk_label_new ("xxxx");
		gtk_widget_modify_font(tuneval1, dfdata);  
		gtk_fixed_put(GTK_FIXED(topwin), tuneval1, 275, 250);
		tuneval2 = gtk_label_new ("xxxx");
		gtk_widget_modify_font(tuneval2, dfdata);  
		gtk_fixed_put(GTK_FIXED(topwin), tuneval2, 275, 300);
		tuneval3 = gtk_label_new ("xxxx");
		gtk_widget_modify_font(tuneval3, dfdata);  
		gtk_fixed_put(GTK_FIXED(topwin), tuneval3, 275, 350);

		tuneval4 = gtk_label_new ("xxxx");
		gtk_widget_modify_font(tuneval4, dfdata);  
		gtk_fixed_put(GTK_FIXED(topwin), tuneval4, 525, 250);
		tuneval5 = gtk_label_new ("xxxx");
		gtk_widget_modify_font(tuneval5, dfdata);  
		gtk_fixed_put(GTK_FIXED(topwin), tuneval5, 525, 300);
		tuneval6 = gtk_label_new ("xxxx");
		gtk_widget_modify_font(tuneval6, dfdata);  
		gtk_fixed_put(GTK_FIXED(topwin), tuneval6, 525, 350);

		tuneval7 = gtk_label_new ("xxxx");
		gtk_widget_modify_font(tuneval7, dfdata);  
		gtk_fixed_put(GTK_FIXED(topwin), tuneval7, 425, 400);

		tuneval8 = gtk_label_new ("xxxx");
		gtk_widget_modify_font(tuneval8, dfdata);  
		gtk_fixed_put(GTK_FIXED(topwin), tuneval8, 275, 450);
		tuneval9 = gtk_label_new ("xxxx");
		gtk_widget_modify_font(tuneval9, dfdata);  
		gtk_fixed_put(GTK_FIXED(topwin), tuneval9, 525, 450);

		// create value adjust buttons
		tune1up = gtk_button_new_with_label("^^");
		gtk_fixed_put(GTK_FIXED(fixed), tune1up, 365, 250);
		gtk_widget_set_size_request(tune1up, 35, 30);
		tune1down = gtk_button_new_with_label("vv");
		gtk_fixed_put(GTK_FIXED(fixed), tune1down, 400, 250);
		gtk_widget_set_size_request(tune1down, 35, 30);
		
		tune2up = gtk_button_new_with_label("^^");
		gtk_fixed_put(GTK_FIXED(fixed), tune2up, 365, 300);
		gtk_widget_set_size_request(tune2up, 35, 30);
		tune2down = gtk_button_new_with_label("vv");
		gtk_fixed_put(GTK_FIXED(fixed), tune2down, 400, 300);
		gtk_widget_set_size_request(tune2down, 35, 30);
		
		tune3up = gtk_button_new_with_label("^^");
		gtk_fixed_put(GTK_FIXED(fixed), tune3up, 365, 350);
		gtk_widget_set_size_request(tune3up, 35, 30);
		tune3down = gtk_button_new_with_label("vv");
		gtk_fixed_put(GTK_FIXED(fixed), tune3down, 400, 350);
		gtk_widget_set_size_request(tune3down, 35, 30);

		tune8up = gtk_button_new_with_label("^^");
		gtk_fixed_put(GTK_FIXED(fixed), tune8up, 365, 450);
		gtk_widget_set_size_request(tune8up, 35, 30);
		tune8down = gtk_button_new_with_label("vv");
		gtk_fixed_put(GTK_FIXED(fixed), tune8down, 400, 450);
		gtk_widget_set_size_request(tune8down, 35, 30);
		
		tune4up = gtk_button_new_with_label("^^");
		gtk_fixed_put(GTK_FIXED(fixed), tune4up, 615, 250);
		gtk_widget_set_size_request(tune4up, 35, 30);
		tune4down = gtk_button_new_with_label("vv");
		gtk_fixed_put(GTK_FIXED(fixed), tune4down, 650, 250);
		gtk_widget_set_size_request(tune4down, 35, 30);
		
		tune5up = gtk_button_new_with_label("^^");
		gtk_fixed_put(GTK_FIXED(fixed), tune5up, 615, 300);
		gtk_widget_set_size_request(tune5up, 35, 30);
		tune5down = gtk_button_new_with_label("vv");
		gtk_fixed_put(GTK_FIXED(fixed), tune5down, 650, 300);
		gtk_widget_set_size_request(tune5down, 35, 30);
		
		tune6up = gtk_button_new_with_label("^^");
		gtk_fixed_put(GTK_FIXED(fixed), tune6up, 615, 350);
		gtk_widget_set_size_request(tune6up, 35, 30);
		tune6down = gtk_button_new_with_label("vv");
		gtk_fixed_put(GTK_FIXED(fixed), tune6down, 650, 350);
		gtk_widget_set_size_request(tune6down, 35, 30);
		
		tune9up = gtk_button_new_with_label("^^");
		gtk_fixed_put(GTK_FIXED(fixed), tune9up, 615, 450);
		gtk_widget_set_size_request(tune9up, 35, 30);
		tune9down = gtk_button_new_with_label("vv");
		gtk_fixed_put(GTK_FIXED(fixed), tune9down, 650, 450);
		gtk_widget_set_size_request(tune9down, 35, 30);

		tune7up = gtk_button_new_with_label("^^");
		gtk_fixed_put(GTK_FIXED(fixed), tune7up, 515, 400);
		gtk_widget_set_size_request(tune7up, 35, 30);
		tune7down = gtk_button_new_with_label("vv");
		gtk_fixed_put(GTK_FIXED(fixed), tune7down, 550, 400);
		gtk_widget_set_size_request(tune7down, 35, 30);

		gtk_signal_connect (GTK_OBJECT (tune1up), "clicked", GTK_SIGNAL_FUNC (tune1up_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune1down), "clicked", GTK_SIGNAL_FUNC (tune1down_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune2up), "clicked", GTK_SIGNAL_FUNC (tune2up_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune2down), "clicked", GTK_SIGNAL_FUNC (tune2down_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune3up), "clicked", GTK_SIGNAL_FUNC (tune3up_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune3down), "clicked", GTK_SIGNAL_FUNC (tune3down_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune4up), "clicked", GTK_SIGNAL_FUNC (tune4up_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune4down), "clicked", GTK_SIGNAL_FUNC (tune4down_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune5up), "clicked", GTK_SIGNAL_FUNC (tune5up_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune5down), "clicked", GTK_SIGNAL_FUNC (tune5down_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune6up), "clicked", GTK_SIGNAL_FUNC (tune6up_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune6down), "clicked", GTK_SIGNAL_FUNC (tune6down_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune7up), "clicked", GTK_SIGNAL_FUNC (tune7up_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune7down), "clicked", GTK_SIGNAL_FUNC (tune7down_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune8up), "clicked", GTK_SIGNAL_FUNC (tune8up_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune8down), "clicked", GTK_SIGNAL_FUNC (tune8down_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune9up), "clicked", GTK_SIGNAL_FUNC (tune9up_action), NULL);
		gtk_signal_connect (GTK_OBJECT (tune9down), "clicked", GTK_SIGNAL_FUNC (tune9down_action), NULL);

		// toggled snap resolution display
		snapres = gtk_label_new (".25 S");
		gtk_widget_modify_font(snapres, dfdata);  
		gtk_fixed_put(GTK_FIXED(topwin), snapres, 800, 480);
		
		// torque test result
		torqueres = gtk_label_new ("-----");
		gtk_widget_modify_font(torqueres, dfdata);  
		gtk_fixed_put(GTK_FIXED(topwin), torqueres, 875, 375);
		
		tuneset = gtk_button_new_with_label("SET");
		gtk_fixed_put(GTK_FIXED(fixed), tuneset, 500, 150);
		gtk_widget_set_size_request(tuneset, 40, 30);
		gtk_signal_connect (GTK_OBJECT (tuneset), "clicked",
							GTK_SIGNAL_FUNC (tuneset_action), NULL);
		tuneget = gtk_button_new_with_label("GET");
		gtk_fixed_put(GTK_FIXED(fixed), tuneget, 560, 150);
		gtk_widget_set_size_request(tuneget, 40, 30);
		gtk_signal_connect (GTK_OBJECT (tuneget), "clicked",
							GTK_SIGNAL_FUNC (tuneget_action), NULL);
		tunetest = gtk_button_new_with_label("Test");
		gtk_fixed_put(GTK_FIXED(fixed), tunetest, 620, 150);
		gtk_widget_set_size_request(tunetest, 40, 40);
		gtk_signal_connect (GTK_OBJECT (tunetest), "clicked",
							GTK_SIGNAL_FUNC (tunetest_action), NULL);


}

// create and attach all of the live data fields
void init_livedata (GtkWidget *topwin)
{
	int i;
	

	// create font specifiers
	dfsmall = pango_font_description_from_string ("Monospace");
	pango_font_description_set_size (dfsmall, 10*PANGO_SCALE);
	dfdata = pango_font_description_from_string ("Monospace");
	pango_font_description_set_size (dfdata, 20*PANGO_SCALE);
	
	for(i=0; i<MAXLD; i++)
	{
		if (l_d[i].canid == 0)
		{
			// at the end
			break;
		}
		
		// create the tag label widget
		l_d[i].taglab = gtk_label_new (l_d[i].tag);
		gtk_widget_modify_font(l_d[i].taglab, dfsmall);  
		gtk_fixed_put(GTK_FIXED(topwin), l_d[i].taglab, l_d[i].xpos, l_d[i].ypos);
		
		// create the live data field
		l_d[i].vallab = gtk_label_new ("xxxx");
		gtk_widget_modify_font(l_d[i].vallab, dfdata);  
		gtk_fixed_put(GTK_FIXED(topwin), l_d[i].vallab, l_d[i].xpos, l_d[i].ypos + 15);
	}
	msg_init (topwin, dfdata);
	fault_init (topwin, dfdata);
	limit_init (topwin, dfdata);

	load_cell_h = gtk_label_new ("yyyy");
	gtk_widget_modify_font(load_cell_h, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), load_cell_h, 910, 90 + 15);
	gtk_widget_show ((GtkWidget *)load_cell_h);

	load_cell_b = gtk_button_new_with_label ("RES");
	gtk_widget_modify_font(load_cell_b, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), load_cell_b, 910, 90 + 45);
	gtk_widget_show ((GtkWidget *)load_cell_b);
	gtk_signal_connect (GTK_OBJECT (load_cell_b), "clicked",
						GTK_SIGNAL_FUNC (load_cell_action), NULL);

	// create recharge button
	recharge_b = gtk_button_new_with_label ("Recharge");
	gtk_widget_modify_font(recharge_b, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), recharge_b, 875, 60);
	gtk_widget_show ((GtkWidget *)recharge_b);
	gtk_signal_connect (GTK_OBJECT (recharge_b), "clicked",
						GTK_SIGNAL_FUNC (recharge_action), NULL);
}

int i;

void init_windows (int argc, char *argv[])
{

	PangoFontDescription *df;
	static pthread_t	win_update;


	// init and create container window
	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "GtkFixed");
	gtk_window_set_default_size(GTK_WINDOW(window), 1024, 600);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

	fixed = gtk_fixed_new();
	gtk_container_add(GTK_CONTAINER(window), fixed);

	create_buttons (window, fixed);

	// add progress bar
	prog = gtk_progress_bar_new ();
	gtk_progress_bar_set_text ((GtkProgressBar *)prog, "Battery 35%");
	gtk_fixed_put(GTK_FIXED(fixed), prog, 800, 10);
	gtk_widget_set_size_request(prog, 214, 50);
	
	// add kw in/out bargraphs
	prog_kw_regen = gtk_progress_bar_new ();
	prog_kw = gtk_progress_bar_new ();
	prog_kw_lag = gtk_progress_bar_new ();
	prog_kw_regen_lag = gtk_progress_bar_new ();

	// set text, orientation and color
	gtk_progress_bar_set_text ((GtkProgressBar *)prog_kw, "Kw");
	gtk_progress_bar_set_text ((GtkProgressBar *)prog_kw_regen, "R Kw");
	gtk_progress_bar_set_orientation ((GtkProgressBar *)prog_kw_regen, GTK_PROGRESS_RIGHT_TO_LEFT);
	GdkColor orange = {0, 0xffff, 0x3fff, 0x0000};
    GdkColor blue = {0, 0x0000, 0xffff, 0x3fff};
    GdkColor white = {0, 0xffff, 0xffff, 0xffff};
    GdkColor black = {0, 0x0000, 0x0000, 0x0000};
	
    gtk_widget_modify_bg((GtkProgressBar *)prog_kw, GTK_STATE_SELECTED, &orange);
    gtk_widget_modify_fg((GtkProgressBar *)prog_kw, GTK_STATE_NORMAL, &white);
    gtk_widget_modify_bg((GtkProgressBar *)prog_kw, GTK_STATE_NORMAL, &black);

    gtk_widget_modify_bg((GtkProgressBar *)prog_kw_regen, GTK_STATE_SELECTED, &blue);
    gtk_widget_modify_fg((GtkProgressBar *)prog_kw_regen, GTK_STATE_NORMAL, &white);
    gtk_widget_modify_bg((GtkProgressBar *)prog_kw_regen, GTK_STATE_NORMAL, &black);
	
	// set position
	gtk_fixed_put(GTK_FIXED(fixed), prog_kw, 600, 24);
	gtk_widget_set_size_request(prog_kw, 180, 30);
	gtk_fixed_put(GTK_FIXED(fixed), prog_kw_regen, 500, 24);
	gtk_widget_set_size_request(prog_kw_regen, 100, 30);

	// add data display labels
	init_livedata (fixed);

	// create the data scope
	create_scope (fixed);

	init_tune (fixed);

	// show the window
	gtk_widget_show_all(window);

	display_tune (FALSE);
	
	// hide as appropriate
	hide_buttons ();
	
	// keep it on top
	gtk_window_set_keep_above(GTK_WINDOW(window),TRUE);
	gtk_window_fullscreen(GTK_WINDOW(window));

	// start updating the data
	pthread_create (&win_update, NULL, update_window, NULL);

}



void *update_window (void *ptr)
{
	int i, cnt;
	static char btemp[30];
	int v1, v2, v3, v4;
	FILE *sens;
	char sens_data[100];
	int	 sens_val, sens_avg = 0, sens_start, zero = 18;
	float	mv, zero_pounds, pounds, kgs;
	FILE	*bpf;
	char 	data[100];
	float hp, tq, kw;
	

	while (1)
	{
		gdk_threads_enter();
		sprintf (btemp, "Battery %4.1f%%", (bp - dischargebp) * 100.0);
		gtk_progress_bar_set_text ((GtkProgressBar *)prog, btemp);
		gtk_progress_bar_set_fraction ((GtkProgressBar *)prog, bp);

		for(i=0; i<MAXLD; i++)
		{
			if (l_d[i].canid == 0)
			{
				// at the end
				break;
			}

			if (data_current)
			{
				gtk_label_set (GTK_LABEL (l_d[i].vallab), l_d[i].valbuf);
				if (l_d[i].canid == SRPM) v1 = l_d[i].val;
				if (l_d[i].canid == RRPM) v2 = l_d[i].val;
				if (l_d[i].canid == TEMP) v4 = l_d[i].val;
				v3 = l_d[2].val * l_d[3].val;

				// only update once per data update
				if (l_d[i].canid == SRPM)
				{
					checkFininshTorquetest (v1, v2);
				}
			}
			else
			{
				gtk_label_set (GTK_LABEL (l_d[i].vallab), "-----");
				v1 = v2 = v3 = v4 = 0;

				// don't leave the test hanging
				checkFininshTorquetest (0, 0);
			}
		}
		if (trend_active())
		{
			update_scope_trend_data (v1, v2, v3, v4);
		}

		if (getdisch)
		{
			getdisch = 0;
			// get the discharge state
			bpf = fopen("chargestate.dat", "r");
			if (bpf != NULL)
			{
				fread(data, 16, 1, bpf);
				sscanf (data, "%f", &dischargebp);
			}
			fclose(bpf);

			// start collecting trend data
			TrendStart();

		}
		if (setdisch)
		{
			setdisch = 0;

			// update the discharge state
			bpf = fopen("chargestate.dat", "w+");
			if (bpf != NULL)
			{
				sprintf (data, "%f", 100 - (bp - dischargebp));
				fwrite(data, strlen(data), 1, bpf);
			}
			fclose(bpf);

			// write the trend updates to a csv file
			TrendWrite();
		}
		if (cleardisch)
		{
			cleardisch = 0;

			// update the discharge state
			bpf = fopen("chargestate.dat", "w+");
			dischargebp = 0.0;
			if (bpf != NULL)
			{
				sprintf (data, "%f", dischargebp);
				fwrite(data, strlen(data), 1, bpf);
			}
			fclose(bpf);
		}
#define torque 1
#ifdef torque		
		sens = fopen("/sys/bus/platform/devices/tiadc/iio:device0/in_voltage4_raw", "r");
		if (sens != NULL)
		{
			cnt = 0;
			fread(sens_data, 5, 1, sens);
			sscanf (sens_data, "%d", &sens_start);
			for(i=0; i<100 && cnt < 3; i++)
			{
				fread(sens_data, 5, 1, sens);
				sscanf (sens_data, "%d", &sens_val);
				if (sens_start == sens_val)
				{
					cnt++;
				}
			}
		}
		fclose (sens);

		if (i != 100)
		{
			sens_avg = ((sens_avg * 7) + sens_val)/8;
		}
		
		// 12 bits 1.8v full scale means mv = sens_avg/4096 * 1.8
		mv = ((float)(sens_avg - zero) / 4096.0 * 1.8) * 1000.0;
		
		pounds = mv * .5119;	// linear?
		
//		if (max < pounds) max = pounds;
//		sprintf(sens_data, "%4.0f", max);
		sprintf (sens_data, "%4d", sens_val);
		gtk_label_set (GTK_LABEL (load_cell_h), sens_data);
		can_cmd (601, sens_val);

#else
		hp = (float) (amps / 10) * (float) volts * .0013404; 
		if (rpm < 10) rpm = 10;		
		tq = hp * 5252 / rpm;
		sprintf(sens_data, "%5.1f", tq);
		gtk_label_set (GTK_LABEL (load_cell_h), sens_data);
#endif

		// kw and regen bargraphs
		kw = (((float) (amps / 10) * (float) volts) / 1000.0) / 100;
		if (kw < 0)
		{
			sprintf (btemp, "R kw %4.1f%", -kw * 100.0);
			gtk_progress_bar_set_text ((GtkProgressBar *)prog_kw_regen, btemp);
			gtk_progress_bar_set_fraction ((GtkProgressBar *)prog_kw, 0.0);
			gtk_progress_bar_set_fraction ((GtkProgressBar *)prog_kw_regen, -kw * 8.0);

			gtk_progress_bar_set_text ((GtkProgressBar *)prog_kw, "kw");
			
		}
		else
		{
			sprintf (btemp, "kw %4.1f%", kw * 100.0);
			gtk_progress_bar_set_text ((GtkProgressBar *)prog_kw, btemp);
			gtk_progress_bar_set_fraction ((GtkProgressBar *)prog_kw_regen, 0.0);
			gtk_progress_bar_set_fraction ((GtkProgressBar *)prog_kw, kw);

			gtk_progress_bar_set_text ((GtkProgressBar *)prog_kw_regen, "R kw");
		}	
		
		gdk_threads_leave();
		data_current = FALSE;

		usleep(250000);
	}
}

