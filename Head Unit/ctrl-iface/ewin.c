//
// eGolf ewin.c
//

#include <gtk/gtk.h>
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
#include "snap.h"
#include "ewin.h"
#include "msg.h"
#include "buttons.h"
#include "dial.h"
#include "tune.h"
#include "batt.h"

#include <stdio.h>
#include <stdlib.h>

extern gfloat x[], y[];
extern gfloat yv[];


static int data_current = FALSE;
static float bp=0.15, lastbp=99.0;
float dischargebp = 0.0;

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
	GtkAdjustment *dial_adjustment;
	GtkWidget *dial;
	int dial_xpos;
	int dial_ypos;
	int dial_xsize;
	int dial_ysize;
	int dial_min;
	int dial_max;
	int dial_redline;
	char dial_tag[TAGSZ];
};

GtkWidget *load_cell_h;
GtkWidget *load_cell_b;

GtkWidget *snapres;
GtkWidget *torqueres;

struct live_data	l_d[] = {
//	{"rrpm", 	"", 0, RRPM, 	100, 90,	NULL, NULL, NULL, NULL,  10, 130, 200, 200,   0, 10000, 7000, "RPM" },
//	{"srpm", 	"", 0, SRPM, 	200, 90,	NULL, NULL, NULL, NULL,   0,   0,   0,   0,   0,   0,      0, ""},
//	{"volts", 	"", 0, VOLTS,	300, 90,	NULL, NULL, NULL, NULL, 210, 130, 150, 150,   0, 400,      0, "Volts"},
//	{"amps", 	"", 0, AMPS,	400, 90,	NULL, NULL, NULL, NULL, 360, 130, 150, 150, -100, 400,    200, "Amps" },
//	{"temp", 	"", 0, TEMP,	500, 90,	NULL, NULL, NULL, NULL, 510, 130, 150, 150,   0, 200,    150, "Temp" },
	{"rrpm", 	"", 0, RRPM, 	100, 90,	NULL, NULL, NULL, NULL,  250, 150, 300, 300,   0, 10000, 7000, "RPM" },
	{"srpm", 	"", 0, SRPM, 	200, 90,	NULL, NULL, NULL, NULL,   0,   0,   0,   0,   0,   0,      0, ""},
	{"volts", 	"", 0, VOLTS,	300, 90,	NULL, NULL, NULL, NULL, 150, 470, 160, 160,   0, 400,      0, "Volts"},
	{"amps", 	"", 0, AMPS,	400, 90,	NULL, NULL, NULL, NULL, 320, 470, 160, 160, -100, 400,    200, "Amps" },
	{"temp", 	"", 0, TEMP,	500, 90,	NULL, NULL, NULL, NULL, 490, 470, 160, 160,   0, 200,    150, "Temp" },
	{"phAc", 	"", 0, PHAC,	600, 90,	NULL, NULL, NULL, NULL,   0,   0,   0,   0,   0,   0,      0, ""},
	{"wattH", 	"", 0, PHCC,	700, 90,	NULL, NULL, NULL, NULL,   0,   0,   0,   0,   0,   0,      0, ""},
	{"wH/M", 	"", 0, BATP,	800, 90,	NULL, NULL, NULL, NULL,   0,   0,   0,   0,   0,   0,      0, ""},
	{"", 		"", 0, 0, 		0, 0, 		NULL, NULL, NULL, NULL,   0,   0,   0,   0,   0,   0,      0, ""}
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

void StepTrend (void)
{
	// step to the next trend record
	if (trendcollecting && (trendpos < MAXCSVTREND))
		{
		trendpos++;
		}
}

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
	case PHAC :
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

#define TTESTSPEEDTHRESHOLD 2000	// arbitrary test endpoint
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
GtkWidget	*prog_kwh_mile;
GtkWidget	*prog_kw_lag;
GtkWidget	*prog_kw_regen;
GtkWidget	*prog_kw_regen_lag;

void *update_window (void *ptr);
char *fmsg[] = {"                    ", "OVER TEMP", "OVER CURRENT", "OVER VOLT", "UNDER VOLT", "OIL PRESSURE", "PRECHARGE", "CURR FAULT","PHASE TEST FAIL", "RELAYPOS", "LOOP CURR A", "LOOP CURR B", "LOOP CURR C"};
char *lmsg[] = {"                    ", "Current", "RPM", "Temperature", "HW Current", "SW Current", "                "};
void handle_tune_data (int);
int tuneindex = 0;
int state;

void batt_parse (bbuffer);

void live_data_update (struct can_frame *frame)
{
	int i;
	short	dv;
	static char temp[50];
	static int limitv = 0;
	static int faultv = 0;
	
	
	switch (frame->can_id)
	{
		case HTRDATA :
			htr_parse (frame->data);
			break;
			
		case BATTDATA :
			// copy first half of the data to a temp buffer
			temp[0] = frame->data[0];
			temp[1] = frame->data[1];
			temp[2] = frame->data[2];
			temp[3] = frame->data[3];
			temp[4] = frame->data[4];
			temp[5] = frame->data[5];
			temp[6] = frame->data[6];
			temp[7] = frame->data[7];
			break;
			
		case BATTDATA2 :
			// copy second half of the data to a temp buffer and parse
			temp[8] = frame->data[0];
			temp[9] = frame->data[1];
			temp[10] = frame->data[2];
			temp[11] = frame->data[3];
			temp[12] = frame->data[4];
			temp[13] = frame->data[5];
			temp[14] = frame->data[6];
			temp[15] = frame->data[7];
			batt_parse (temp);
			break;
			
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
					
					// trigger snap collection if requested
					if(7 == frame->data[0] && fault_snap_get())
					{
						fault_snap_clear();
						snap_get_start ();
					}
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

static int snap_res_val = 0;
int get_snapres (void)
{
	if (snap_res_val > 4) 
	{
		return ((snap_res_val - 5)>4?4:(snap_res_val - 5));	
	}
	else
	{
		return snap_res_val;
	}
}

extern char *srestime[];
void toggle_snapres(void)
{
	snap_res_val++;
	if (snap_res_val > 10) snap_res_val = 0;
	gtk_label_set (GTK_LABEL (snapres), srestime[snap_res_val]);	
}

int get_snapres_val (void)
{
	return snap_res_val;
}

float	max = 0.0;
void load_cell_action (void)
{
	max = 0.0;
}

PangoFontDescription *dfsmall;
PangoFontDescription *dfdata;
GtkWidget *window;
GtkWidget *fixed;

GdkColor gray = {0, 0x7fff, 0x7fff, 0x7fff};
GdkColor orange = {0, 0xffff, 0x3fff, 0x0000};
GdkColor green = {0xf000, 0x0000, 0xf000, 0x0000};
GdkColor black = {0, 0x0000, 0x0000, 0x0000};
GdkColor white = {0, 0xefff, 0xefff, 0xefff};

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
		
		// setup data text string
		if (l_d[i].xpos != 0)
		{
			// create the tag label widget
			l_d[i].taglab = gtk_label_new (l_d[i].tag);
			gtk_widget_modify_font(l_d[i].taglab, dfsmall);  
			gtk_fixed_put(GTK_FIXED(topwin), l_d[i].taglab, l_d[i].xpos, l_d[i].ypos);
			
			// create the live data field
			l_d[i].vallab = gtk_label_new ("0000.0");
			gtk_widget_modify_font(l_d[i].vallab, dfdata);  
			gtk_fixed_put(GTK_FIXED(topwin), l_d[i].vallab, l_d[i].xpos, l_d[i].ypos + 15);
		}
		
		// setup dial
		if (l_d[i].dial_xpos != 0)
		{
			l_d[i].dial_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new (0, l_d[i].dial_min, l_d[i].dial_max, 0.1, 0.5, 0));
	  
			l_d[i].dial = gtk_dial_new(l_d[i].dial_adjustment);
			gtk_dial_set_update_policy (GTK_DIAL(l_d[i].dial), GTK_UPDATE_DELAYED);
			gtk_dial_set_tag (GTK_DIAL(l_d[i].dial), l_d[i].dial_tag);
			gtk_dial_set_redline (GTK_DIAL(l_d[i].dial), l_d[i].dial_redline);
			gtk_fixed_put(GTK_FIXED(fixed), l_d[i].dial, l_d[i].dial_xpos, l_d[i].dial_ypos);
			gtk_widget_set_size_request(l_d[i].dial, l_d[i].dial_xsize, l_d[i].dial_ysize);
			
			gtk_widget_modify_fg((GtkWidget *)l_d[i].dial, GTK_STATE_NORMAL, &gray);		// ticks
			gtk_widget_modify_bg((GtkWidget *)l_d[i].dial, GTK_STATE_NORMAL, &black);		// pointer
			gtk_widget_modify_bg((GtkWidget *)l_d[i].dial, GTK_STATE_ACTIVE, &white);	// background
			
			
		}
	}
	msg_init (topwin, dfdata);
	fault_init (topwin, dfdata);
	limit_init (topwin, dfdata);
	batt_data_init (topwin, dfdata);
	
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

