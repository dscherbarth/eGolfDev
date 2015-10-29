//
// battery monitor
//  read bit stream data from balancers
//  display per cell and per battery voltages and status
//  generate visible alarms and remote charger control and battery fault (shorted or under voltage) outputs

// Init lcd display pins
#define sclk 13
#define mosi 11
#define cs   10
#define dc   9
#define rst  8  

#define MARKTMO  2000000  // 2s wait up to 2seconds for a break to make sure we start at a start bit
#define MARKEMPTY 1200    // 1.2ms represents 6 bit times (it needs to be marking for this long to make sure we are not in the middle of a message)
#define SUBWAIT  5000  // how long to wait looking for all cells (11) in a sub-battery (in ms)

#define NORMAL 0
#define HIGHVOLT 1
#define LOWVOLT 3
#define HIGHTEMP 4
#define BALANCE 2

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, mosi, sclk, rst);//

// setup colors
int normal_color, balance_color, highvolt_color, lowvolt_color, shorted_color, hightemp_color;

void setup() 
{
  // initialize serial communication at 9600 bits per second for debug:
  Serial.begin(9600);

  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab

  // setup the balancer serial inputs
  pinMode (22, INPUT);
  pinMode (24, INPUT);
  pinMode (25, INPUT);
  pinMode (26, INPUT);
  pinMode (30, INPUT);
  pinMode (32, INPUT);
  pinMode (34, INPUT);
  pinMode (36, INPUT);

  // charge/discharge enable outputs
  pinMode (2, OUTPUT);
  pinMode (3, OUTPUT);
  
  pinMode (12, OUTPUT);    // for debugging the bit read location

 // setup the colors
  normal_color   = ST7735_GREEN;
  balance_color  = ST7735_YELLOW;
  highvolt_color = ST7735_RED;
  lowvolt_color  = ST7735_BLUE;
  shorted_color  = ST7735_CYAN;
  hightemp_color = ST7735_MAGENTA;
}

// 
// battery data structure definitions
//
struct cell_data
{
  int state;
  int state_count;
  int valid_state;    // set to the persistant state (must be true 3 uninterrupted status reads)
  int volts;
};

struct sub_batt
{
  struct cell_data c_d[11];
  int num_cells;            // number of cells detected
  int bvolts;               // summed battery voltage * 100 (2 dec places)
};

struct battery
{
  struct sub_batt batt[8];
  int batt_volts;          // summed over all cells 
  int num_cells_reporting; // over all sub-batteries (sure hope this is 88 :) )
  int charge_enable;       // if we are not shorted or over-volted
  int discharge_enable;       // if we are not shorted or under-volted
};

struct battery full_batt;



#define RCVRCLOCKTIME 205 // clocktime in uS (this is 5KHz)
int rcvrclk = 205;

//
// read a 4 bit data word, we are in the middle of the first data bit
//
char read_4_bits (int channel)
{
char temp = 0;
int i;
  
  
//  digitalWrite (12, 1);    // debug
//  digitalWrite (12, 0);
  for(i=0; i<4; i++)
  {
    temp |= ((rcvrstate (channel) & 1) << i);
    delayMicroseconds (rcvrclk);
  }
  // skip the next start bit
  delayMicroseconds (rcvrclk);
  return (temp);
}

//
// wait for the serial line to see the start of a message and then wait till the center of the first data bit
//
void wait_for_start (int channel)
{
  unsigned long start_time;


  // wait for the start bit
  start_time = micros();
  while (1)
  {
    if ((micros() - start_time) > MARKTMO || rcvrstate(channel) != 0)
      break;
  }
  
  // wait 1.5 clocks more so we are in the middle of a data bit
  delayMicroseconds (rcvrclk + rcvrclk/2);
}

//
// abstraction of the serial input channel
//
int rcvrstate (int channel)
{
  digitalWrite (12, 1);    // debug (might need to leave this in for correct timing..)
  digitalWrite (12, 0);
  return digitalRead ((channel*2) + 22);
}

//
// since there is no data clocking mechanism, we need to wait for the start of a message.
// we do this by waiting for a 6 clock long blank space (marking)
//
int wait_for_marking (int channel)
{
  unsigned long time, start_time;
  int rv = 1; 
  

  // we need a quiet period of 6 clocks to make sure we are at the start of a message
  // 6 clocks at 5KHz is 1200uS
  start_time = micros();
  while (1)
  {
    while ((micros() - start_time) < MARKTMO && rcvrstate(channel) != 0); // wait for zero
    if ((micros() - start_time) >= MARKTMO)
    {
      rv = 0;  
      break;
    }
    
    time = micros();
    while (1)
    {
      if (rcvrstate(channel) != 0)
        break;
      if ((micros() - time) > MARKEMPTY)
        break;
    }
    
  // see if the bit is still clear
  if (rcvrstate(channel) == 0)
    break;
    
  }
  return (rv);
}

