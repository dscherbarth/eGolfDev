
/*!
LTC6804-1: Battery stack monitor


REVISION HISTORY
 */



#include <Arduino.h>
#include <stdint.h>
#include "Linduino.h"
#include "LT_SPI.h"

#include "UserInterface.h"
#include "LTC68041.h"
#include <SPI.h>
#include "mcp_can.h"
#include <Servo.h>

#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, 13, NEO_GRB + NEO_KHZ800);

MCP_CAN CAN(A5);

Servo chargeI;

const uint8_t TOTAL_IC = 8;//!<number of ICs in the daisy chain
//const uint8_t TOTAL_IC = 4;//!<number of ICs in the daisy chain
uint16_t cell_codes[TOTAL_IC][12]; 
uint16_t aux_codes[TOTAL_IC][6];
uint32_t last_data = 0; // updated as reads complete
uint8_t data_valid[TOTAL_IC][2];    // incremented when read is good (cells, aux) reset to 0 on a bad read, no rollover

#define UNDERVOLT 0x01
#define OVERVOLT  0x02
#define UNDERTEMP 0x04
#define OVERTEMP  0x08
#define BALANCING 0x10

uint8_t sub_batt_status[TOTAL_IC];  // undervolt, overvolt, overtemp, balancing, normal

/*!<
 The GPIO codes will be stored in the aux_codes[][6] array in the following format:
 
 |  aux_codes[0][0]| aux_codes[0][1] |  aux_codes[0][2]|  aux_codes[0][3]|  aux_codes[0][4]|  aux_codes[0][5]| aux_codes[1][0] |aux_codes[1][1]|  .....    |
 |-----------------|-----------------|-----------------|-----------------|-----------------|-----------------|-----------------|---------------|-----------|
 |IC1 GPIO1        |IC1 GPIO2        |IC1 GPIO3        |IC1 GPIO4        |IC1 GPIO5        |IC1 Vref2        |IC2 GPIO1        |IC2 GPIO2      |  .....    |
*/

uint8_t tx_cfg[TOTAL_IC][6];
uint8_t rx_cfg[TOTAL_IC][8];

#define STATEIDLE         1
#define STATECHARGING     2
#define STATECHINIT       21  // initialize the charge
#define STATECHINPROG     22  // constant current charge
#define STATECHINPCHG     221 // charging
#define STATECHINPIDN     222 // charging, drop current
#define STATECHINPPORD    223 // charging, power off read data
#define STATECHCONSTV     23  // constant voltage charge
#define STATECHPOSTCHARGE 24  // post charge equalize
#define STATEMONITORING   3
#define STATEEQUALIZE     4
#define STATEEQINIT       41
#define STATEEQINPROG     42

int currentState = STATEIDLE;
int currentSubState = 0;
int currentsubsubState = 0;
/*!**********************************************************************
 \brief  Inititializes hardware and variables
 
 A0 is vdiv for charger/car powered
 A1 is charger range set
 A2 is charger current hi/lo
 A3 is charger on/off control
 A4 is iso-spi comm chip select
 A5 is CANBUS chip select
 D1 is charger current control digital pot chip select (GPIOTx)
 D0 is CANBUS interrupt (GPIORx)
 D13 is NeoPix control
 D12 - D5 + SDA and SDL are 8 DO's for subcell fan control
 
 ***********************************************************************/
void setup()                  
{
  // setup peripheral io
  pinMode(1, OUTPUT);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, OUTPUT);     // charger requested to start (j1772 plug inserted)
  
  digitalWrite(A3, LOW);	// turn charger off

  // init the 6804 interface
  LTC6804_initialize(); //Initialize LTC6804 hardware
  init_cfg();           //initialize the 6804 configuration array to be written
  pinMode(A4, OUTPUT);   // chip select for ltc6820 spi interface 

  pinMode(13, OUTPUT);   // neopix

  pinMode(12, OUTPUT);  digitalWrite(12, HIGH); // 7
  pinMode(11, OUTPUT);  digitalWrite(11, HIGH); // 8
  pinMode(10, OUTPUT);  digitalWrite(10, HIGH); // 1
  pinMode(9, OUTPUT);   digitalWrite(9, HIGH);  // 2 
  pinMode(6, OUTPUT);   digitalWrite(6, HIGH);  // 4
  pinMode(5, OUTPUT);   digitalWrite(5, HIGH);  // 3
  pinMode(21, OUTPUT);  digitalWrite(21, HIGH); // 6
  pinMode(20, OUTPUT);  digitalWrite(20, HIGH); // 5

  // monitor or charge requested on startup
  if (digitalRead(A0))
  {
      currentState = STATEMONITORING;
  }
  else
  {
//currentState = STATEIDLE;
    currentState = STATECHARGING;
    currentSubState = STATECHINIT;
  }
