//
// eGolf buttons.c
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
#include <errno.h>
 
#include <linux/can.h>
#include <linux/can/raw.h>

#include "buttons.h"
#include "ecan.h"
#include "scope.h"
#include "msg.h"

void snapshot_action (void);

static	GtkWidget *ctrlbutton = NULL;
static	GtkWidget *dirbutton = NULL;
static	GtkWidget *snapresbutton = NULL;

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
int getdisch = 0, setdisch = 0;

void ctrl_cmd_action (void) 
{
	char *curr_state;

	
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
	}
}

void change_cmd_action (void) 
{
	can_cmd (CMDSEND, HEADCMDCHANGEDIR);
}

#define JOGINCR	10
#define JOGSET	382
#define TORQUESET	383

static GtkWidget *vscale;
static int jogval = 0;
void slider_change (void)
{
	gdouble val;
	
	
	// limit test
	if (jogval > 4000) jogval = 4000;

	val = gtk_range_get_value (vscale);
	
	// send to controller
	can_cmd (TORQUESET, (int)val);
}
void ttest_action (void)
{
	// reset torque test 
	// start the torque test
	startTorquetest ();
}

struct chan_select {
	GtkWidget *chan;
	int yloc;
	char *label;
};

struct chan_select	csel[] = {
{NULL, 	200, 	"St Pt Q" },
{NULL, 	240, 	"Iq" },
{NULL, 	280, 	"Vq" },
{NULL, 	320, 	"St Pt D" },
{NULL, 	360, 	"Id" },
{NULL, 	400, 	"Vd" },
{NULL, 	440, 	"Torq " }
};

struct chan_select	tsel[] = {
{NULL, 	200, 	"S rpm" },
{NULL, 	240, 	"R rpm" },
{NULL, 	280, 	"Power" },
{NULL, 	320, 	"Temp" }
};

static 	GtkWidget *rad_trend;
static 	GtkWidget *rad_tune;
static	GtkWidget *rad_snap;

int trend_active (void)
{
	return gtk_toggle_button_get_active ((GtkToggleButton *)rad_trend);
}

void trend_snap_action (void)
{
	int i;
	
	if (gtk_toggle_button_get_active ((GtkToggleButton *)rad_trend))
	{
		display_scope (1);

		// disable snapshot checkboxes and switch to trend view
		for (i=0; i<7; i++)
		{
			gtk_widget_hide ((GtkWidget *)csel[i].chan);
			update_scope_snapshot_view (i+1, FALSE);
		}

		// display trend selection checkboxes
		for (i=0; i<4; i++)
		{
			gtk_widget_show ((GtkWidget *)tsel[i].chan);
			update_scope_trend_view (i+1, gtk_toggle_button_get_active ((GtkToggleButton *)tsel[i].chan));
		}
	}
	if (gtk_toggle_button_get_active ((GtkToggleButton *)rad_tune))
	{
		// tune selected, turn off trend display and turn on tuning params
		// disable snapshot checkboxes and switch to trend view
		display_scope (0);
		for (i=0; i<7; i++)
		{
			gtk_widget_hide ((GtkWidget *)csel[i].chan);
			update_scope_snapshot_view (i+1, FALSE);
		}

		// disable trend selection checkboxes
		for (i=0; i<4; i++)
		{
			gtk_widget_hide ((GtkWidget *)tsel[i].chan);
			update_scope_trend_view (i+1, FALSE);
		}
		
		// and display tune controls
		display_tune (TRUE);
		
	}
	if (gtk_toggle_button_get_active ((GtkToggleButton *)rad_snap))
	{
		display_scope (1);

		// hide trend selection checkboxes
		for (i=0; i<4; i++)
		{
			gtk_widget_hide ((GtkWidget *)tsel[i].chan);
			update_scope_trend_view (i+1, FALSE);
		}

		// enable snapshot checkboxes and switch to snapshot view
		for (i=0; i<7; i++)
		{
			gtk_widget_show ((GtkWidget *)csel[i].chan);
			update_scope_snapshot_view (i+1, gtk_toggle_button_get_active ((GtkToggleButton *)csel[i].chan));
		}
	}
}

void hide_buttons ()
{
	int i;
	
	for (i=0; i<4; i++)
	{
		gtk_widget_hide ((GtkWidget *)tsel[i].chan);
	}
}

