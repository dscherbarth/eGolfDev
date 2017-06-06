//
// eGolf snap.c
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

#include "buttons.h"

#include "ecan.h"
#include "scope.h"
#include "snap.h"

int	snap_fetched = 0;
int	snap_ready = 0;
int snap_fetching = FALSE;
int ctog = 0;
int16_t snapdata1[MAXSNAP];		// space for this snapshot
int16_t snapdata2[MAXSNAP];		// space for this snapshot
int16_t snapdata3[MAXSNAP];		// space for this snapshot
int16_t snapdata4[MAXSNAP];		// space for this snapshot
int16_t snapdata5[MAXSNAP];		// space for this snapshot
int16_t snapdata6[MAXSNAP];		// space for this snapshot
int16_t snapdata7[MAXSNAP];		// space for this snapshot
int16_t snapdata8[MAXSNAP];		// space for this snapshot
int16_t snapdata9[MAXSNAP];		// space for this snapshot
int16_t snapdata10[MAXSNAP];		// space for this snapshot
int16_t snapdata11[MAXSNAP];		// space for this snapshot
int16_t snapdata12[MAXSNAP];		// space for this snapshot
int16_t snapdata13[MAXSNAP];		// space for this snapshot

extern GtkWidget *box;
extern GdkColor color;
extern GtkDataboxGraph *graph1;
extern GtkDataboxGraph *graph2;

void handle_snap_frame (struct can_frame *frame)
{
	switch (frame->can_id)
	{
		case SNAPHEADER :
			frame->can_id = GETSNAP;
			frame->can_dlc = 8;
			can_cmd (GETSNAP, 0);
			break;

		case SNAPDATA1 :
			// save the data
			if (ctog)
			{
				snapdata8[snap_fetched] = frame->data[0] + (frame->data[1] << 8);
				snapdata9[snap_fetched] = frame->data[2] + (frame->data[3] << 8);
				snapdata10[snap_fetched] = frame->data[4] + (frame->data[5] << 8);
			} 
			else
			{
				snapdata1[snap_fetched] = frame->data[0] + (frame->data[1] << 8);
				snapdata2[snap_fetched] = frame->data[2] + (frame->data[3] << 8);
				snapdata3[snap_fetched] = frame->data[4] + (frame->data[5] << 8);
			}
			break;

		case SNAPDATA2 :
			if (snap_fetching)
			{
			// save the data
				if (ctog)
				{
				// save the data
					snapdata11[snap_fetched] = frame->data[0] + (frame->data[1] << 8);
					snapdata12[snap_fetched] = frame->data[2] + (frame->data[3] << 8);
					snapdata13[snap_fetched] = frame->data[4] + (frame->data[5] << 8);
				}
				else
				{
					snapdata4[snap_fetched] = frame->data[0] + (frame->data[1] << 8);
					snapdata5[snap_fetched] = frame->data[2] + (frame->data[3] << 8);
					snapdata6[snap_fetched] = frame->data[4] + (frame->data[5] << 8);
				}
				
				// ask for the next one
				if (ctog ) snap_fetched++;

				// collection toggle
				if (ctog) ctog = 0; else ctog = 1;

				frame->can_id = GETSNAP;
				frame->can_dlc = 8;
				can_cmd (GETSNAP, 0);
			}
			break;

		case SNAPCOMP :
			snap_fetching = FALSE;
			break;
		
	}
}

static int	 sens_avg = 0;
int get_one_torque (void)
{
	int i;
	FILE *sens;
	char sens_data[100];
	int	 sens_val, zero = 18;
	float	mv, zero_pounds, pounds, kgs;


	gdk_threads_enter();
	sens = fopen("/sys/bus/platform/devices/tiadc/iio:device0/in_voltage4_raw", "r");
	if (sens != NULL)
	{
		fread(sens_data, 5, 1, sens);
		sscanf (sens_data, "%d", &sens_val);
	}
	fclose (sens);
	gdk_threads_leave();

	if (i != 10)
	{
		sens_avg = ((sens_avg * 3) + sens_val)/4;
	}
	
	// 12 bits 1.8v full scale means mv = sens_avg/4096 * 1.8
	mv = ((float)(sens_avg - zero) / 4096.0 * 1.8) * 1000.0;
	
	pounds = mv * .5119;	// linear?

	return pounds * 10;
		
}

