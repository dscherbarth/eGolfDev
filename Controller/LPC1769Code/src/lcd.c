#include "LPC17xx.h"
#include "type.h"
#include "timer.h"
#include "i2c.h"
#include "string.h"
#include "lcd.h"
#include "gpio.h"

volatile uint32_t I2CMasterState[I2C_PORT_NUM] = {I2C_IDLE,I2C_IDLE,I2C_IDLE};
volatile uint32_t timeout[I2C_PORT_NUM] = {0, 0, 0};

volatile uint8_t I2CMasterBuffer[I2C_PORT_NUM][BUFSIZE];
volatile uint8_t I2CSlaveBuffer[I2C_PORT_NUM][BUFSIZE];
volatile uint32_t I2CCount[I2C_PORT_NUM] = {0, 0, 0};
volatile uint32_t I2CReadLength[I2C_PORT_NUM];
volatile uint32_t I2CWriteLength[I2C_PORT_NUM];

volatile uint32_t RdIndex0 = 0, RdIndex1 = 0, RdIndex2 = 0;
volatile uint32_t WrIndex0 = 0, WrIndex1 = 0, WrIndex2 = 0;


#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

void lcd_send(char *buffer, int len);
void lcd_init_cmd(void);

/*
From device to device, the I2C communication protocol may vary,
in the example below, the protocol uses repeated start to read data from or
write to the device:
For master read: the sequence is: STA,Addr(W),offset,RE-STA,Addr(r),data...STO
for master write: the sequence is: STA,Addr(W),offset,RE-STA,Addr(w),data...STO
Thus, in state 8, the address is always WRITE. in state 10, the address could
be READ or WRITE depending on the I2C command.
*/

/*****************************************************************************
** Function name:		I2C_IRQHandler
**
** Descriptions:		I2C interrupt handler, deal with master mode only.
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
#ifdef lcd
void I2C0_IRQHandler(void)
{
  uint8_t StatValue;

  timeout[0] = 0;
  /* this handler deals with master read and master write only */
  StatValue = LPC_I2C0->STAT;
  switch ( StatValue )
  {
	case 0x08:			/* A Start condition is issued. */
	WrIndex0 = 0;
	RdIndex0 = 0;		// dwb :-)
	LPC_I2C0->DAT = I2CMasterBuffer[0][WrIndex0++];
	LPC_I2C0->CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC);
	break;

	case 0x10:			/* A repeated started is issued */
	RdIndex0 = 0;
	/* Send SLA with R bit set, */
	LPC_I2C0->DAT = I2CMasterBuffer[0][WrIndex0++];
	LPC_I2C0->CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC);
	break;

	case 0x18:			/* Regardless, it's a ACK */
	if ( I2CWriteLength[0] == 1 )
	{
	  LPC_I2C0->CONSET = I2CONSET_STO;      /* Set Stop flag */
	  I2CMasterState[0] = I2C_NO_DATA;
	}
	else
	{
	  LPC_I2C0->DAT = I2CMasterBuffer[0][WrIndex0++];
	}
	LPC_I2C0->CONCLR = I2CONCLR_SIC;
	break;

	case 0x28:	/* Data byte has been transmitted, regardless ACK or NACK */
	if ( WrIndex0 < I2CWriteLength[0] )
	{
	  LPC_I2C0->DAT = I2CMasterBuffer[0][WrIndex0++]; /* this should be the last one */
	}
	else
	{
	  if ( I2CReadLength[0] != 0 )
	  {
		LPC_I2C0->CONSET = I2CONSET_STA;	/* Set Repeated-start flag */
	  }
	  else
	  {
		LPC_I2C0->CONSET = I2CONSET_STO;      /* Set Stop flag */
		I2CMasterState[0] = I2C_OK;
	  }
	}
	LPC_I2C0->CONCLR = I2CONCLR_SIC;
	break;

	case 0x30:
	LPC_I2C0->CONSET = I2CONSET_STO;      /* Set Stop flag */
	I2CMasterState[0] = I2C_NACK_ON_DATA;
	LPC_I2C0->CONCLR = I2CONCLR_SIC;
	break;

	case 0x40:	/* Master Receive, SLA_R has been sent */
	if ( (RdIndex0 + 1) < I2CReadLength[0] )
	{
	  /* Will go to State 0x50 */
	  LPC_I2C0->CONSET = I2CONSET_AA;	/* assert ACK after data is received */
	}
	else
	{
	  /* Will go to State 0x58 */
	  LPC_I2C0->CONCLR = I2CONCLR_AAC;	/* assert NACK after data is received */
	}
	LPC_I2C0->CONCLR = I2CONCLR_SIC;
	break;

	case 0x50:	/* Data byte has been received, regardless following ACK or NACK */
	I2CSlaveBuffer[0][RdIndex0++] = LPC_I2C0->DAT;
	if ( (RdIndex0 + 1) < I2CReadLength[0] )
	{
	  LPC_I2C0->CONSET = I2CONSET_AA;	/* assert ACK after data is received */
	}
	else
	{
	  LPC_I2C0->CONCLR = I2CONCLR_AAC;	/* assert NACK on last byte */
	}
	LPC_I2C0->CONCLR = I2CONCLR_SIC;
	break;

	case 0x58:
	I2CSlaveBuffer[0][RdIndex0++] = LPC_I2C0->DAT;
	I2CMasterState[0] = I2C_OK;
	LPC_I2C0->CONSET = I2CONSET_STO;	/* Set Stop flag */
	LPC_I2C0->CONCLR = I2CONCLR_SIC;	/* Clear SI flag */
	break;

	case 0x20:		/* regardless, it's a NACK */
	case 0x48:
	LPC_I2C0->CONSET = I2CONSET_STO;      /* Set Stop flag */
	I2CMasterState[0] = I2C_NACK_ON_ADDRESS;
	LPC_I2C0->CONCLR = I2CONCLR_SIC;
	break;

	case 0x38:		/* Arbitration lost, in this example, we don't
					deal with multiple master situation */
	default:
	I2CMasterState[0] = I2C_ARBITRATION_LOST;
	LPC_I2C0->CONCLR = I2CONCLR_SIC;
	break;
  }
  return;
}