void chan_sel (void)
{
	int i;
	
	if (gtk_toggle_button_get_active ((GtkToggleButton *)rad_trend))
	{
		for (i=0; i<4; i++)
		{
			update_scope_trend_view (i+1, gtk_toggle_button_get_active ((GtkToggleButton *)tsel[i].chan));
		}
	}
	else
	{
		// check the toggled state of each of the channels and re-display the databox
		for (i=0; i<7; i++)
		{
			update_scope_snapshot_view (i+1, gtk_toggle_button_get_active ((GtkToggleButton *)csel[i].chan));
		}
	}
	
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
	
	// add accel slider
	vscale = gtk_vscale_new_with_range (0.0, 2000.0, 10.0);
	gtk_fixed_put(GTK_FIXED(fixed), vscale, 780, 200);
	gtk_range_set_inverted (vscale, TRUE);
	gtk_widget_set_size_request(vscale, 50, 375);
	gtk_signal_connect (GTK_OBJECT (vscale), "value-changed",
						GTK_SIGNAL_FUNC (slider_change), NULL);
	
	// add buttons
	button1 = gtk_button_new_with_label("Done");
	gtk_fixed_put(GTK_FIXED(fixed), button1, 900, 550);
	gtk_widget_set_size_request(button1, 80, 35);

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

	// selection radio buttons (for trend/snapshot selection)
	rad_tune = gtk_radio_button_new_with_label (NULL, "Tune");
	rl = g_slist_append (rl, rad_tune);
	rad_trend = gtk_radio_button_new_with_label (NULL, "Trend");
	rl = g_slist_append (rl, rad_trend);
	rad_snap  = gtk_radio_button_new_with_label ((GSList *)rl, "Snapshot");
	gtk_toggle_button_set_active ((GtkToggleButton *)rad_snap, TRUE);
	gtk_toggle_button_set_active ((GtkToggleButton *)rad_trend, FALSE);
	gtk_toggle_button_set_active ((GtkToggleButton *)rad_tune, FALSE);
	gtk_fixed_put(GTK_FIXED(fixed), rad_tune, 220, 520);
	gtk_fixed_put(GTK_FIXED(fixed), rad_trend, 380, 520);
	gtk_fixed_put(GTK_FIXED(fixed), rad_snap, 540, 520);
	gtk_widget_set_size_request(rad_tune, 60, 25);
	gtk_widget_set_size_request(rad_trend, 75, 25);
	gtk_widget_set_size_request(rad_snap, 90, 25);
	gtk_signal_connect (GTK_OBJECT (rad_trend), "clicked",
						GTK_SIGNAL_FUNC (trend_snap_action), NULL);
	gtk_signal_connect (GTK_OBJECT (rad_snap), "clicked",
						GTK_SIGNAL_FUNC (trend_snap_action), NULL);
	gtk_signal_connect (GTK_OBJECT (rad_tune), "clicked",
						GTK_SIGNAL_FUNC (trend_snap_action), NULL);

	// channel selection checkboxes
	for (i=0; i<7; i++)
	{
		csel[i].chan = gtk_check_button_new_with_label(csel[i].label);
		gtk_fixed_put(GTK_FIXED(fixed), csel[i].chan, 10, csel[i].yloc);
		gtk_toggle_button_set_active ((GtkToggleButton *)csel[i].chan, TRUE);
		gtk_widget_set_size_request(csel[i].chan, 75, 30);
		gtk_signal_connect (GTK_OBJECT (csel[i].chan), "clicked",
							GTK_SIGNAL_FUNC (chan_sel), NULL);
	}
	
	// trend selectors (need to hide these)
	for (i=0; i<4; i++)
	{
		tsel[i].chan = gtk_check_button_new_with_label(tsel[i].label);
		gtk_fixed_put(GTK_FIXED(fixed), tsel[i].chan, 10, tsel[i].yloc);
		gtk_toggle_button_set_active ((GtkToggleButton *)tsel[i].chan, TRUE);
		gtk_widget_set_size_request(tsel[i].chan, 75, 30);
		gtk_signal_connect (GTK_OBJECT (tsel[i].chan), "clicked",
							GTK_SIGNAL_FUNC (chan_sel), NULL);
	}
	
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