void *snap_wait (void *ptr)
{
	int i;

	switch(get_snapres())
	{
	case 0:						// .25 seconds
		for(i=0; i<2000; i++)
		{
			snapdata7[i] = 0;
			usleep (150);
		}
		break;
	case 1:						// .5 seconds
		for(i=0; i<2000; i++)
		{
			snapdata7[i] = 0;
			usleep (300);
		}
		break;
	case 2:						// 2.5 seconds
		for(i=0; i<2000; i++)
		{
			snapdata7[i] = 0;
			usleep (1300);
//			snapdata7[i] = get_one_torque ();
		}
		break;
	case 3:						// 5 seconds
		for(i=0; i<2000; i++)
		{
			snapdata7[i] = 0;
			usleep (2600);
//			snapdata7[i] = get_one_torque ();
//			usleep (100);
		}
		break;
	case 4:						// 10 seconds
		for(i=0; i<2000; i++)
		{
			snapdata7[i] = 0;
			usleep (5200);
//			snapdata7[i] = get_one_torque ();
//			usleep (2100);
		}
		break;
	}
	snap_ready = 1;
}

extern int state;

char *srestime[] = {".25S", ".5S", "2.5S", "5S", "10S"};

void *snap_get (void *ptr)
{
	int i;
	static char temp[80];
	static snapnum = 0;
	FILE * snapf;
	char tstr[200];
	time_t t;
	struct tm *tmp;

	
	// wait for the collect to be done
	// if we are faulted, just start getting the data now
	if (state != FAULTED)
	{
		for (i=0; i<150; i++)
		{
			usleep(100000);	// .1 sec
			if (snap_ready == 1)
			{
				break;
			}
		}
	}
	
	// start the fetch
	snap_fetched = 0;
	snap_fetching = TRUE;
	can_cmd (GETSNAP, 0);

	// update the fetch status
	// make sure this does not take too long and time out if so
	// on complete, update the databox
	for (i=0; i<100; i++)
	{
		usleep(150000);	// .1 sec
		sprintf(temp, "got %d records", snap_fetched);
		gdk_threads_enter();
		disp_msg (temp);
		gdk_threads_leave();
		if (!snap_fetching)
		{
			break;
		}
	}
	
	// handle cleanup
//	if (i == 100)
	if (i == 100)
	{
		sprintf(temp, "Timed out fetching snapshot at %d", snap_fetched);
		gdk_threads_enter();
		disp_msg (temp);
		gdk_threads_leave();
		sleep(5);
		gdk_threads_enter();
		clear_msg();
		gdk_threads_leave();
	}
	else
	{					
		gdk_threads_enter();
		clear_msg();

		// update the databox with new data
		update_scope_snapshot_data (&snapdata1[0], 
									&snapdata2[0], 
									&snapdata3[0], 
									&snapdata4[0], 
									&snapdata5[0], 
									&snapdata6[0], 
									&snapdata7[0], 
									MAXSNAP);
		chan_sel ();
		
		// save this data to an appropriate csv file

		// discover what the next snap data file will be
		t = time (NULL);
		tmp = localtime (&t);
		strftime (tstr, 200, "%F_%H:%M:%S", tmp);
		sprintf (temp, "SNAP_%s.csv", tstr);
		snapf = fopen(temp, "w+");
		if (snapf != NULL)
		{
			snapnum++;
			fprintf (snapf, "Qsp, Iq, Vq, Ia, Ib, Ic, Id, Vd, Sa, RPM, sin, cos \n");
			for(i=0; i<2000; i++)
			{
				fprintf(snapf, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n", 	
snapdata1[i], snapdata2[i], snapdata3[i], snapdata4[i], snapdata5[i], snapdata6[i], 
snapdata8[i],snapdata9[i],snapdata10[i],snapdata11[i],snapdata12[i],snapdata13[i] ); 
			}
			fclose (snapf);

		gdk_threads_leave();
		gdk_threads_enter();
			sprintf(temp, "Saved as snap #%d", snapnum);
			disp_msg (temp);
		gdk_threads_leave();
			sleep(2);
		gdk_threads_enter();
			clear_msg();
		gdk_threads_leave();
		gdk_threads_enter();
		}
		
		gdk_threads_leave();
	}

}

static pthread_t	snap_1;
static pthread_t	snap_2;
void snapshot_action (void) 
{
	int i;

	// reset the collection toggle
	ctog = 0;
	
	// trigger the snapshot
	can_cmd (STARTSNAP, get_snapres());
	
	// wait an appropriate time
	snap_ready = 0;
	disp_msg ("Snapshotting..");
	pthread_create (&snap_1, NULL, snap_wait, NULL);

	// start a thread to wait/update snapshot results
	pthread_create (&snap_2, NULL, snap_get, NULL);

}