	// toggled snap resolution display
	snapres = gtk_label_new (".12 S");
	gtk_widget_modify_font(snapres, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), snapres, 800, 480);
	
	// torque test result
	torqueres = gtk_label_new ("-----");
	gtk_widget_modify_font(torqueres, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), torqueres, 875, 375);
		
}

static float instwhpermile = 0.0;
static float whpermile = 0.0;
static float total_kwh = 0.0;
static float total_mile = 0.0;

void update_kwh (int rpm, float kw)
{
	GdkColor yellow = {0, 0xffff, 0x3fff, 0x0000};
    GdkColor red = {0, 0xffff, 0x0000, 0x0000};
    GdkColor green = {0, 0x0000, 0xffff, 0x1fff};
	static float kwh_mile = 0.0;
	char btemp[40];
	
	
	// calc mph rpm/100 in second gear is approx mph. This is called 4 times/second so..
	float miles = (float)rpm/(3600.0*396.8);
	total_mile += miles;
	
	// calc kwh/mile
	float kwh = (kw<0.0?0.0:kw)/(3600.0*3.968);
	if (kwh > 0.0)
		total_kwh += kwh;

	if (miles > 0)
		instwhpermile = (kwh * 1000.0) / miles;
	else
		instwhpermile = 0.0;

	// set the wh/mile average
	if (total_mile > 0.0)
	{
		whpermile = (total_kwh * 1000.0) / total_mile;
	}
	else
	{
		whpermile = 0.0;
	}
	
	float internal_kwh_mile = miles/kwh > 10.0 ? 10.0 : miles/kwh;
	
	// calc average
	if(rpm < 100)
	{
		// reset the average
		kwh_mile = 0.0;
	}
	else
	{
		kwh_mile = (kwh_mile * .3) + (internal_kwh_mile * .7);
	}
		
	// display progress bar
	sprintf (btemp, "%4.1f%", kwh_mile);
	gtk_progress_bar_set_text ((GtkProgressBar *)prog_kwh_mile, btemp);
	gtk_progress_bar_set_fraction ((GtkProgressBar *)prog_kwh_mile, ((kwh_mile/10.0) > 1.0)?1.0:(kwh_mile/10.0));
	
	if(kwh_mile < 3)
	{
		// red
		gtk_widget_modify_bg((GtkWidget *)prog_kwh_mile, GTK_STATE_SELECTED, &red);
	}
	else if (kwh_mile < 5.0)
	{
		// yellow
		gtk_widget_modify_bg((GtkWidget *)prog_kwh_mile, GTK_STATE_SELECTED, &yellow);
	}
	else
	{
		// green
		gtk_widget_modify_bg((GtkWidget *)prog_kwh_mile, GTK_STATE_SELECTED, &green);
	}
		
	
	return;
}