//currentState = STATEEQUALIZE;
//currentSubState = STATEEQINIT;
//currentState = STATEMONITORING;
  Serial.begin(115200);

  // servo setup
  chargerCurrentSet(0);

  // start the can bus interface
  CAN.begin(CAN_125KBPS);

}

float chargeRate  =0.0;
int x = 0;
int y = 0;

int hvbloc, hvcloc;
int lvbloc, lvcloc;
int htbloc, htcloc;

int lowestV()
{
    int lowvolts = 42000;
    for (int i=0; i<TOTAL_IC; i++)
    {
      for (int j=0; j<11; j++)
      {
//        if (data_valid[i][0] > 3 && (cell_codes[i][j] < lowvolts))
        if ((cell_codes[i][j] < lowvolts))
        {
          lowvolts = cell_codes[i][j];
          lvbloc = i; lvcloc = j;
        }
      }
    }
    return lowvolts;
}

int highestV()
{
    int highvolts = 0;
    for (int i=0; i<TOTAL_IC; i++)
    {
      for (int j=0; j<11; j++)
      {
        if (data_valid[i][0] > 3 && (cell_codes[i][j] > highvolts))
        {
          highvolts = cell_codes[i][j];
          hvbloc = i; hvcloc = j;
        }
      }
    }
    return highvolts;
}

int highestT()
{
    unsigned int hightemp = 15000;
    for (int i=0; i<TOTAL_IC; i++)
    {
      for (int j=0; j<5; j++)
      {
        if (aux_codes[i][j] < hightemp)
        {
          hightemp = aux_codes[i][j];
          htbloc = i; htcloc = j;
        }
      }
    }
    return hightemp;
}

void getdetailed (int subcell, unsigned char *buf)
{
	int i;
	int ht;
	
	buf[0] = (unsigned char)subcell;
	
  if(data_valid[subcell][0] > 3)
  {
    // reform and save voltages
    for(i=0; i<11; i++)
    {
      buf[1+i] = (unsigned char)((cell_codes[subcell][i] / 100) - 280); // significant info
    }
    
    // reform and save temps
    ht = 0;
    for(i=0; i<5; i++)
    {
      if ((unsigned char)(((15000 - aux_codes[subcell][i]) / 200) + 25) > ht)
      {
        ht = (unsigned char)(((15000 - aux_codes[subcell][i]) / 200) + 25);
      }
    }
    for(i=0; i<4; i++)
    {
      buf[12+i] = (unsigned char)ht;
    }
  } 
  else
  {
    for(i=0; i<15; i++)
    {
      buf[i+1] = 0;
    }
  }
}

void getcond (char *buf)
{
  int tv = totalVolts();
  int lv = lowestV();
  int hv = highestV();
  int ht = highestT();
  
  buf[0] = 123;    // marker
  
  buf[1] = (char)(0xff & (tv >> 8)); buf[2] = (char)(0xff & tv ); // total volts

  buf[3] = (char)(0xff & (lv >> 8)); buf[4] = (char)(0xff & lv ); // lowest
  buf[5] = (char)lvbloc; buf[6] = (char)lvcloc; 

  buf[7] = (char)(0xff & (hv >> 8)); buf[8] = (char)(0xff & hv ); // highest
  buf[9] = (char)hvbloc; buf[10] = (char)hvcloc; 

  buf[11] = (char)(0xff & (ht >> 8)); buf[12] = (char)(0xff & ht ); // highest T
  buf[13] = (char)htbloc; buf[14] = (char)htcloc; 

  buf[15] = (char)currentState;
}

int chargeEnabled = 0;  // digital output
int balFanOn = 0;       // digital output
int tempFanOn = 0;      // if fan needs to be on for high temp reasons

