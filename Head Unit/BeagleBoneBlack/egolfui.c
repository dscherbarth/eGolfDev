//
// eGolf ctrl interface for BBB + chipsee display
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
#include "ecan.h"
#include "ewin.h"

GtkDataboxGraph *graph1 = NULL;
GtkDataboxGraph *graph2 = NULL;
GtkWidget *box;
GdkColor color;
gfloat x[2500], y[2500];

void init_io (void)
{
}

int main (int argc, char *argv[])
{

	// initialize
//	g_threads_init();
	gdk_threads_init();
	init_windows (argc, argv);
	usleep(100000);
	init_can ();
	
	// handle window activity
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

}