void init_kw (GtkWidget *fixed_win)
{
	GdkColor orange = {0, 0xffff, 0x3fff, 0x0000};
    GdkColor blue = {0, 0x0000, 0xffff, 0x3fff};
    GdkColor white = {0, 0xffff, 0xffff, 0xffff};
    GdkColor black = {0, 0x0000, 0x0000, 0x0000};

	// add kw in/out bargraphs
	prog_kw_regen = gtk_progress_bar_new ();
	prog_kw = gtk_progress_bar_new ();

	// add kwh/mile
	prog_kwh_mile = gtk_progress_bar_new ();
	gtk_fixed_put(GTK_FIXED(fixed), prog_kwh_mile, 810, 323);
	gtk_widget_set_size_request(prog_kwh_mile, 60, 150);
	gtk_progress_bar_set_text ((GtkProgressBar *)prog_kwh_mile, "Kwh/mile");
	gtk_progress_bar_set_orientation ((GtkProgressBar *)prog_kwh_mile, GTK_PROGRESS_BOTTOM_TO_TOP);
    gtk_widget_modify_fg((GtkWidget *)prog_kwh_mile, GTK_STATE_NORMAL, &white);
    gtk_widget_modify_bg((GtkWidget *)prog_kwh_mile, GTK_STATE_NORMAL, &black);

	// set text, orientation and color
	gtk_progress_bar_set_text ((GtkProgressBar *)prog_kw, "Kw");
	gtk_progress_bar_set_text ((GtkProgressBar *)prog_kw_regen, "R Kw");
	gtk_progress_bar_set_orientation ((GtkProgressBar *)prog_kw_regen, GTK_PROGRESS_RIGHT_TO_LEFT);
	
    gtk_widget_modify_bg((GtkWidget *)prog_kw, GTK_STATE_SELECTED, &orange);
    gtk_widget_modify_fg((GtkWidget *)prog_kw, GTK_STATE_NORMAL, &white);
    gtk_widget_modify_bg((GtkWidget *)prog_kw, GTK_STATE_NORMAL, &black);

    gtk_widget_modify_bg((GtkWidget *)prog_kw_regen, GTK_STATE_SELECTED, &blue);
    gtk_widget_modify_fg((GtkWidget *)prog_kw_regen, GTK_STATE_NORMAL, &white);
    gtk_widget_modify_bg((GtkWidget *)prog_kw_regen, GTK_STATE_NORMAL, &black);
	
	// set position
	gtk_fixed_put(GTK_FIXED(fixed_win), prog_kw, 600, 24);
	gtk_widget_set_size_request(prog_kw, 180, 30);
	gtk_fixed_put(GTK_FIXED(fixed_win), prog_kw_regen, 500, 24);
	gtk_widget_set_size_request(prog_kw_regen, 100, 30);

}

