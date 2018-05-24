//
// eGolf batt.c
//

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

#include <string.h>
#include <time.h>

void *update_battery_data (void *ptr);
void *update_battery_window (void *ptr);

void batt_fetch_and_parse (void);
void batt_data_update (void);

static int sockfd = 0;
int init_battmon (void)
{
	static pthread_t	batt_update;
	static pthread_t	batt_update2;

	// can bus is already running just start the threads
	
	// construct the drawing area and the labels to hold voltage data
	
	// we are ready to get battery data
	// create and start the update threads
	pthread_create (&batt_update, NULL, update_battery_data, NULL);
	pthread_create (&batt_update2, NULL, update_battery_window, NULL);
	return 1;
}

GtkWidget 	*htrlabel = NULL;
GtkWidget 	*pvlabel = NULL;
GtkWidget 	*btlabel = NULL;
GtkWidget 	*lvlabel = NULL;
GtkWidget 	*hvlabel = NULL;
GtkWidget 	*htrdata = NULL;
GtkWidget 	*pvdata = NULL;
GtkWidget 	*btdata = NULL;
GtkWidget 	*lvdata = NULL;
GtkWidget 	*hvdata = NULL;
void batt_data_init (GtkWidget *topwin, PangoFontDescription *dfdata)
{
	pvlabel = gtk_label_new ("Pack Volts");
	gtk_widget_modify_font(pvlabel, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), pvlabel, 5, 660);
	btlabel = gtk_label_new ("Pack Temp");
	gtk_widget_modify_font(btlabel, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), btlabel, 5, 690);
	lvlabel = gtk_label_new ("Low Volt");
	gtk_widget_modify_font(lvlabel, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), lvlabel, 5, 720);
	hvlabel = gtk_label_new ("High Volt");
	gtk_widget_modify_font(hvlabel, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), hvlabel, 5, 750);

	pvdata = gtk_label_new ("0.0");
	gtk_widget_modify_font(pvdata, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), pvdata, 170, 660);
	btdata = gtk_label_new ("0.0");
	gtk_widget_modify_font(btdata, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), btdata, 170, 690);
	lvdata = gtk_label_new ("0.0");
	gtk_widget_modify_font(lvdata, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), lvdata, 170, 720);
	hvdata = gtk_label_new ("0.0");
	gtk_widget_modify_font(hvdata, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), hvdata, 170, 750);

	// heater temp
	htrlabel = gtk_label_new ("HTemp");
	gtk_widget_modify_font(htrlabel, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), htrlabel, 1050, 490);
	htrdata = gtk_label_new ("000");
	gtk_widget_modify_font(htrdata, dfdata);  
	gtk_fixed_put(GTK_FIXED(topwin), htrdata, 1140, 490);

}

GtkWidget	*batt_cell[90];
GtkWidget	*batt_temp[10];

void init_batt_bar (GtkWidget *fixed_win)
{
	GdkColor green = {0, 0x0000, 0xffff, 0x0000};
    GdkColor white = {0, 0xffff, 0xffff, 0xffff};
    GdkColor black = {0, 0x0000, 0x0000, 0x0000};
	
	int i, xloc;
	
	// add batt cell bars
	for (i=0; i<88; i++)
	{
		batt_cell[i+2] = gtk_progress_bar_new ();
		xloc = 350 + i*8 + i/11;
		gtk_fixed_put(GTK_FIXED(fixed_win), batt_cell[i+2], xloc, 680);
		gtk_widget_set_size_request(batt_cell[i+2], 6, 100);
		gtk_progress_bar_set_orientation ((GtkProgressBar *)batt_cell[i+2], GTK_PROGRESS_BOTTOM_TO_TOP);
		gtk_widget_modify_bg((GtkWidget *)batt_cell[i+2], GTK_STATE_SELECTED, &green);
		gtk_widget_modify_fg((GtkWidget *)batt_cell[i+2], GTK_STATE_NORMAL, &white);
		gtk_widget_modify_bg((GtkWidget *)batt_cell[i+2], GTK_STATE_NORMAL, &black);
	}
	
	// add temp bars
	for (i=0; i<8; i++)
	{
		batt_temp[i+2] = gtk_progress_bar_new ();
		xloc = 350 + i*89;
		gtk_fixed_put(GTK_FIXED(fixed_win), batt_temp[i+2], xloc, 660);
		gtk_widget_set_size_request(batt_temp[i+2], 80, 8);
		gtk_progress_bar_set_orientation ((GtkProgressBar *)batt_temp[i+2], GTK_PROGRESS_LEFT_TO_RIGHT);
		gtk_widget_modify_bg((GtkWidget *)batt_temp[i+2], GTK_STATE_SELECTED, &green);
		gtk_widget_modify_fg((GtkWidget *)batt_temp[i+2], GTK_STATE_NORMAL, &white);
		gtk_widget_modify_bg((GtkWidget *)batt_temp[i+2], GTK_STATE_NORMAL, &black);
	}
}

