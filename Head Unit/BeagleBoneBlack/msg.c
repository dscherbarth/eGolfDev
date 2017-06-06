//
// eGolf msg.c
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

#include "msg.h"
//
// msg interface 
//
char *msgarray[] = {"Waveform Off    ",
					"Check Press Sw  ",
					"Zeroize Voltage ",
					"Precharge       ",
					"HV on           ",
					"Zeroize amps    ",
					"Oil Cooling on  ",
					"Inverter on     ",
					"Zeroize throttle",
					"Waveform ON     ",
					"No OilP Fault   ",
					"Oil Sw Fault    ",
					"Flux Cap Charged",
					"1.21 Gw Avail   ",
					"Ramp RPM Down   ",
					"Inverter off    ",
					"Cooling oil off ",
					"HV off          ",
					"Precharge off   ",
					"                ",
					"PRE-PRECharge Fail",
					"PRE-Charge Fail "

};

GtkWidget 	*msglabel = NULL;
void msg_init (GtkWidget *topwin, PangoFontDescription *dfdata)
{
	msglabel = gtk_label_new ("                                        ");
	gtk_widget_modify_font(msglabel, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), msglabel, 10, 560);
}

GtkWidget 	*faultlabel = NULL;
void fault_init (GtkWidget *topwin, PangoFontDescription *dfdata)
{
	GdkColor color;

	
	color.red = 65535; color.green = 0; color.blue = 0;

	faultlabel = gtk_label_new ("                    ");
	gtk_widget_modify_font(faultlabel, dfdata);  
	gtk_widget_modify_fg (faultlabel, GTK_STATE_NORMAL, &color);
	gtk_fixed_put(GTK_FIXED(topwin), faultlabel, 10, 10);
}

void clear_fault (void)
{
	gtk_label_set (GTK_LABEL (faultlabel), "                    ");
}

void disp_fault (char *msg)
{
	gtk_label_set (GTK_LABEL (faultlabel), msg);
}

GtkWidget 	*limitlabel = NULL;
void limit_init (GtkWidget *topwin, PangoFontDescription *dfdata)
{
	GdkColor color;

	
	color.red = 65535; color.green = 32767; color.blue = 0;

	limitlabel = gtk_label_new ("                ");
	gtk_widget_modify_font(limitlabel, dfdata);  
	gtk_widget_modify_fg (limitlabel, GTK_STATE_NORMAL, &color);
	gtk_fixed_put(GTK_FIXED(topwin), limitlabel, 310, 10);
}

void clear_limit (void)
{
	gtk_label_set (GTK_LABEL (limitlabel), "                ");
}

void disp_limit (char *msg)
{
	gtk_label_set (GTK_LABEL (limitlabel), msg);
}

void clear_msg (void)
{
	gtk_label_set (GTK_LABEL (msglabel), "                                        ");
}

void disp_msg (char *msg)
{
	gtk_label_set (GTK_LABEL (msglabel), msg);
}
void disp_msg_num (int msgnum)
{
	if (msgnum <= MSGBLANK && msgnum > -1)
	{
		gtk_label_set (GTK_LABEL (msglabel), msgarray[msgnum]);
	}
}
//
//
//