// crcAndSend 
//    calculate a crc over the supplied data and format into lv and send over the ble uart connection
//    we will only get here if we are already connected
//
void crcAndSend (char * data, int data_length)
{
}

// fetch all cell data via isoSPI and set valid/timestamp
void fetchCells()
{
  uint16_t perICPecError[TOTAL_IC];

  // fetch the data
  wakeup_sleep();
  LTC6804_adcv();
  delay(3);
  for(int i=0; i<TOTAL_IC; i++)
  {
    perICPecError[i] = 0;
  }
  
  LTC6804_rdcv(0, TOTAL_IC,cell_codes, &perICPecError[0]); // Set to read back all cell voltage registers

  cell_codes[5][4] -= 400;
  cell_codes[5][8] -= 200;
  // update the valid data counters
  for(int i=0; i<TOTAL_IC; i++)
  {
//Serial.print(i);Serial.print(" PICEC ");Serial.print(perICPecError[i]);
    if(!perICPecError[i])
    {
      for(int j=0; j<11; j++)
      {
//Serial.print(" "); Serial.print(cell_codes[i][j]/10);
      }
//Serial.println();
    }
//Serial.print(" PE "); Serial.print(i); Serial.print(perICPecError[i]);

    if (perICPecError[i])
    {
      data_valid[i][0] = 0;
    }
    else if (data_valid[i][0] < 5)
    {
      data_valid[i][0] += 1;
    }
  }
}

// fetch all aux data via isoSPI and set valid/timestamp
void fetchAux()
{
  uint16_t perICPecError[TOTAL_IC];

  // fetch the data
  wakeup_sleep();
  LTC6804_adax();
  delay(3);
  wakeup_sleep();
  LTC6804_rdaux(0, TOTAL_IC, aux_codes, &perICPecError[0]); // Set to read back all cell voltage registers

  // update the valid data counters
  for(int i=0; i<TOTAL_IC; i++)
  {
    if(!perICPecError[i])
    {
      for(int j=0; j<5; j++)
      {
//Serial.print(" "); Serial.print((((15000 - aux_codes[i][j]) / 200) + 25));
      }
//Serial.println();
    }
    if (perICPecError[i])
    {
      data_valid[i][1] = 0;
    }
    else if (data_valid[i][1] < 5)
    {
      data_valid[i][1] += 1;
    }
  }

  // update the last collected counter
  if (data_valid[0][1]) last_data++;
}

#define BALTHRESH 42000   // 4.20 volts
#define BALDELTA  100     // .01 volts
int balcnt = 0;
int numBal()
{
  return balcnt;
}

//
// setBalance
//   apply balancing if voltage is over balvolts.
//
boolean setBalance(boolean take_action, int balvolts)
{
  boolean balFlag = false;
  uint8_t icBal[TOTAL_IC][6];
  int hightemp;

  
  balcnt = 0;
  balFanOn = 0;
  
  // check all the valid data in cells for > BALLIMIT
//Serial.print(balvolts);
  for(int i=0; i<TOTAL_IC; i++)
  {
    sub_batt_status[i] &= ~BALANCING;

    icBal[i][0] = 0xf9; icBal[i][1] = 1; icBal[i][2] = 0x3d; icBal[i][3] = 0x6e;
    icBal[i][4] = icBal[i][5] = 0;
    for(int j=0; j<11; j++)
    {
      hightemp = 0; 
      for(int k=0; k<5; k++)
      {
        if ((((15000 - aux_codes[i][k]) / 200) + 25) > hightemp)
        {
          hightemp = (((15000 - aux_codes[i][k]) / 200) + 25);
        }
      }
      if (hightemp < 65 && data_valid[i][0] && cell_codes[i][j] > (balvolts + BALDELTA))
      {
        // this one needs balancing
        if (j < 8) icBal[i][4] |= (1 << j); else icBal[i][5] |= (1 << (j-8));

        // some balancing is happening
        balcnt++;
        balFlag = true;
        balFanOn |= (1 << i);

        // set the sub-batt status
        sub_batt_status[i] |= BALANCING;
      }
    }
//Serial.print(" "); Serial.print(balcnt); Serial.println();
  }

  if (take_action)
  {
    // write the balance config
    wakeup_sleep();
    LTC6804_wrcfg(TOTAL_IC,icBal);

    // if balancing turn on the fan
    if (!balFlag) { balFanOn = 0; }
  }
  else
  {
    icBal[0][4] = 0; icBal[0][5] = 0;
    LTC6804_wrcfg(TOTAL_IC,icBal);
    balFanOn = 0;
  }

  return balFlag;
}