float cells [88];  // values from 0 to 99 representing 2.8v to 4.2v
float temps [8];

int hvb, hvc;			// high volt location
int lowest_volt = 42000;
int lvb, lvc;			// low volt location
void update_batt_bar()
{
	GdkColor green = {0, 0x0000, 0xffff, 0x0000};
	GdkColor red = {0, 0xffff, 0x0000, 0x0000};
	GdkColor blue = {0, 0x0000, 0x0000, 0xffff};
	int i;

	gdk_threads_enter();
	for (i=0; i<88; i++)
	{
		gtk_progress_bar_set_fraction ((GtkProgressBar *)batt_cell[i+2], cells[i]);
		if(i == (lvb * 11 + lvc))
		{
			gtk_widget_modify_bg((GtkWidget *)batt_cell[i+2], GTK_STATE_SELECTED, &blue);
		}
		else if(i == (hvb * 11 + hvc))
		{
			gtk_widget_modify_bg((GtkWidget *)batt_cell[i+2], GTK_STATE_SELECTED, &red);
		}
		else
		{
			gtk_widget_modify_bg((GtkWidget *)batt_cell[i+2], GTK_STATE_SELECTED, &green);
		}
	}
	
	// temps
	for (i=0; i<8; i++)
	{
		gtk_progress_bar_set_fraction ((GtkProgressBar *)batt_temp[i+2], temps[i]);
	}
	
	gdk_threads_leave();
}

int snap_pause = 0;
void snap_pausef(void)
{
	snap_pause = 1;
}
void snap_continue(void)
{
	snap_pause = 0;
}

int freshness = 0;
void *update_battery_data (void *ptr)
{
	while(1)
	{
		if (!snap_pause)
		{
			usleep(500000);
			freshness++;
			batt_fetch ();
		}
		else
		{
			usleep(500000);
		}
	}
}

void *update_battery_window (void *ptr)
{
	while(1)
	{
		if (!snap_pause)
		{
			usleep(750000);
			batt_data_update ();
		}
		else
		{
			usleep(500000);
		}
	}
}

int highest_temp = 0;
int htb, htc;			// high temp location
int highest_volt = 0;
int pack_volts = 0;

void can_cmd (int cmd, int data0);

int bank_waiting[8] = {0,0,0,0,0,0,0,0};
int fetch_cell = 0;

void batt_fetch (void)
{
	can_cmd (524, 1);
	can_cmd (524, 2 | fetch_cell << 8);
	bank_waiting[fetch_cell]++;		// need to hear back 
	fetch_cell++; if(fetch_cell > 7)fetch_cell = 0;

#ifdef HTR_DEBUG	
	if(fetch_cell == 0)
	{
		can_cmd(544, 1);
	}
#endif
}

int htrtemp = 0;
int htrout = 0;
void htr_parse (unsigned char *bbuffer)
{
	char temp[30];
	
	htrtemp = (int)bbuffer[1];
	htrout = (int)bbuffer[2];

	gdk_threads_enter();
	sprintf (temp, "%03d (%03d)", htrtemp, htrout);
	gtk_label_set (GTK_LABEL (htrdata), temp);	
	gdk_threads_leave();
}