//
// wait for data from all 11 cells in a given sub-battery (serial channel)
//
int get_battery_data (int sub_battery)
{
  unsigned long start_time;
  int i;
  int rv = 0;
  
  
  // mark as not read yet
  for (i=0; i<11; i++)
  {
    full_batt.batt[sub_battery].c_d[i].state = -1;
  }
  
  // read cell data on this sub-battery for up to 5 seconds
  start_time = millis();
  
  // try to collect data on all of the cells
  while ((millis() - start_time) < SUBWAIT)
  {
    get_one_cell (sub_battery);
    if (how_many_seen(sub_battery) == 11)
    {
      rv = 1;
      break;
    }
  }
  
  // compile current battery status
  update_battery_status (sub_battery);
  return rv;
}

void update_battery_status (int sub_battery)
{
  int i;
  
  for(i=0; i<11; i++)
  {
    // set the valid state
    switch (full_batt.batt[sub_battery].c_d[i].state)
    {
      case HIGHVOLT:
      case LOWVOLT:
      case HIGHTEMP:
      case -1:      // missing..
        full_batt.batt[sub_battery].c_d[i].state_count++;
        if (full_batt.batt[sub_battery].c_d[i].state_count > 3)
        {
           full_batt.batt[sub_battery].c_d[i].valid_state = full_batt.batt[sub_battery].c_d[i].state;
        }
        break;
        
      default:
        // reset the state and counter
        full_batt.batt[sub_battery].c_d[i].valid_state = 0;
        full_batt.batt[sub_battery].c_d[i].state_count = 0;
        break;
    }
  }  
}

int how_many_seen (int sub_battery)
{
  int rv = 0;
  int i;
  
  for (i=0; i<11; i++)
  {
    if (full_batt.batt[sub_battery].c_d[i].state != -1)
    {
      rv ++;
    }
  }
  return (rv);
}
int get_one_cell (int battery)
{
  int id, state, voltsl, voltsh, volts, checksum, calc_check;
  int rv = 0;   // return value

  
  // wait for off for at least 6 clocks
  wait_for_marking(battery);
  
  // wait for start bit
  wait_for_start(battery);
  
  // read id, state, volts, checksum (4 bits each)
  id    = read_4_bits (battery);
  state = read_4_bits (battery);
  voltsl = read_4_bits (battery);
  voltsh = read_4_bits (battery);
  checksum = read_4_bits(battery);

  volts = voltsl + (voltsh << 4);
  if ((id >= 1 && id <= 11))
  {
    calc_check = (id ^ state ^ voltsl ^ voltsh);
//    if (calc_check == checksum)
//    {
      full_batt.batt[battery].c_d[id-1].state = state;
      full_batt.batt[battery].c_d[id-1].volts = volts;
//    }
//  Serial.print(id); Serial.print(" ");
//  Serial.print(state); Serial.print(" ");
//  Serial.print(volts); Serial.print(" ");
//  Serial.print(volts + 183); Serial.print(" ");
//  Serial.print(calc_check); Serial.print(" "); Serial.println(checksum);
    
    rv = 1;
  }
  
  return (rv);
}

void draw_template (void)
{
  int i;
  int temp_color;
  
  
  temp_color = tft.Color565(0xad, 0xc6, 0xFF);

  tft.setTextSize(0);
  tft.setCursor(5, 5);
  tft.setTextColor(temp_color);
  tft.print("eGolf Battery Status");

  // draw borders
  for (i=0; i<8; i++)
  {
    draw_batt(i);
  }
}
void draw_batt (int b)
{
  int i;
  // box
  tft.drawRect (b*16+1, 85, 15,75, ST7735_WHITE);
  
}

void update_full_battery (void)
{
  int i;

  for (i=0; i<8; i++)
    {
      update_batt(i);
    }
}

