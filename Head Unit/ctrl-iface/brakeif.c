#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <memory.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

/**********************************************************************
*
* brakeif.c
*
*	interface for access to the brake light switch
*
***********************************************************************/


int brakefd;

int set_interface_attribs (int fd)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		return -1;
	}

	cfsetospeed (&tty, B115200);
	cfsetispeed (&tty, B115200);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK;         // disable break processing
	tty.c_lflag = 0;                // no signaling chars, no echo,
									// no canonical processing
	tty.c_oflag = 0;                // no remapping, no delays
	tty.c_cc[VMIN]  = 1;            // read does block
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
									// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag |= CRTSCTS;			// previously made sure that the tinyg
									// is configured for this

	if (tcsetattr (fd, TCSANOW, &tty) != 0)
	{
		return -1;
	}
	
    return 0;
}

void set_blocking (int fd, int should_block)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		return;
	}

	tty.c_cc[VMIN]  = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	tcsetattr (fd, TCSANOW, &tty);
}

void openSerial()
{
	int i;
	char temp[80];
	
	
	for (i = 0; i<100; i++)
	{
		usleep(100);
		sprintf(temp, "/dev/ttyACM%d", i);
		brakefd = open (temp, O_RDWR | O_NOCTTY | O_SYNC);
		if (brakefd > 0)
		{
			break; // got a connection
		}
	}
	if (i == 100)
	{
		usleep(100000);
	}
	else
	{
		set_interface_attribs (brakefd);  // set speed to 115,200 bps, 8n1 (no parity)
		set_blocking (brakefd, 1);                // set blocking
		usleep(100000);
	}
}

unsigned int getCount(void)
{
	char buf[32];
	int count = 0;
	unsigned int result = 0;
	
	
	write(brakefd, "C", 1);
	while(1)
	{
		read(brakefd, &buf[count], 1);
		if(buf[count] == 'x')
		{
			buf[count] = 0;
			break;
		}
		count++;
	}
	sscanf(buf, "%ul", &result);
	return result;
}

int brakelightstate = 0;
setBraking(int on)
{
	if(on != brakelightstate)
	{
		if(on)
		{
			write(brakefd, "S", 1);
		}
		else
		{
			write(brakefd, "G", 1);
		}
		brakelightstate = on;
	}
}