void batt_parse (unsigned char *bbuffer)
{
	int 	i, j, hindex, lindex, int1, int2, tb;
	int hightemp;
	
	// parse into cell volts and temps
//	pack_volts = -1;		// indicate no data
	if (bbuffer[0] == 123)
	{
		// indicate that we have some data
		freshness = 0;
		
		// we have reasonable data to parse
		// get tv

		// combine bytes
		int1 = bbuffer[1];
		int2 = bbuffer[2];
		pack_volts = ((int1 & 0xff)*256) + (int2 & 0xff);
		pack_volts /= 100;
		
		int1 = bbuffer[11];
		int2 = bbuffer[12];
		highest_temp = ((int1 & 0xff)*256) + (int2 & 0xff);
		highest_temp = ((15000 - highest_temp)/200) + 25;
		htb = bbuffer[13]; htc = bbuffer[14];
		
		int1 = bbuffer[3];
		int2 = bbuffer[4];
		lowest_volt = ((int1 & 0xff)*256) + (int2 & 0xff);
		lvb = bbuffer[5]; lvc = bbuffer[6];

		int1 = bbuffer[7];
		int2 = bbuffer[8];
		highest_volt = ((int1 & 0xff)*256) + (int2 & 0xff);
		hvb = bbuffer[9]; hvc = bbuffer[10];
		
	} else 
	{
		if (bbuffer[0] >= 0 && bbuffer[0] < 8)
		{
			// parse detailed cell info
			bank_waiting[bbuffer[0]] = 0;	// reset the counter
			
			// volts
			for(i=0; i<11; i++)
			{
				cells[ i + (11 * (int)bbuffer[0])] = (float)bbuffer[i+1] / 140.0;
			}
			
			// get the highest temp
			hightemp = 0;
			for(i=0; i<5; i++)
			{
				if (bbuffer[i+12] > hightemp) hightemp = bbuffer[i+12];
			}
			
			temps[(int)bbuffer[0]] = (float)hightemp / 100.0;	// range is 0 to 100C
	
		}
	}
	
	update_batt_bar();

}

void heateron_action (void) 
{
	can_cmd (544, 2);
}

void heateroff_action (void) 
{
	can_cmd (544, 3);
}

void battcharge_action (void) 
{
	can_cmd (524, 5);
}

void battmonitor_action (void) 
{
	can_cmd (524, 3);
}

void battequalize_action (void) 
{
	can_cmd (524, 4);
}

GtkWidget *htron;
GtkWidget *htroff;
GtkWidget *battcharge;
GtkWidget *battmonitor;
GtkWidget *battequalize;
void 	create_batt_buttons(GtkWidget *fixed_win)
{
	htron = gtk_button_new_with_label("HTR ON");
	gtk_fixed_put(GTK_FIXED(fixed_win), htron, 1160, 530);
	gtk_widget_set_size_request(htron, 85, 45);
	gtk_signal_connect (GTK_OBJECT (htron), "clicked",
						GTK_SIGNAL_FUNC (heateron_action), NULL);
	htroff = gtk_button_new_with_label("HTR OFF");
	gtk_fixed_put(GTK_FIXED(fixed_win), htroff, 1160, 580);
	gtk_widget_set_size_request(htroff, 85, 45);
	gtk_signal_connect (GTK_OBJECT (htroff), "clicked",
						GTK_SIGNAL_FUNC (heateroff_action), NULL);

	battcharge = gtk_button_new_with_label("Charge");
	gtk_fixed_put(GTK_FIXED(fixed_win), battcharge, 1060, 630);
	gtk_widget_set_size_request(battcharge, 85, 45);
	gtk_signal_connect (GTK_OBJECT (battcharge), "clicked",
						GTK_SIGNAL_FUNC (battcharge_action), NULL);

	battmonitor = gtk_button_new_with_label("Monitor");
	gtk_fixed_put(GTK_FIXED(fixed_win), battmonitor, 1060, 680);
	gtk_widget_set_size_request(battmonitor, 85, 45);
	gtk_signal_connect (GTK_OBJECT (battmonitor), "clicked",
						GTK_SIGNAL_FUNC (battmonitor_action), NULL);

	battequalize = gtk_button_new_with_label("Equalize");
	gtk_fixed_put(GTK_FIXED(fixed_win), battequalize, 1060, 730);
	gtk_widget_set_size_request(battequalize, 85, 45);
	gtk_signal_connect (GTK_OBJECT (battequalize), "clicked",
						GTK_SIGNAL_FUNC (battequalize_action), NULL);
}