void update_batt(int b)
{
  int i;
  static int toggle = 0;
  int color;
  int volt;
  int bpos;
  
  // get cell status for bar length and color
  //  green ok, yellow balance, red overvolt, blue undervolt, white missing, purple overtemp

  // cells
  for(i=0; i<11; i++)
  {
    switch(full_batt.batt[b].c_d[i].state)
    {
      case -1:
        color = shorted_color;
        break;
      case NORMAL:
        color = normal_color;
        break;
      case HIGHVOLT:
        color = highvolt_color;
        break;
      case LOWVOLT:
        color = lowvolt_color;
        break;
      case HIGHTEMP:
        color = hightemp_color;
        break;
      case BALANCE:
        color = balance_color;
        break;
    }

    // draw the state and volt bar    
    volt = full_batt.batt[b].c_d[i].volts / 22; // bar should represent 3.0 to 4.2 volts
    bpos = i*6 + i/4;
    tft.drawLine(b*16+3, 90 + bpos, b*16+3 + volt, 90 + bpos, color);
    tft.drawLine(b*16+3, 91 + bpos, b*16+3 + volt, 91 + bpos, color);
  }
}

// the loop routine runs over and over again forever:
void loop() {
  unsigned int i, j;
  int missing, overvolt, overtemp, undervolt, balancing;
  
  
// init our state
// read all cells/all batteries
// compile per battery state and voltages
// produce alarms and status do's
// update lcd display
// take action on:
//  Missing cells (shorted?)
//  Low voltage alarm(s)
//  high voltage alarm(s)

   Serial.println("<== Loop start");
   tft.fillScreen(ST7735_BLACK);

  // init the battery structure
  for(j=0; j<8; j++)
  {
    full_batt.batt[j].num_cells = 0;            // number of cells detected
    full_batt.batt[j].bvolts = 0;               // summed battery voltage * 100 (2 dec places)
    for (i=0; i<11; i++)
    {
      full_batt.batt[j].c_d[i].state = -1;
      full_batt.batt[j].c_d[i].state_count = 0;
      full_batt.batt[j].c_d[i].valid_state = 0;
    }
  }

  // allow the charger to run
  // turn off the battery fault indication
  full_batt.charge_enable = 1;       // if we are not shorted or over-volted
  full_batt.discharge_enable = 1;       // if we are not shorted or under-volted

  // draw battery outline
  draw_template();
  
  // update the state forever
  while(1)
  {
    for(i=0; i<8; i++)
    {
      get_battery_data (i);
      update_batt(i);
    }

    // allow the charger to run as default
    full_batt.charge_enable = 1;       // default is not shorted or over volted or over temp'ed
    full_batt.discharge_enable = 1;    // default is not shorted or under-volted

    // locate persistent alarm conditions
    missing = overvolt = overtemp = undervolt = balancing = 0;    
    for(i=0; i<7; i++)
    {
      for(j=0; j<11; j++)
      {
        switch (full_batt.batt[i].c_d[j].valid_state)
        {
        case -1:
          missing = 1;
          break;

        case BALANCE:
          balancing = 1;
          break;

        case HIGHVOLT:
          overvolt = 1;
          break;

        case LOWVOLT:
          undervolt = 1;
          break;

        case HIGHTEMP:
          overtemp = 1;
          break;
        }
      }
    }
    
    // do we need to stop charging?
    if (missing || overvolt || overtemp)
    {
      full_batt.charge_enable = 0;
    }
    
    // do we need to stop discharging?
    if (missing || undervolt)
    {
      full_batt.discharge_enable = 0;
    }

    // drive relays to appropriate state and update the alarm text
    // should we beep?
    if (balancing)    // if we are balancing might need to disable charging
    {
      if (full_batt.charge_enable)
      {
        digitalWrite (2, 0);
        tft.fillRect(5,20,120,20,ST7735_BLACK);
      }
      else
      {
        digitalWrite (3, 0);
        tft.fillRect(5,40,120,20,ST7735_BLACK);

        digitalWrite (2, 1);
        tft.setTextSize(0);
        tft.setCursor(5, 20);
        tft.setTextColor(ST7735_RED);
        tft.print("Charge Disabled");
      }
    }
    else    // not balancing, might need to disable discharge/signal fault
    {
      if (full_batt.discharge_enable)
      {
        digitalWrite (3, 0);
        tft.fillRect(5,40,120,20,ST7735_BLACK);
      }
      else
      {
        digitalWrite (2, 0);
        tft.fillRect(5,20,120,20,ST7735_BLACK);

        digitalWrite (3, 1);
        tft.setTextSize(0);
        tft.setCursor(5, 40);
        tft.setTextColor(ST7735_RED);
        tft.print("DisCharge Disabled");
      }
    }
  }
  
}