/*****************************************************************************
** Function name:		I2CStart
**
** Descriptions:		Create I2C start condition, a timeout
**				value is set if the I2C never gets started,
**				and timed out. It's a fatal error.
**
** parameters:			None
** Returned value:		true or false, return false if timed out
**
*****************************************************************************/
uint32_t I2CStart( uint32_t portNum )
{
  uint32_t retVal = FALSE;

  timeout[portNum] = 0;
  /*--- Issue a start condition ---*/
  LPC_I2C[portNum]->CONSET = I2CONSET_STA;	/* Set Start flag */

  /*--- Wait until START transmitted ---*/
  while( 1 )
  {
	if ( I2CMasterState[portNum] == I2C_STARTED )
	{
	  retVal = TRUE;
	  break;
	}
	if ( timeout[portNum] >= MAX_TIMEOUT )
	{
	  retVal = FALSE;
	  break;
	}
	timeout[portNum]++;
  }
  return( retVal );
}

/*****************************************************************************
** Function name:		I2CStop
**
** Descriptions:		Set the I2C stop condition, if the routine
**				never exit, it's a fatal bus error.
**
** parameters:			None
** Returned value:		true or never return
**
*****************************************************************************/
uint32_t I2CStop( uint32_t portNum )
{
  LPC_I2C[portNum]->CONSET = I2CONSET_STO;      /* Set Stop flag */
  LPC_I2C[portNum]->CONCLR = I2CONCLR_SIC;  /* Clear SI flag */

  /*--- Wait for STOP detected ---*/
  while( LPC_I2C[portNum]->CONSET & I2CONSET_STO );
  return TRUE;
}

/*****************************************************************************
** Function name:		I2CInit
**
** Descriptions:		Initialize I2C controller as a master
**
** parameters:			None
** Returned value:		None
**
*****************************************************************************/
void I2C0Init( void )
{
  LPC_SC->PCONP |= (1 << 7);

  /* set PIO0.27 and PIO0.28 to I2C0 SDA and SCL */
  /* function to 01 on both SDA and SCL. */
  LPC_PINCON->PINSEL1 &= ~((0x03<<22)|(0x03<<24));
  LPC_PINCON->PINSEL1 |= ((0x01<<22)|(0x01<<24));

  /*--- Clear flags ---*/
  LPC_I2C0->CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC | I2CONCLR_I2ENC;

  /*--- Reset registers ---*/
#if FAST_MODE_PLUS
  LPC_PINCON->I2CPADCFG |= ((0x1<<0)|(0x1<<2));
  LPC_I2C0->SCLL   = I2SCLL_HS_SCLL;
  LPC_I2C0->SCLH   = I2SCLH_HS_SCLH;
#else
  LPC_PINCON->I2CPADCFG &= ~((0x1<<0)|(0x1<<2));
  LPC_I2C0->SCLL   = I2SCLL_SCLL;
  LPC_I2C0->SCLH   = I2SCLH_SCLH;
#endif

//  /* Install interrupt handler */
  NVIC_EnableIRQ(I2C0_IRQn);

  LPC_I2C0->CONSET = I2CONSET_I2EN;
  return;
}

