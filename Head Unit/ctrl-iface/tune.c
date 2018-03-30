//
// eGolf tune.c
//

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <string.h>
#include <time.h>
#include <errno.h>
 
#include "tune.h"
#include "ecan.h"
#include "buttons.h"

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
int tunetr = 0, tunerf = 0,	tuneda = 0;

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

extern PangoFontDescription *dfsmall;
extern PangoFontDescription *dfdata;
extern GtkWidget *window;
extern GtkWidget *fixed;

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

extern int tuneindex;

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

void tuneget_action (void)
{
	
	
	// fetch tune values and update the control values
	can_cmd (390, 0);	// make the request

	gtk_label_set (GTK_LABEL (tuneval1), "YYYY");	

	// wait for responses
	tuneindex = 0;

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
