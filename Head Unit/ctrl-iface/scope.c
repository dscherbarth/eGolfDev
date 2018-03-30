//
// eGolf scope.c
//	create and manage the databox (scope) for snapshots and trending 
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

#include "scope.h"
#include "snap.h"
#include "msg.h"

extern GtkWidget *box;
static GtkWidget *table;
static	gfloat	xv[MAXSNAP];	// inited to 0 - 2499 just used as the timebase for the graph

// create the scope window and initialize related data structures
// set initial view to trending
void create_scope (GtkWidget *fixed)
{
	GdkColor color;
	int i;
	
	
	// init the timeline
	for (i=0; i<MAXSNAP; i++)
	{
		xv[i] = i;
	}
	
	// the livedata graph box
	gtk_databox_create_box_with_scrollbars_and_rulers (&box, &table,
													TRUE, TRUE, TRUE, TRUE);
	gtk_fixed_put(GTK_FIXED(fixed), table, 100, 140);
	gtk_widget_set_size_request(table, 650, 375);

	color.red = 8191;
	color.green = 8191;
	color.blue = 8191;
	gtk_widget_modify_bg (box, GTK_STATE_NORMAL, &color);
}

// set view to either trend or snapshot
// calling with the existing view re-scales and re-displays present selection
void switch_scope_view (int view)
{
	switch (view)
	{
	case VIEWSNAP :
		break;
		
	case VIEWTREND :
		break;
		
	}
}


static gfloat	snapval1[MAXSNAP];
static gfloat	snapval2[MAXSNAP];
static gfloat	snapval3[MAXSNAP];
static gfloat	snapval4[MAXSNAP];
static gfloat	snapval5[MAXSNAP];
static gfloat	snapval6[MAXSNAP];
static gfloat	snapval7[MAXSNAP];

static GtkDataboxGraph *graphv1 = NULL;
static GtkDataboxGraph *graphv2 = NULL;
static GtkDataboxGraph *graphv3 = NULL;
static GtkDataboxGraph *graphv4 = NULL;
static GtkDataboxGraph *graphv5 = NULL;
static GtkDataboxGraph *graphv6 = NULL;
static GtkDataboxGraph *graphv7 = NULL;

static GtkDataboxGraph *trendv1 = NULL;
static GtkDataboxGraph *trendv2 = NULL;
static GtkDataboxGraph *trendv3 = NULL;
static GtkDataboxGraph *trendv4 = NULL;

#define MAXTREND	600				// 10 min at 1 / sec
static gfloat	trendval1[MAXTREND];
static gfloat	trendval2[MAXTREND];
static gfloat	trendval3[MAXTREND];
static gfloat	trendval4[MAXTREND];

GtkDataboxGraph *update_scope_graph (GtkDataboxGraph *gr, gfloat *ydata, int col1, int col2, int col3)
{
	GdkColor color;

	
	gtk_databox_graph_remove( (GtkDatabox*)box, gr );

	color.red = col1; color.green = col2; color.blue = col3;

	gr = (GtkDataboxGraph *)gtk_databox_lines_new( MAXSNAP-1, xv, ydata, &color, 1 );
	gtk_databox_graph_add (GTK_DATABOX (box), gr);
	
	return gr;
}

GtkDataboxGraph *update_trend_graph (GtkDataboxGraph *gr, gfloat *ydata, int col1, int col2, int col3)
{
	GdkColor color;

	
	gtk_databox_graph_remove( (GtkDatabox*)box, gr );

	color.red = col1; color.green = col2; color.blue = col3;

	gr = (GtkDataboxGraph *)gtk_databox_lines_new( MAXTREND-1, xv, ydata, &color, 1 );
	gtk_databox_graph_add (GTK_DATABOX (box), gr);
	
	return gr;
}

// update scope snapshot data
// keep local static copies of the snapshot data and the graph widgets to add/remove
void update_scope_snapshot_data (int16_t *v1, int16_t *v2, int16_t *v3, int16_t *v4, int16_t *v5, int16_t *v6, int16_t *v7, int count)
{
	int i;
	
	// get the values in place
	for (i=0; i<count; i++)
	{
		snapval1[i] = v1[i];
		snapval2[i] = v2[i];
		snapval3[i] = v3[i];
		snapval4[i] = v4[i];
		snapval5[i] = v5[i];
		snapval6[i] = v6[i];
		snapval7[i] = v7[i];
	}

	// update the graph widgets
	graphv1 = update_scope_graph (graphv1, &snapval1[0], 0, 65535, 65535);
	graphv2 = update_scope_graph (graphv2, &snapval2[0], 0, 0, 65535);
	graphv3 = update_scope_graph (graphv3, &snapval3[0], 0, 65535, 0);
	graphv4 = update_scope_graph (graphv4, &snapval4[0], 65535, 65535, 65535);
	graphv5 = update_scope_graph (graphv5, &snapval5[0], 65535, 65535, 0);
	graphv6 = update_scope_graph (graphv6, &snapval6[0], 65535, 0, 0);
	graphv7 = update_scope_graph (graphv7, &snapval7[0], 65535, 0, 65535);
	
	gtk_databox_set_total_limits (GTK_DATABOX (box), -1000., 5000., -10000., 23000.);
	gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);

}

