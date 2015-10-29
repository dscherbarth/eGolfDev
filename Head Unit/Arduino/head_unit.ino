// eGolf head unit
// by Doug Scherbarth and Dave Barrett

// Created Oct 24 2013

// this head unit has a 4/20 lcd display, a sd card datalogger
// a qe spinner and 2 buttons.  It connects to the nxp controller using a 2515 canbus interface on SPI 

// initial menu allows selection of: 
//  Live data/status display
//  logger select/start/stop
//  fault detail and reset
//  PID tuning param adjustment

// io allocation:
//  11 - 13       spi for data logger and can
//  4  - 8 and 1  4x20 lcd
//  3 and ai 2    spinner
//  ai 0 and 1    button input
//  9             cs for sd card
//  10            cs for can
//  2             int for can

// what to display/log: Stator RPM, Rotor RPM, Bus Volts, Bus Amps, IGBT temp, 
//  Throttle pos, Fault condition/reason, Batt % 


// todo:
//

#include "LiquidCrystal.h"
#include "CANInterface.h"
#include <SPI.h>
#include <SD.h>

// dac reflector sources
static char dacr1[] PROGMEM = "PhI1";
static char dacr2[] PROGMEM = "PhI2";
static char dacr3[] PROGMEM = "PhI3";
static char dacr4[] PROGMEM = "PWM1";
static char dacr5[] PROGMEM = "PWM2";
static char dacr6[] PROGMEM = "PWM3";
static char dacr7[] PROGMEM = "RPos";
static char dacr8[] PROGMEM = "PidD";
static char dacr9[] PROGMEM = "PidQ";

PROGMEM const char *dacrArray[] = 
{
   dacr1,
   dacr2,
   dacr3,
   dacr4,
   dacr5,
   dacr6,
   dacr7,
   dacr8,
   dacr9
};
#define MAXDACR  8

static char blank[] = "                ";
static char msg1[] PROGMEM =            "Waveform Off";
static char msg2[] PROGMEM = 		"Check Press Sw";
static char msg3[] PROGMEM = 		"Zeroize Voltage";
static char msg4[] PROGMEM = 		"Precharge";
static char msg5[] PROGMEM = 		"HV on";
static char msg6[] PROGMEM = 		"Zeroize amps";
static char msg7[] PROGMEM = 		"Oil Cooling on";
static char msg8[] PROGMEM = 		"Inverter on";
static char msg9[] PROGMEM = 		"Zeroize throttle";
static char msg10[] PROGMEM = 		"Waveform ON";
static char msg11[] PROGMEM = 		"No OilP Fault";
static char msg12[] PROGMEM = 		"Oil Sw Fault";
static char msg13[] PROGMEM = 		"Flux Cap Charged";
static char msg14[] PROGMEM = 		"1.21 Gw Avail";
static char msg15[] PROGMEM = 		"Ramp RPM Down";
static char msg16[] PROGMEM = 		"Inverter off";
static char msg17[] PROGMEM = 		"Cooling oil off";
static char msg18[] PROGMEM = 		"HV off";
static char msg19[] PROGMEM = 		"Precharge off";

PROGMEM const char *messageArray[] = 
{
   msg1,
   msg2,
   msg3,
   msg4,
   msg5,
   msg6,
   msg7,
   msg8,
   msg9,
   msg10,
   msg11,
   msg12,
   msg13,
   msg14,
   msg15,
   msg16,
   msg17,
   msg18,
   msg19
};   

int msgindex = -1;

#define MSGID		20

#define PIDD            30
#define PIDQ            31
#define PIDS            32

#define MSGBLANK	19

#define STARTING 1
#define RUNNING  2
#define STOPPING 3
#define STOPPED  4
#define FAULTED  5

#define HEADCMDSTART	1
#define HEADCMDSTOP	2
#define HEADCMDCLEAR	3

#define NUMBTNSEL      11

static int startState = 0;    // 0 is stopped, 1 is running, 2 is faulted
static int ctrlCmd = 123;
static int numDataRcved = 0;
static int dataMsg = 0;
static int dacRIndex = 0;

int SDCS = 9;
int CANCS = 10;

static int SDOK = 0;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(4,5,6,7,8,1);

volatile long encoder0Pos = 0;

char tbuf[20];

int  pidDp = 300, pidDi = 100, pidQp = 250, pidQi = 150, pidSp = 200, pidSi = 99;
int  tpidDp = 300, tpidDi = 100, tpidQp = 250, tpidQi = 150, tpidSp = 200, tpidSi = 120;

void BaseMenu()
{
  char temp[30];
  lcd.setCursor(0, 0);

  lcd.setCursor(0, 1);
  lcd.print("ROTR STAT ACC     ST");
  lcd.setCursor(0, 3);
}

void setup()
{ 
  int canret;
  
  
  // lcd backlight
  pinMode (0, OUTPUT);
  pinMode (1, OUTPUT);
  backlightWhite();
  
  // LCD Display setup
  // set up the LCD's number of rows and columns: 
  lcd.begin(20, 4);
  delay(100);
  lcd.clear();
  delay(200);

  // can setup
  init_SPI_CS();
  SPI.begin();
  if(1 == (canret = CAN.Init(CANSPEED_125)))/*SPI must be configured first*/
  {
    lcd.setCursor(0, 3);
    sprintf(tbuf, "CRun %d", canret);
    lcd.print(tbuf);
    delay(200);
  }
  else
  {
    lcd.setCursor(0, 3);
    sprintf(tbuf, "CFail %d", canret);
    lcd.print(tbuf);
    delay(500);
  }

  // SD setup
  pinMode(9, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(9)) 
  {
    lcd.setCursor(0, 3);
    lcd.print("SD init failed");
    SDOK = 0;
  }
  else
  {
    lcd.setCursor(0, 3);
    lcd.print("SD initialized");
    SDOK = 1;
  }
  delay(200);
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  
  // QE spinner, button
  pinMode (2, INPUT);
  pinMode (3, INPUT);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  
  attachInterrupt(1, doSpin, CHANGE); // int on pin 3
  attachInterrupt(0, canUpd, CHANGE); // int on pin 2
  
  // display the menu
  BaseMenu();
}

extern tCAN message1;
extern int tp0, tp1, tp2;
extern int h0, h1;
extern int snapAvail;

// main loop
void loop()
{
  static int refresh = 0;
  // check for action on the buttons
  checkButtonCmd();
  
  // update the spinner
  checkSpinner();
  
  dispBtnState ();
  
  // display data as received from controller
  dispRemoteData();
  
  numDataRcved--;
  
//  if (snapAvail)
//  {
//    SnapData(h0, h1, 765);
//    SnapData(tp0, tp1, tp2);
//    SnapStop();
//    snapAvail = 0;
//  }
  delay (100);
  
}