#define HVLIMIT  42100    // 4.21 volts

// check valid values for an over-voltage condition (greater than HVLIMIT)
int overVoltage()
{
  int ovs = 0;

  for(int i=0; i<TOTAL_IC; i++)
  {
    // reset status
    sub_batt_status[i] &= ~OVERVOLT;
    
    for(int j=0; j<11; j++)
    {
      if ((data_valid[i][0] > 3) && cell_codes[i][j] > HVLIMIT)
      {
        ovs = 1;

        // update the status
        sub_batt_status[i] |= OVERVOLT;
      }
    }
  }
  return ovs;
}

#define LVLIMIT  28000    // 2.8 volts
#define HTLIMIT  50       // 50 deg C

//
// overTemp
//  convert temperature values and test against high limits
//
boolean overTemp( void )
{
  boolean ots = false;  

  tempFanOn = 0;
  for(int i=0; i<TOTAL_IC; i++)
  {
    // reset status
    sub_batt_status[i] &= ~OVERTEMP;
    
    for(int j=0; j<5; j++)
    {
      if ((data_valid[i][0] > 3)&& (((15000 - aux_codes[i][j]) / 200) + 25) > HTLIMIT)
      {
        ots = true;

        // update the status
        sub_batt_status[i] |= OVERTEMP;
        tempFanOn |= (1 << i);
      }
    }
  }
  return ots;
}

// check valid values for an under-voltage condition (less than LVLIMIT)
int underVoltage()
{
  int uvs = 0;

  for(int i=0; i<TOTAL_IC; i++)
  {
    // reset status
    sub_batt_status[i] &= ~UNDERVOLT;
    
    for(int j=0; j<11; j++)
    {
      if ((data_valid[i][0] > 3)&& cell_codes[i][j] < LVLIMIT)
      {
        uvs = 1;

        // update the status
        sub_batt_status[i] |= UNDERVOLT;
      }
    }
  }
  return uvs;
}

#define BALTHRESH 42000   // 4.20 volts
int chargeMax = 355;
int chargeAmps = 90;

// totalVolts
//   sum all valid cells voltage
uint16_t totalVolts (void)
{
  uint16_t tv = 0;

  for(int i=0; i<TOTAL_IC; i++)
  {
    for(int j=0; j<11; j++)
    {
      if (data_valid[i][0] > 3)
      {
        tv += (cell_codes[i][j]/100);
      }
    }
  }
  
  return tv;
}

#define POSTCHARGEWAIT  3600    // balance for up to 1 hour
#define CHAREGERCYCLE   120      // total charger cycle time
#define CHGON   5              // time to collect data with charger off

int postChargeBalance = 0;
int eqvolts = 41500;    // default start point
int eqcounter = 0;