int trendindex = 0;
void update_scope_trend_data (int v1, int v2, int v3, int v4)
{
	int i;
	
	
	if (trendindex < MAXTREND)
	{
		trendval1[trendindex] = v1;
		trendval2[trendindex] = v2;
		trendval3[trendindex] = v3;
		trendval4[trendindex] = v4;
		
		trendindex++;
	}
	else
	{
		// shuffle values down and add at the end
		for (i=0; i < MAXTREND-1; i++)
		{
			trendval1[i] = trendval1[i+1];
			trendval2[i] = trendval2[i+1];
			trendval3[i] = trendval3[i+1];
			trendval4[i] = trendval4[i+1];
		}
		trendval1[MAXTREND-1] = v1;
		trendval2[MAXTREND-1] = v2;
		trendval3[MAXTREND-1] = v3;
		trendval4[MAXTREND-1] = v4;
	}
	gtk_databox_set_total_limits (GTK_DATABOX (box), -1000., 5000., -10000., 23000.);
	gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);
}

void update_scope_trend_view (int channel, int visible)
{
	trendval1[0] = 10; trendval1[1] = 20; trendval1[2] = 30;
	switch (channel)
	{
	case 1:
		if (visible)
		{
			trendv1 = update_trend_graph (trendv1, &trendval1[0], 65535, 0, 0);
		}
		else
		{
			gtk_databox_graph_remove( (GtkDatabox*)box, trendv1 );
		}
		gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);
		break;

	case 2:
		if (visible)
		{
			trendv2 = update_trend_graph (trendv2, &trendval2[0], 0, 65535, 0);
		}
		else
		{
			gtk_databox_graph_remove( (GtkDatabox*)box, trendv2 );
		}
		gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);
		break;

	case 3:
		if (visible)
		{
			trendv3 = update_trend_graph (trendv3, &trendval3[0], 0, 65535, 65535);
		}
		else
		{
			gtk_databox_graph_remove( (GtkDatabox*)box, trendv3 );
		}
		gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);
		break;

	case 4:
		if (visible)
		{
			trendv4 = update_trend_graph (trendv4, &trendval4[0], 0, 0, 65535);
		}
		else
		{
			gtk_databox_graph_remove( (GtkDatabox*)box, trendv4 );
		}
		gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);
		break;
	}
}

void update_scope_snapshot_view (int channel, int visible)
{
	switch (channel)
	{
	case 1:
		if (visible)
		{
			graphv1 = update_scope_graph (graphv1, &snapval1[0], 65535, 0, 0);
		}
		else
		{
			gtk_databox_graph_remove( (GtkDatabox*)box, graphv1 );
		}
		gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);
		break;
		
	case 2:
		if (visible)
		{
			graphv2 = update_scope_graph (graphv2, &snapval2[0], 65535, 65535, 0);
		}
		else
		{
			gtk_databox_graph_remove( (GtkDatabox*)box, graphv2 );
		}
		gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);
		break;
		
	case 3:
		if (visible)
		{
			graphv3 = update_scope_graph (graphv3, &snapval3[0], 0, 65535, 0);
		}
		else
		{
			gtk_databox_graph_remove( (GtkDatabox*)box, graphv3 );
		}
		gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);
		break;
		
	case 4:
		if (visible)
		{
			graphv4 = update_scope_graph (graphv4, &snapval4[0], 0, 0, 65535);
		}
		else
		{
			gtk_databox_graph_remove( (GtkDatabox*)box, graphv4 );
		}
		gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);
		break;
		
	case 5:
		if (visible)
		{
			graphv5 = update_scope_graph (graphv5, &snapval5[0], 0, 65535, 65535);
		}
		else
		{
			gtk_databox_graph_remove( (GtkDatabox*)box, graphv5 );
		}
		gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);
		break;
		
	case 6:
		if (visible)
		{
			graphv6 = update_scope_graph (graphv6, &snapval6[0], 65535, 0, 65535);
		}
		else
		{
			gtk_databox_graph_remove( (GtkDatabox*)box, graphv6 );
		}
		gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);
		break;

	case 7:
		if (visible)
		{
			graphv7 = update_scope_graph (graphv7, &snapval7[0], 65535, 65535, 65535);
		}
		else
		{
			gtk_databox_graph_remove( (GtkDatabox*)box, graphv7 );
		}
		gtk_databox_auto_rescale (GTK_DATABOX (box), 0.05);
		break;
		
	}
}

// update trend data on scope
//	we will trend rotor rpm, stator rpm, power, temperature, 

void update_scope_trend (void)
{
}

void display_scope (int display)
{
	if (display)
	{
		gtk_widget_show ((GtkWidget *)box);
		gtk_widget_show ((GtkWidget *)table);
	}
	else
	{
		gtk_widget_hide ((GtkWidget *)box);
		gtk_widget_hide ((GtkWidget *)table);
	}
}
