//
// eGolf buttons.c
//

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



static int can_s;

void can_cmd (int cmd, int data0)
{
	struct can_frame frame;

	memset (&frame, 0, sizeof (frame));
	frame.can_id = cmd;
	frame.can_dlc = 8;
	frame.data[0] = (char)(data0 & 0xff);
	frame.data[1] = (char)(data0 >> 8);
	write(can_s, &frame, sizeof(frame));
}

int read_full_frame (int sock, char *frame, int size)
{
	int len = 0;
	int count = 0;
	

	// make sure the entire frame gets read
	while (len < size && count < 1000)
	{
		len += read(sock, &frame[len], size - len);
		if (len < size)
		{
			usleep(100);
			count++;
		}
	}
	if (len == size)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void *read_can (void *ptr)
{
	struct can_frame frame;
	static int val=0;
	char buf[30];
	

	while (1)
	{
		// read and display frames
		if (read_full_frame(can_s, (char *)(&frame), sizeof(frame)) > 0)
		{
			// update the appropriate field
//			printf("can data in id%d data %d %d\n", frame.can_id, frame.data[0], frame.data[1]);
			live_data_update (&frame);
		}
		else
		{
			// if the read errored out, don't spin
			sleep (1);
		}
	}
}

// initialize the can socket
// ip link set can0 up type can bitrate 125000
// ifconfig can0 up
//
void init_can (void)
{
	struct ifreq ifr;
	struct sockaddr_can addr;
	static pthread_t	can_reader;
	int bytes;
	struct can_frame frame;
	
	
	// make sure the interface is up
	system ("ip link set can0 up type can bitrate 125000");
//	system ("ip link set can0 up type can bitrate 500000");
	system ("ifconfig can0 up");
	sleep (1);
	
	// get the socket configured
	memset(&ifr, 0x0, sizeof(ifr));
	memset(&addr, 0x0, sizeof(addr));

	/* open CAN_RAW socket */
	can_s = socket(PF_CAN, SOCK_RAW, CAN_RAW);

	/* setup address for bind */
	strcpy(ifr.ifr_name, "can0");
    if (ioctl(can_s, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
    }

    addr.can_ifindex = ifr.ifr_ifindex;	
	addr.can_family = AF_CAN;

	/* bind socket to the can0 interface */
	bind(can_s, (struct sockaddr *)&addr, sizeof(addr));

	// create the reading thread
	pthread_create (&can_reader, NULL, read_can, NULL);

}