int lastTV = 0;
// handleCurrentState
//  manage the state machine
//
void handleCurrentState()
{
  int tc = 0;
  static int chtimer = 0;
  static int newdata = 0;    
  switch (currentState)
  {
    case   STATEIDLE :
      break;

    case STATEEQUALIZE :
Serial.println("Equalizing");
      // get some recent data
      fetchCells();
      fetchAux();
      overTemp();

      switch (currentSubState)
      {
        case STATEEQINIT :
          // determine the lowest voltage
          eqvolts = lowestV();
Serial.print(eqvolts); Serial.print(" ");
Serial.println("eq volts");

          // make sure that we don't equalize too low
          if(eqvolts < 30000) eqvolts = 30000;
          
          // set to next step
          currentSubState = STATEEQINPROG;
          chargeEnabled = 0;          
          break;

        case STATEEQINPROG :
          // see if we are done balancing
          int notdone = 0; tc = 0;
          if (eqcounter++ > 10)
          {
            for (int i=0; i<TOTAL_IC; i++)
            {
              for (int j=0; j<11; j++)
              {
                if (data_valid[i][0] > 3 && (cell_codes[i][j] > eqvolts))
                {
                  tc++;
                  notdone = 1;
                }
              }
            }
            if (notdone)
            {
              // enable balance for all appropriate cells
              setBalance(true, eqvolts);
            }
            else
            {
              // finished equalizing
              currentState = STATEIDLE;
              setBalance(false, eqvolts);
Serial.print(" ");
Serial.println("done!!!");
              balFanOn = 0;
            }
          }
          break;
      }

//
//  22.5 = 0
//  45   = 1.3A
//  70   = 2.5A
//  80   = 4A
//  90   = 5A
//  140  = 8A
//  170  = 10A
//  180  = 12.5

    case STATECHARGING :
      switch (currentSubState)
      {
        case STATECHINIT :
          // setup
          // enable charger
          chargerCurrentSet( chargeAmps );
          chargeEnabled = 1;
          chtimer = CHAREGERCYCLE;

          // move to in prog
          currentSubState = STATECHINPROG;
          currentsubsubState = STATECHINPCHG;
          
          // charge rate set to zero
          chargeRate = 0.0;
          break;
          
        case STATECHINPROG :
        case STATECHCONSTV :
          // while not charged and not overvolt and not overtemp
          switch (currentsubsubState) {
            case STATECHINPCHG :
              if(chtimer-- <= 0)
              {
                currentsubsubState = STATECHINPIDN;
              }
              break;
              
            case STATECHINPIDN :
              chtimer = CHGON;
              chargeEnabled = 0;
              handlePeripherals();
              currentsubsubState = STATECHINPPORD;
              break;

            case STATECHINPPORD :
              if(chtimer-- <= 0)
              {
                currentsubsubState = STATECHINPCHG;
                chargeEnabled = 1;
                chtimer = CHAREGERCYCLE;
                newdata++;
              }
              else
              {
                fetchCells();
                fetchAux();
                overTemp();
              }
              break;
            }
            
          if(currentSubState == STATECHCONSTV)
          {
            // make sure no cell is overcharged
            // if the pack volts are too high, lower charge current
            // if the pack volts are too low increase the charge current
            // if the charge current is below threshold we are done   
            if (overVoltage() || chargerCurrent() < 25)
            {
              // disable charger
              chargeEnabled = 0;          
              chargerCurrentSet( 0 );
  

              // move to postcharge
              currentSubState = STATECHPOSTCHARGE;
  
              // start timer
              postChargeBalance = POSTCHARGEWAIT;
            }
            if(newdata)
            {
              if (totalVolts() > chargeMax)
              {
                chargerCurrentAdjust( -10);
              }
              else if (totalVolts() < chargeMax)
              {
                chargerCurrentAdjust( 5);
              }
              newdata = 0;
            }
          }
          
          if(currentSubState == STATECHINPROG)
          {
            // adjust charger current and set balance
            if (totalVolts() >= chargeMax || overVoltage())
            {
              // transition to constant voltage charge
              // reduce charge current and move to const voltage mode
//                currentSubState = STATECHPOSTCHARGE;
              currentSubState = STATECHCONSTV;
              chargerCurrentAdjust( -10);
            }
            else
            {
              #define CHEQTHRESH 500   // .05 Volts
              int lowV = lowestV() + CHEQTHRESH;
              setBalance(true, lowV < BALTHRESH? lowV : BALTHRESH);
              handlePeripherals();
            }
          }
          break;
            
        case STATECHPOSTCHARGE :
          // pack charge reached (total volts > 360 or cell(s) over-voltage)
          chargeEnabled = 0;          
          chargerCurrentSet( 0 );
          handlePeripherals();

          // fetch data and
          fetchCells();
          fetchAux();

          // if no more balance needed or timer runs out we are done with post charge
          if (!setBalance(true, BALTHRESH) || postChargeBalance-- <= 0)
          {
            // done with post charge
            currentSubState = 0;
            chargeEnabled = 0;          
            chargerCurrentSet( 0 );
            currentState = STATEIDLE;
          }

          // update status
          overVoltage();
          underVoltage();
          break;
      }
      break;

    case STATEMONITORING :
      // fetch data
      chargeEnabled = 0;          
      chargerCurrentSet( 0 );
      fetchCells();
      fetchAux();

      // update status
      overVoltage();
      underVoltage();
      overTemp();
      setBalance(true, BALTHRESH);
      
      break;
  }
}