void batt_data_update (void)
{
	char temp[80];
	GdkColor red = {0, 0xffff, 0x0000, 0x0000};
    GdkColor black = {0, 0x0000, 0x0000, 0x0000};
	float std_dev, avg;
	int i, j;
	
	
	if( freshness > 5)
	{
		int i;
		
		// reset data values
		highest_temp = 0;
		highest_volt = 0;
		lowest_volt = 0;
		pack_volts = 0;
		for(i=0; i<88; i++) cells [i] = 0;
		for(i=0; i<8; i++) temps [i] = 0;
	}
	
	for(i=0; i<8; i++)
	{
		if(bank_waiting[i] > 5)	// data is old
		{
			for(j=0; j<11; j++) cells [i*11+j] = 0;
			temps [i] = 0;
		}
	}
	// calc average
	avg = 0;
	for(i=0; i<88; i++)
	{
		avg += cells[i];
	}
	
	avg /= 88;
	std_dev = 0;
	for(i=0; i<88; i++)
	{
		if(cells[i] > avg) std_dev += (cells[i] - avg);
		if(cells[i] < avg) std_dev += (avg - cells[i]);
	}
	
	update_batt_bar();

	gdk_threads_enter();
	sprintf (temp, "%d (%4.2f)", pack_volts, std_dev);
	gtk_label_set (GTK_LABEL (pvdata), temp);	
	sprintf (temp, "%d (%d/%d)", highest_temp, htb, htc);
	gtk_label_set (GTK_LABEL (btdata), temp);	

	sprintf (temp, "%4.2f (%d/%d)", ((float)lowest_volt/10000.0), lvb, lvc);
	gtk_label_set (GTK_LABEL (lvdata), temp);	
	sprintf (temp, "%4.2f (%d/%d)", ((float)highest_volt/10000.0), hvb, hvc);
	gtk_label_set (GTK_LABEL (hvdata), temp);	

	// set an alarm color
	if (lowest_volt < 28000)
	{
		gtk_widget_modify_fg((GtkWidget *)lvdata, GTK_STATE_NORMAL, &red);
	}
	else
	{
		gtk_widget_modify_fg((GtkWidget *)lvdata, GTK_STATE_NORMAL, &black);
	}
	if (highest_volt > 42000)
	{
		gtk_widget_modify_fg((GtkWidget *)hvdata, GTK_STATE_NORMAL, &red);
	}
	else
	{
		gtk_widget_modify_fg((GtkWidget *)hvdata, GTK_STATE_NORMAL, &black);
	}
	gdk_threads_leave();

}

#ifdef ref
void sendData( int sockfd, int x ) {
  int n;

  char buffer[32];
  sprintf( buffer, "%d\n", x );
  if ( (n = write( sockfd, buffer, strlen(buffer) ) ) < 0 )
      error( const_cast<char *>( "ERROR writing to socket") );
  buffer[n] = '\0';
}

int getData( int sockfd ) {
  char buffer[32];
  int n;

  if ( (n = read(sockfd,buffer,31) ) < 0 )
       error( const_cast<char *>( "ERROR reading from socket") );
  buffer[n] = '\0';
  return atoi( buffer );
}

int main(int argc, char *argv[])
{
    int sockfd, portno = 51717, n;
    char serverIp[] = "169.254.0.2";
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];
    int data;

    if (argc < 3) {
      // error( const_cast<char *>( "usage myClient2 hostname port\n" ) );
      printf( "contacting %s on port %d\n", serverIp, portno );
      // exit(0);
    }
    if ( ( sockfd = socket(AF_INET, SOCK_STREAM, 0) ) < 0 )
        error( const_cast<char *>( "ERROR opening socket") );

    if ( ( server = gethostbyname( serverIp ) ) == NULL ) 
        error( const_cast<char *>("ERROR, no such host\n") );
    
    bzero( (char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy( (char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if ( connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        error( const_cast<char *>( "ERROR connecting") );

    for ( n = 0; n < 10; n++ ) {
      sendData( sockfd, n );
      data = getData( sockfd );
      printf("%d ->  %d\n",n, data );
    }
    sendData( sockfd, -2 );

    close( sockfd );
    return 0;
}

#endif