GdkColor backg = {0, 0xcfff, 0xcfff, 0xcfff};

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
    gtk_widget_modify_fg((GtkWidget *)window, GTK_STATE_NORMAL, &backg);
    gtk_widget_modify_bg((GtkWidget *)window, GTK_STATE_NORMAL, &backg);

	fixed = gtk_fixed_new();
	gtk_container_add(GTK_CONTAINER(window), fixed);

	create_buttons (window, fixed);
					
	// add progress bar
	prog = gtk_progress_bar_new ();
	gtk_progress_bar_set_text ((GtkProgressBar *)prog, "Battery 35%");
	gtk_fixed_put(GTK_FIXED(fixed), prog, 800, 10);
	gtk_widget_set_size_request(prog, 214, 50);
	
	init_kw(fixed);

	init_batt_bar(fixed);
	
	// add data display labels
	init_livedata (fixed);

	// show the window
	gtk_widget_show_all(window);

	display_tune (FALSE);
	
	// keep it on top
	gtk_window_set_keep_above(GTK_WINDOW(window),TRUE);
	gtk_window_fullscreen(GTK_WINDOW(window));

	// start updating the data
	pthread_create (&win_update, NULL, update_window, NULL);

}

void handle_soc()
{
	FILE	*bpf;
	char 	data[100];
	
	if (get_disch_action(GETDISCHFILE) == 1)
	{
		// get the discharge state
		bpf = fopen("chargestate.dat", "r");
		if (bpf != NULL)
		{
			fread(data, 35, 1, bpf);
			sscanf (data, "%f %f", &total_kwh, &total_mile);
		}
		fclose(bpf);

		// start collecting trend data
		TrendStart();

	}
	if (get_disch_action(SETDISCHFILE) == 1)
	{
		// update the discharge state
		bpf = fopen("chargestate.dat", "w+");
		if (bpf != NULL)
		{
			sprintf (data, "%f %f", total_kwh, total_mile);
			fwrite(data, strlen(data), 1, bpf);
		}
		fclose(bpf);

		// write the trend updates to a csv file
		TrendWrite();
	}

	if (get_disch_action(CLEARDISCHFILE) == 1)
	{
		gtk_progress_bar_set_text ((GtkProgressBar *)prog, "Recharged!");
		// update the discharge state
		bpf = fopen("chargestate.dat", "w+");
		total_kwh = total_mile = 0.0;
		if (bpf != NULL)
		{
			sprintf (data, "0.00 0.00");
			fwrite(data, strlen(data), 1, bpf);
		}
		fclose(bpf);
	}
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

		for(i=0; i<6; i++)
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

				// see if we are done with a torque test
				if (l_d[i].canid == SRPM)
				{
					checkFininshTorquetest (v1, v2);
				}
				
				// update the dial displays
				if (l_d[i].dial)
				{
					gtk_adjustment_set_value(l_d[i].dial_adjustment, i == 3?l_d[i].val/10:l_d[i].val);
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

		handle_soc();
		
//#define torque 1
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
		if (rpm < 10) rpm = 0;		
		sprintf(sens_data, "%6.1f", whpermile);
		gtk_label_set (GTK_LABEL (load_cell_h), sens_data);
		
		sprintf(sens_data, "%6.1f", total_kwh * 1000);
		gtk_label_set (GTK_LABEL (l_d[6].vallab), sens_data);
		sprintf(sens_data, "%6.1f", instwhpermile);
		gtk_label_set (GTK_LABEL (l_d[7].vallab), sens_data);

#endif

		
		// kw and regen bargraphs
		kw = (((float) (amps / 10) * (float) volts) / 1000.0) / 100;

		update_kwh (rpm, kw * 100);

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