#define LEDSTATIDLE   1     // flashing green
#define LEDSTATCHARGE 2     // solid green
#define LEDSTATPOST   3     // green-wash to yellow
#define LEDSTATOV     4     // green-wash to red
#define LEDSTATMON    5     // Blue
#define LEDSTATOT     6     // Blue-wash to red

int ledState = 0;       // neopixel status led

  static int toggle = 0;  
//
// statusLeds
//  update the status leds
//
void statusLeds(void)
{
  float barVal = 0;
  int ibarVal = 0;
  
  // status LED (16 neopixels)
  //  1 is overal status (standby, charging, balancing,hi-volt,lo-volt)
  //  8 are sub-batt status 
  //    White  - comm fail
  //    Blue   - low volt cell
  //    Green  - normal
  //    Yellow - balancing cell
  //    Red    - high volt cell
  //    Purple - high temp

  // clear the colors
  for(int i=0; i<16; i++)
  {
    strip.setPixelColor(i, strip.Color(0,0,0));
  }

  // main status
  if (currentState == STATEIDLE)
  {
    strip.setPixelColor(6, strip.Color(64,64,64));
    ibarVal = 0;
  } else if (currentState == STATECHARGING)
  {
    if (toggle)
    {
      strip.setPixelColor(6, strip.Color(0,128,128));
      toggle = 0;
    }
    else
    {
      strip.setPixelColor(6, strip.Color(0,64,0));
      toggle = 1;
    }
    int lowV = lowestV() + CHEQTHRESH;
    if(setBalance(true, lowV < BALTHRESH? lowV : BALTHRESH))
    {
      strip.setPixelColor(7, strip.Color(64,64,0));   // yellow
    } else if(overVoltage())
    {
      strip.setPixelColor(7, strip.Color(64,0,0));     // red
    } else if (underVoltage())
    {
      strip.setPixelColor(7, strip.Color(0,0,64));     // blue
    } else if (currentState == STATEEQUALIZE)
    {
      strip.setPixelColor(7, strip.Color(64,0,64));     // purple
    } else
    {
      strip.setPixelColor(7, strip.Color(0,64,0));     // green
    }
    if (currentSubState == STATECHPOSTCHARGE)
    {
      strip.setPixelColor(7, strip.Color(64,0,64));
      ibarVal = numBal();
    }
    else
    {
      barVal = (float)(((float)totalVolts()/100.0) - 246.0)/1.8;
      ibarVal = barVal;;
    }
  } else if (currentState == STATEMONITORING)
  {
    strip.setPixelColor(6, strip.Color(0,0,64));
    barVal = (float)(((float)totalVolts()/100.0) - 246.0)/1.94;
    ibarVal = barVal;;
  } else if (currentState == STATEEQUALIZE)
  {
    strip.setPixelColor(6, strip.Color(64,0,64));
    ibarVal = numBal();
  }

  if (ibarVal < 0) ibarVal = 0;
  if (ibarVal > 63) ibarVal = 63;

// volts bargraph
  for(int i=0; i<6; i++)
  {
    if (ibarVal & (1 << i))
    {
      strip.setPixelColor(i, strip.Color(0,32,32));
    }
  }
  
  // per sub battery status
  for(int i=0; i<TOTAL_IC; i++)
  {
    if(currentState != STATEIDLE)
    {
      if (!data_valid[i][0])
      {
        strip.setPixelColor(i+8, strip.Color(32,32,32));     // white
      } 
      else
      {
        if (sub_batt_status[i] & OVERVOLT)
        {
          strip.setPixelColor(i+8, strip.Color(64,0,0));     // red
        } 
        else if (sub_batt_status[i] & UNDERVOLT)
        {
          strip.setPixelColor(i+8, strip.Color(0,0,64));     // blue
        } 
        else if (sub_batt_status[i] & BALANCING)
        {
          strip.setPixelColor(i+8, strip.Color(64,64,0));     // yellow
        }
        else
        {
          strip.setPixelColor(i+8, strip.Color(0,64,0));     // green
        }
      }
    }
  }

  // display the colors
  strip.show();

}

int fanmap[] = {10, 9, 5, 6, 20, 21, 12, 11}; // maps subcell fan number to Dig Out
void fanOnOff(int state, int num)
{
  digitalWrite (fanmap[num], state);
}

