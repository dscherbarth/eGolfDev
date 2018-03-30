//
// eGolf buttons.c
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
#include <errno.h>
 
#include <linux/can.h>
#include <linux/can/raw.h>

#include "buttons.h"
#include "ecan.h"
#include "scope.h"
#include "msg.h"

void snapshot_action (void);

GtkWidget *ctrlbutton = NULL;
static	GtkWidget *dirbutton = NULL;
static	GtkWidget *snapresbutton = NULL;

static GtkWidget *recharge_b;

extern double dischargebp;

extern void toggle_snapres(void);

void snapresbutton_cmd_action (void) 
{
	// cycle through and show snap resolutions
	toggle_snapres();
}

void done_action (void) 
{
     g_print ("Done\n");
}

void destroy (void) 
{
     gtk_main_quit ();
}
static int getdisch = 0, setdisch = 0, cleardisch = 0;
	
int get_disch_action(int what)
{
	switch (what)
	{
		case GETDISCHFILE :
			if (1 == getdisch)
			{
				getdisch = 0;
				return 1;
			}
			break;
			
		case SETDISCHFILE :
			if (1 == setdisch)
			{
				setdisch = 0;
				return 1;
			}
			break;
			
		case CLEARDISCHFILE :
			if (1 == cleardisch)
			{
				cleardisch = 0;
				return 1;
			}
			break;
	}
	return 0;
}

void recharge_action (void)
{
	cleardisch = 1;
}

void ctrl_cmd_action (void) 
{
	const char *curr_state;

	
	curr_state = gtk_button_get_label ((GtkButton *)ctrlbutton);
	
	if (strcmp(curr_state, "START") == 0)
	{
		can_cmd (CMDSEND, HEADCMDSTART);
		getdisch = 1;
	}
	else if (strcmp(curr_state, "STOP") == 0)
	{
		can_cmd (CMDSEND, HEADCMDSTOP);
		setdisch = 1;
	}
	else if (strcmp(curr_state, "CLR FAULT") == 0)
	{
		can_cmd (CMDSEND, HEADCMDCLEAR);
		setdisch = 1;						// same as 'stop'
	}
}

void change_cmd_action (void) 
{
	can_cmd (CMDSEND, HEADCMDCHANGEDIR);
}

#define JOGINCR	10
#define JOGSET	382
#define TORQUESET	383

void ttest_action (void)
{
	// reset torque test 
	// start the torque test
	startTorquetest ();
}

void create_buttons (GtkWidget *window, GtkWidget *fixed)
{
	GtkWidget *button1;
	GtkWidget *button3;
	GtkWidget *button4;
	GtkWidget *button5;

	GtkObject *adj1;
	
	static GSList *rl = NULL;

	int i;
	
	// add buttons
	
	create_batt_buttons(fixed);
	
	button1 = gtk_button_new_with_label("Done");
	gtk_fixed_put(GTK_FIXED(fixed), button1, 1150, 740);
	gtk_widget_set_size_request(button1, 120, 55);

	ctrlbutton = gtk_button_new_with_label("START");
	gtk_fixed_put(GTK_FIXED(fixed), ctrlbutton, 875, 200);
	gtk_widget_set_size_request(ctrlbutton, 120, 50);
	gtk_signal_connect (GTK_OBJECT (ctrlbutton), "clicked",
						GTK_SIGNAL_FUNC (ctrl_cmd_action), NULL);

	snapresbutton = gtk_button_new_with_label("SnapRes");
	gtk_fixed_put(GTK_FIXED(fixed), snapresbutton, 875, 480);
	gtk_widget_set_size_request(snapresbutton, 120, 50);
	gtk_signal_connect (GTK_OBJECT (snapresbutton), "clicked",
						GTK_SIGNAL_FUNC (snapresbutton_cmd_action), NULL);
		
	dirbutton = gtk_button_new_with_label("E STOP");
	gtk_fixed_put(GTK_FIXED(fixed), dirbutton, 875, 260);
	gtk_widget_set_size_request(dirbutton, 120, 50);
	gtk_signal_connect (GTK_OBJECT (dirbutton), "clicked",
						GTK_SIGNAL_FUNC (change_cmd_action), NULL);

	button3 = gtk_button_new_with_label("Snapshot");
	gtk_fixed_put(GTK_FIXED(fixed), button3, 875, 420);
	gtk_widget_set_size_request(button3, 120, 50);
	gtk_signal_connect (GTK_OBJECT (button3), "clicked",
						GTK_SIGNAL_FUNC (snapshot_action), NULL);

	button4 = gtk_button_new_with_label("T test");
	gtk_fixed_put(GTK_FIXED(fixed), button4, 875, 320);
	gtk_widget_set_size_request(button4, 90, 50);
	gtk_signal_connect (GTK_OBJECT (button4), "clicked",
						GTK_SIGNAL_FUNC (ttest_action), NULL);

	// create recharge button
	recharge_b = gtk_button_new_with_label ("Recharged");
	gtk_fixed_put(GTK_FIXED(fixed), recharge_b, 875, 70);
	gtk_widget_set_size_request(recharge_b, 90, 35);
	gtk_widget_show ((GtkWidget *)recharge_b);
	gtk_signal_connect (GTK_OBJECT (recharge_b), "clicked",
						GTK_SIGNAL_FUNC (recharge_action), NULL);

	// connect button signals
	gtk_signal_connect (GTK_OBJECT (window), "destroy",
						GTK_SIGNAL_FUNC (destroy), NULL);
	gtk_signal_connect (GTK_OBJECT (button1), "clicked",
						GTK_SIGNAL_FUNC (done_action), NULL);
	gtk_signal_connect_object (GTK_OBJECT (button1), "clicked",
								GTK_SIGNAL_FUNC (gtk_widget_destroy),
								GTK_OBJECT (window));
}

// set the command button text based on current controller state
void set_cmd_button_state (int state, int fault)
{
	// adjust for previous fault condition
	if (state == FAULTED && fault == 0)
	{
		state = STOPPED;
	}
	
	// display on button label
	switch (state)
	{
	case RUNNING :
		gtk_button_set_label ((GtkButton *)ctrlbutton, "STOP");
		break;
		
	case STOPPED :
		gtk_button_set_label ((GtkButton *)ctrlbutton, "START");
		break;
		
	case FAULTED :
		gtk_button_set_label ((GtkButton *)ctrlbutton, "CLR FAULT");
		break;
		
	}
}