/*****************************************************************************
** Function name:		I2CEngine
**
** Descriptions:		The routine to complete a I2C transaction
**						from start to stop. All the intermitten
**						steps are handled in the interrupt handler.
**						Before this routine is called, the read
**						length, write length, I2C master buffer,
**						and I2C command fields need to be filled.
**						see i2cmst.c for more details.
**
** parameters:			I2C port number
** Returned value:		master state of current I2C port.
**
*****************************************************************************/
uint32_t I2CEngine( uint32_t portNum )
{


  /*--- Issue a start condition ---*/
  I2CMasterState[portNum] = I2C_BUSY;
  LPC_I2C0->CONSET = I2CONSET_STA;

  // read the time in .25 msec
  // if an operation is in progress and no timeout just return
  // if timed out, mark as such and re-start
  // else save the time and start the operation

  while ( I2CMasterState[portNum] == I2C_BUSY )
  {
	if ( timeout[portNum] >= MAX_TIMEOUT )
	{
	  I2CMasterState[portNum] = I2C_TIME_OUT;
	  LPC_I2C0->CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC | I2CONCLR_I2ENC;
	  LPC_I2C0->CONSET = I2CONSET_I2EN;

	  break;
	}
	timeout[portNum]++;
  }
  LPC_I2C0->CONCLR = I2CONCLR_STAC;

  return ( I2CMasterState[portNum] );
}

/******************************************************************************
** Function name:		lcd_clear
**
** Descriptions:		erase the lcd display
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_clear(void)
{
	  lcd_cursor(0,0);
	  lcd_write("                    ", 20);
	  lcd_cursor(1,0);
	  lcd_write("                    ", 20);
}

/******************************************************************************
** Function name:		lcd_cursor
**
** Descriptions:		set the cursor to row and column
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/
void lcd_cursor (int row, int col)
{
	  char configbuf[4];

	  configbuf[0] = 0x78; configbuf[1] = 0x00; configbuf[2] = 0x80 | (col + row*0x40);
	  lcd_send(configbuf, 3);

}

static uint32_t cursorOn = 0;
static uint32_t selcurR = 0;
static uint32_t selcurC = 0;

void lcd_cursor_on ( uint32_t R, uint32_t C )
{
	cursorOn = 1;
	selcurR = R;
	selcurC = C;
}

void lcd_cursor_off ( void )
{
	cursorOn = 0;
}

void lcd_write(char *buffer, int len)
{
	char tempbuf[44];
	int i;


	tempbuf[0] = 0x78; tempbuf[1] = 0x40;
	for(i=0;i<len;i++) tempbuf[i+2] = buffer[i];
	lcd_send(tempbuf, len+2);

	if (cursorOn)
	{
		tempbuf[0] = 0x78; tempbuf[1] = 0x00; tempbuf[2] = 0x0f;
		lcd_send(tempbuf, 3);
		lcd_cursor (selcurR, selcurC);
	}
	else
	{
		tempbuf[0] = 0x78; tempbuf[1] = 0x00; tempbuf[2] = 0x0c;
		lcd_send(tempbuf, 3);
	}

}

void lcd_init_cmd(void)
{
	  char configbuf[12];

	  configbuf[0] = 0x78; configbuf[1] = 0x00; configbuf[2] = 0x38;
	  configbuf[3] = 0x39;
	  configbuf[4] = 0x14; configbuf[5] = 0x78; configbuf[6] = 0x5e;
	  configbuf[7] = 0x6d; configbuf[8] = 0x0c; configbuf[9] = 0x01; configbuf[10] = 0x06;
	  lcd_send(configbuf, 11);
	  delayMs(0, 10);

}

void lcd_init(void)
{
	  I2C0Init( );			/* initialize I2c0 */

	  lcd_init_cmd();
}

void lcd_send(char *buffer, int len)
{
	int i;

	// form up and send the request
	I2CWriteLength[0] = len;
	I2CReadLength[0] = 0;
	for (i=0; i<len; i++) I2CMasterBuffer[0][i] = buffer[i];
	I2CEngine( 0 );

}

void data_send(char *buffer, int len)  // low level routine
{
	int i;

	// form up and send the request
	I2CWriteLength[0] = len;
	I2CReadLength[0] = 0;
	for (i=0; i<len; i++) I2CMasterBuffer[0][i] = buffer[i];
	I2CEngine( 0 );

}

void data_write(char type, char *buffer, int len)		// high level routine
{
	char tempbuf[30];
	int i;

	tempbuf[0] = 0x04; tempbuf[1] = type;	//tempbuf[0] is address, tempbuf[1] is command
	for(i=0;i<len;i++) tempbuf[i+2] = buffer[i];
	data_send(tempbuf, len+2);
}

void data_request(char *buffer, uint8_t len)
{
	int i;

	// form up and send the request
	I2CWriteLength[0] = 0;
	I2CReadLength[0] = len;
//	for (i=0; i<len; i++) I2CSlaveBuffer[0][i] = 0;
	I2CMasterBuffer[0][0] = 0x5;	// address 4 + read bit
	I2CEngine( 0 );
	for (i=0; i<len; i++) {
		buffer[i] = I2CSlaveBuffer[0][i];
		//I2CSlaveBuffer[0][i] = 0;
	}
}
#endif