void fans(int bal, int temp)
{
  int i;
    for(i=0; i<8; i++)
    {
//      if((bal & (1 << i)) || (temp & (1 << i)))
      if((temp & (1 << i)))
      {
        fanOnOff(LOW, i);
      }
      else
      {
        fanOnOff(HIGH, i);
      }
    }
}

//
// handlePeripherals
//  apply io controls
void handlePeripherals()
{
  static int flag = 0;
  static int flag1 = 0;
  static int counter = 0;
  
  // toggle activity led on feather
  
  // charger output
  if(chargeEnabled) { digitalWrite(A3, LOW); } else { digitalWrite(A3, HIGH); }

  // fan output
  if (counter++ > 5)
  {
    fans(balFanOn, tempFanOn);
    counter = 0;
  }

  
}

// we almost certainly need interrupts to handle this at speed
void handleCAN()
{
	// see if we have a can bus request
    unsigned char len = 0;
    unsigned char buf[18];
	int i;
	
	// multiple messages might be queued up (like condensed + all 8 subcells..)
  for(i=0; i<10; i++)
	{
		if (CAN_MSGAVAIL == CAN.checkReceive())
		{
		  CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf

		  if (524 == CAN.getCanId())
		  {
			// handle collecting and returning condensed status
			if (1 == buf[0])
			{

      
				// send the collected data
				getcond((char *)buf);
				CAN.sendMsgBuf(525,0,8,buf);
				CAN.sendMsgBuf(526,0,8,&buf[8]);
			}
			// handle collecting and returning detailed status for a particular subcell
			else if(2 == buf[0])
			{
				getdetailed ((int)buf[1], buf);
				CAN.sendMsgBuf(525,0,8,buf);
				CAN.sendMsgBuf(526,0,8,&buf[8]);
			}
      else if(3 == buf[0]) // monitor
      {
        currentState = STATEMONITORING;
        Serial.println("got monitor cmd");
      }
      else if(4 == buf[0]) // equalize
      {
        currentState = STATEEQUALIZE;
        currentSubState = STATEEQINIT;
        Serial.println("got equalize cmd");
      }
      else if(5 == buf[0]) // equalize
      {
        currentState = STATECHARGING;
        currentSubState = STATECHINIT;
        Serial.println("got charge cmd");
      }
		}
	}
	}

}

/*!*********************************************************************
  \brief main loop

***********************************************************************/
void loop()                     
{
  int error, count = 0;
  char temp[30];
  int cmd;
  int i;
  int j = 0;


  // main 1 second loop:
  while (1)
  {
  for(i=0; i<100; i++)
  	{
  		// handle can requests
  		handleCAN();
  
  		// wait for next loop
  		delay(10);    // .01 seconds
  	}

  	// handle current state
    handleCurrentState();

	  // handle peripherals like cooling fan, charger enable, charger command and status led
    handlePeripherals();

    // display current status
    statusLeds();

    if (digitalRead(A1))
    {
      chargeMax = 355;
    }
    else
    {
      chargeMax = 368;
    }
    if (digitalRead(A2))
    {
      chargeAmps = 90;  // 5 amps
    }
    else
    {
      chargeAmps = 150;  // 9 amps (might up this to 180 or more..)
    }
  
}

}


/*!***********************************
 Initializes the configuration array
 **************************************/
void init_cfg()
{
  for(int i = 0; i<TOTAL_IC;i++)
  {
    tx_cfg[i][0] = 0xFE;
    tx_cfg[i][1] = 0x00 ; 
    tx_cfg[i][2] = 0x00 ;
    tx_cfg[i][3] = 0x00 ; 
    tx_cfg[i][4] = 0x00 ;
    tx_cfg[i][5] = 0x00 ;
  }
 
}

int chargerAttached = 0;
void chargerAttach()
{
  
  if(!chargerAttached)
  {
    chargeI.attach(1);
    chargerAttached = 1;
  }
}
void chargerDetach()
{
  if(chargerAttached)
  {
    chargeI.detach();
    chargerAttached = 0;
  }
}

int currentSet = 0;
void chargerCurrentSet(int value)
{
  if(value != currentSet)
  {
    chargerAttach();
    chargeI.write(180 - value);
    delay(500);
    chargerDetach();
  }
  
  currentSet = value;
}

int chargerCurrent(void)
{
  return currentSet;
}

void chargerCurrentAdjust(int value)
{
  chargerCurrentSet(currentSet + value);
}


