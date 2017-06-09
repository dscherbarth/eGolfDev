
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

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include <Adafruit_NeoPixel.h>

// COMMON SETTINGS
// ----------------------------------------------------------------------------------------------
// These settings are used in both SW UART, HW UART and SPI mode
// ----------------------------------------------------------------------------------------------
#define BUFSIZE                        128   // Size of the read buffer for incoming data
#define VERBOSE_MODE                   true  // If set to 'true' enables debug output

// SHARED SPI SETTINGS
// ----------------------------------------------------------------------------------------------
// The following macros declare the pins to use for HW and SW SPI communication.
// SCK, MISO and MOSI should be connected to the HW SPI pins on the Uno when
// using HW SPI.  This should be used with nRF51822 based Bluefruit LE modules
// that use SPI (Bluefruit LE SPI Friend).
// ----------------------------------------------------------------------------------------------
#define BLUEFRUIT_SPI_CS               8
#define BLUEFRUIT_SPI_IRQ              7
#define BLUEFRUIT_SPI_RST              4    // Optional but recommended, set to -1 if unused

// SOFTWARE SPI SETTINGS
// ----------------------------------------------------------------------------------------------
// The following macros declare the pins to use for SW SPI communication.
// This should be used with nRF51822 based Bluefruit LE modules that use SPI
// (Bluefruit LE SPI Friend).
// ----------------------------------------------------------------------------------------------
#define BLUEFRUIT_SPI_SCK              13
#define BLUEFRUIT_SPI_MISO             12
#define BLUEFRUIT_SPI_MOSI             11

#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, A2, NEO_GRB + NEO_KHZ800);

const uint8_t TOTAL_IC = 8;//!<number of ICs in the daisy chain
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

/*!**********************************************************************
 \brief  Inititializes hardware and variables
 ***********************************************************************/
void setup()                  
{
  // init the 6804
  LTC6804_initialize(); //Initialize LTC6804 hardware
  init_cfg();           //initialize the 6804 configuration array to be written
  pinMode(10,OUTPUT);   // chip select for ltc6820 spi interface 

  // init the ble radio
  ble.begin(VERBOSE_MODE);
  ble.factoryReset();
  ble.echo(false);
  ble.verbose(false);  // debug info is a little annoying after this point!

  // setup peripheral io
  pinMode(A0, OUTPUT);    // charger enable
  pinMode(A1, OUTPUT);    // fan control
  pinMode(A2, OUTPUT);    // status led (neopixel)
  pinMode(A3, INPUT);     // charger requested to start (j1772 plug inserted)
  pinMode (13, OUTPUT);   // local activity indication
  
  Serial.begin(115200);
}

//
// handleBLEConnect()
//
//  test current connect state and ble connected state and set the appropriate mode and flag
//
static int Connected = 0;

void handleBLEConnect()
{
    if (!Connected && ble.isConnected())
    {
      Serial.println("Connected");
      ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
      ble.setMode(BLUEFRUIT_MODE_DATA);
      Connected = 1;
    }
    else if (Connected && !ble.isConnected())
    {
      Serial.println("Not Connected");
      ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
      Connected = 0;
    }
}

#define BLECMDCHARGE  1
#define BLECMDSTOP    2
#define BLECMDMONITOR 3
#define BLECMDFETCH   4
#define BLECMDEQUALIZE   5

#define STATEIDLE         1
#define STATECHARGING     2
#define STATECHINIT       21
#define STATECHINPROG     22
#define STATECHPOSTCHARGE 23
#define STATEMONITORING   3
#define STATEEQUALIZE     4
#define STATEEQINIT       41
#define STATEEQINPROG     42

// accept and parse a ble channel command
int getBLECommand(int *cmd)
{
  // if connected read available bytes and convert to a command
  *cmd = 0;
  
  if (Connected)
  {
    if ( ble.available() )
    {
      int c = ble.read();
  
      switch (c)
      {
        case 'C' :
        case 'c' :
          Serial.println("Cmd charge");
          *cmd = BLECMDCHARGE;
          break;
          
        case 'S' :
        case 's' :
          Serial.println("Cmd stop");
          *cmd = BLECMDSTOP;
          break;
       
        case 'M' :
        case 'm' :
          Serial.println("Cmd monitor");
          *cmd = BLECMDMONITOR;
          break;

        case 'F' :
        case 'f' :
          Serial.println("Cmd fetch");
          *cmd = BLECMDFETCH;
          break;

        case 'E' :
        case 'e' :
          Serial.println("Cmd equalize");
          *cmd = BLECMDEQUALIZE;
          break;
      }
//      Serial.print((char)c);
  
//      // Hex output too, helps w/debugging!
//      Serial.print(" [0x");
//      if (c <= 0xF) Serial.print(F("0"));
//      Serial.print(c, HEX);
//      Serial.print("] ");
      return 1;
    }
  }
  return 0;
}

//int currentState = STATEIDLE;
int currentState = STATEMONITORING;
//int currentState = STATECHARGING;
int currentSubState = 0;
int chargeEnabled = 0;  // digital output
int balFanOn = 0;       // digital output

void executeBLECommand(int cmd)
{
  switch (cmd)
  {
    case BLECMDCHARGE :
      // need to be in idle mode
      if (currentState == STATEIDLE)
      {
        Serial.println("exe charge");
        currentState = STATECHARGING;
        currentSubState = STATECHINIT;
      }
      break;
      
    case BLECMDEQUALIZE :
      // need to be in idle mode
      if (currentState == STATEIDLE)
      {
        Serial.println("exe charge");
        currentState = STATEEQUALIZE;
        currentSubState = STATEEQINIT;
      }
      break;
      
    case BLECMDSTOP :
      // need to be in charge or monitor mode
      if (currentState == STATECHARGING || currentState == STATEEQUALIZE || currentState == STATEMONITORING )
      {
        // set proper states
        Serial.println("exe stop");
        currentState = STATEIDLE;
        currentSubState = 0;

        // disable the charger
        Serial.println("3 disable charge");
        chargeEnabled = 0;
        
        // shut off the fan
        balFanOn = 0;
        
      }
      break;
      
    case BLECMDMONITOR :
      // need to be in idle mode
      if (currentState == STATEIDLE)
      {
        Serial.println("Cmd monitor");
        currentState = STATEMONITORING;
      }
      break;

    case BLECMDFETCH :
      // need to be in monitor or equalize or charge mode
      if (currentState == STATEMONITORING || currentState == STATEEQUALIZE || currentState == STATECHARGING)
      {
        // assuming we are connected, respond with current data
        if (Connected)
        {
          // send cell data and crc (looks like 16 bits of count of data and crc, data, 32bit crc)
          crcAndSend ((char *)&TOTAL_IC, 1);
          
          // send cell data and crc (looks like 16 bits of count of data and crc, data, 32bit crc)
//          crcAndSend ((char *)cell_codes, sizeof(cell_codes));
          crcAndSend ((char *)aux_codes, sizeof(aux_codes));
          
          // send aux data and crc (looks like 16 bits of count of data and crc, data, 32bit crc)
//          crcAndSend ((char *)aux_codes, sizeof(aux_codes));
          crcAndSend ((char *)cell_codes, sizeof(cell_codes));
          
          // send data_valid and crc (looks like 16 bits of count of data and crc, data, 32bit crc)
          crcAndSend ((char *)&data_valid[0], sizeof(data_valid));

          // send last_data counter (looks like 16 bits of count of data and crc, data, 32bit crc)
          crcAndSend ((char *)&last_data, sizeof(last_data));
        }
      }
      break;
  }
}

// crcAndSend 
//    calculate a crc over the supplied data and format into lv and send over the ble uart connection
//    we will only get here if we are already connected
//
void crcAndSend (char * data, int data_length)
{
//  uint32_t  crc = 0;

//  for(int i=0; i<data_length; i++)
//  {
//    crc += data[i];
//  }

//  data_length += 4; // add the length of the crc
//  ble.write((const uint8_t *)&data_length, 2);         // sent the length
//  ble.write((const uint8_t *)data, data_length - 4);   // send the data
  ble.write((const uint8_t *)data, data_length);   // send the data
//  ble.write((const uint8_t *)&crc, 4);                 // send the crc
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

  // update the valid data counters
  for(int i=0; i<TOTAL_IC; i++)
  {
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

#define BALTHRESH 41000   // 4.10 volts

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

  balcnt = 0;
  // check all the valid data in cells for > BALLIMIT
  for(int i=0; i<TOTAL_IC; i++)
  {
    sub_batt_status[i] &= ~BALANCING;

    icBal[i][0] = 0xf9; icBal[i][1] = 1; icBal[i][2] = 0x3d; icBal[i][3] = 0x6e;
    icBal[i][4] = icBal[i][5] = 0;
    for(int j=0; j<11; j++)
    {
      if (data_valid[i][0] && cell_codes[i][j] > balvolts)
      {
        // this one needs balancing
        if (j < 8) icBal[i][4] |= (1 << j); else icBal[i][5] |= (1 << (j-8));

        // some balancing is happening
        balcnt++;
        balFlag = true;

        // set the sub-batt status
        sub_batt_status[i] |= BALANCING;
      }
    }
  }

  if (take_action)
  {
    // write the balance config
    wakeup_sleep();
    LTC6804_wrcfg(TOTAL_IC,icBal);

    // if balancing turn on the fan
    if (balFlag) { balFanOn = 1; } else { balFanOn = 0; }
  }

  return balFlag;
}

#define HVLIMIT  42500    // 4.25 volts

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
#define HTLIMIT  70       // 70 deg C

//
// overTemp
//  convert temperature values and test against high limits
//
boolean overTemp( void )
{
  boolean ots = false;  

  for(int i=0; i<TOTAL_IC; i++)
  {
    // reset status
    sub_batt_status[i] &= ~OVERTEMP;
    
//    Serial.println("Temps ");
    for(int j=0; j<5; j++)
    {
//      Serial.print((((15000 - aux_codes[i][j]) / 320) + 25)); Serial.print(" ");
      if ((data_valid[i][0] > 3)&& (((aux_codes[i][j] - 15000) / 300) + 25) > HTLIMIT)
      {
        ots = true;

        // update the status
        sub_batt_status[i] |= OVERTEMP;
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

#define BALTHRESH 41000   // 4.10 volts
#define PACKCHARGED 36200 // target is 360 but measure during charging is low

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
  
Serial.print(tv); Serial.print(" ");
Serial.println("total volts");
  return tv;
}

#define POSTCHARGEWAIT  3600    // balance for up to 1 hour
int postChargeBalance = 0;
int eqvolts = 41500;    // default start point
int eqcounter = 0;

int lowestV()
{
    int lowvolts = 41000;
    for (int i=0; i<TOTAL_IC; i++)
    {
      for (int j=0; j<11; j++)
      {
        if (data_valid[i][0] > 3 && (cell_codes[i][j] < lowvolts))
        {
          lowvolts = cell_codes[i][j];
        }
      }
    }
    return lowvolts;
}

// handleCurrentState
//  manage the state machine
//
void handleCurrentState()
{

  
  switch (currentState)
  {
    case   STATEIDLE :
      break;

    case STATEEQUALIZE :
      // get some recent data
      fetchCells();
      fetchAux();

      switch (currentSubState)
      {
        case STATEEQINIT :
          // determine the lowest voltage
          eqvolts = lowestV();
Serial.print(eqvolts); Serial.print(" ");
Serial.println("eq volts");

          // make sure that we don't equalize too low
          if(eqvolts < 28000) eqvolts = 28000;
          
          // set to next step
          currentSubState = STATEEQINPROG;
          break;

        case STATEEQINPROG :
          // see if we are done balancing
          int notdone = 0;
          if (eqcounter++ > 10)
          {
            for (int i=0; i<TOTAL_IC; i++)
            {
              for (int j=0; j<11; j++)
              {
                if (data_valid[i][0] > 3 && (cell_codes[i][j] > eqvolts))
                {
Serial.print(i); Serial.print(" ");
Serial.println("not done");
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
              balFanOn = 0;
            }
          }
          break;
      }
      
    case STATECHARGING :
      switch (currentSubState)
      {
        case STATECHINIT :
          // setup
          // enable charger
          chargeEnabled = 1;          

          // move to in prog
          currentSubState = STATECHINPROG;
          break;
          
        case STATECHINPROG :
          // while not charged and not overvolt and not overtemp
          fetchCells();
          fetchAux();
          
          // fetch data and set balance
          if (totalVolts() >= PACKCHARGED || overVoltage())
          {
            // disable charger
            chargeEnabled = 0;          

            // move to postcharge
            currentSubState = STATECHPOSTCHARGE;

            // start timer
            postChargeBalance = POSTCHARGEWAIT;
          }
          else
          {
            #define CHEQTHRESH 50   // .05 Volts
            int lowV = lowestV() + CHEQTHRESH;
            setBalance(true, lowV < BALTHRESH? lowV : BALTHRESH);
          }
          break;
          
        case STATECHPOSTCHARGE :
          // pack charge reached (total volts > 360 or cell(s) over-voltage)

          // fetch data and
          fetchCells();
          fetchAux();

          // if no more balance needed or timer runs out we are done with post charge
          if (!setBalance(true, BALTHRESH) || postChargeBalance-- <= 0)
          {
            // done with post charge
            currentSubState = 0;
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
      fetchCells();
      fetchAux();

      // update status
      overVoltage();
      underVoltage();
      overTemp();
      setBalance(true, BALTHRESH);
//      setBalance(false, BALTHRESH);
      
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

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

//
// statusLeds
//  update the status leds
//
int a=0,b=0,c=0;
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
  a += 1; b += 5; c += 7;
//  if(a>255) a=0; if(b>255) b=0; if(c>255) c=0;
//  strip.setPixelColor(15, Wheel(a));
//  strip.setPixelColor(14, Wheel((a+64)%256));
//  strip.setPixelColor(13, Wheel((a+128)%256));
//  strip.setPixelColor(12, Wheel((a+192)%256));
  
  // main status
  if (currentState == STATEIDLE)
  {
    strip.setPixelColor(9, strip.Color(64,64,64));
    ibarVal = 0;
  } else if (currentState == STATECHARGING)
  {
    strip.setPixelColor(9, strip.Color(0,64,0));
    int lowV = lowestV() + CHEQTHRESH;
    if(setBalance(true, lowV < BALTHRESH? lowV : BALTHRESH))
    {
      strip.setPixelColor(8, strip.Color(64,64,0));   // yellow
    } else if(overVoltage())
    {
      strip.setPixelColor(8, strip.Color(64,0,0));     // red
    } else if (underVoltage())
    {
      strip.setPixelColor(8, strip.Color(0,0,64));     // blue
    } else if (currentState == STATEEQUALIZE)
    {
      strip.setPixelColor(8, strip.Color(64,0,64));     // purple
    } else
    {
      strip.setPixelColor(8, strip.Color(0,64,0));     // green
    }
    barVal = (float)(totalVolts() - 246)/1.8;
    ibarVal = barVal;;
  } else if (currentState == STATEMONITORING)
  {
    strip.setPixelColor(9, strip.Color(0,0,64));
    barVal = (float)((totalVolts()/100.0) - 246)/1.8;
    ibarVal = barVal;;
  } else if (currentState == STATEEQUALIZE)
  {
    strip.setPixelColor(9, strip.Color(64,0,64));
    ibarVal = numBal();
  }

  Serial.print(barVal);
  Serial.print(ibarVal);
  Serial.println();

  if (ibarVal < 0) ibarVal = 0;
  if (ibarVal > 63) ibarVal = 63;

  Serial.print(ibarVal);
  Serial.println();
  
// volts bargraph
  for(int i=0; i<6; i++)
  {
    if (ibarVal & (1 << i))
    {
      strip.setPixelColor(10 + i, strip.Color(0,32,32));
    }
  }
  
  // per sub battery status
  for(int i=0; i<TOTAL_IC; i++)
  {
    if(currentState != STATEIDLE)
    {
      if (!data_valid[i][0])
      {
        strip.setPixelColor(i, strip.Color(32,32,32));     // white
      } 
      else
      {
        if (sub_batt_status[i] & OVERVOLT)
        {
          strip.setPixelColor(i, strip.Color(64,0,0));     // red
        } 
        else if (sub_batt_status[i] & UNDERVOLT)
        {
          strip.setPixelColor(i, strip.Color(0,0,64));     // blue
        } 
        else if (sub_batt_status[i] & BALANCING)
        {
          strip.setPixelColor(i, strip.Color(64,64,0));     // yellow
        }
        else
        {
          strip.setPixelColor(i, strip.Color(0,64,0));     // green
        }
      }
    }
  }

  // display the colors
  strip.show();

}

//
// handlePeripherals
//  apply io controls
void handlePeripherals()
{
  static int flag = 0;
  static int flag1 = 0;
  
  // toggle activity led on feather
  if (flag) {digitalWrite(13, HIGH); flag = 0;}else{digitalWrite(13,LOW); flag = 1;}
  
  // charger output
  if(chargeEnabled) { digitalWrite(A0, HIGH); } else { digitalWrite(A0, LOW); }

  // fan output
  if(balFanOn) 
  {
    digitalWrite(A1, HIGH);
  }
  else
  {
    digitalWrite(A1, LOW);
  }

  // display current status
  statusLeds();
  
  // charge requested via j1772 plugin
  if (digitalRead(A3))
  {
    if (currentState == STATEIDLE)
    {
//      currentState = STATECHARGING;
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


#define newcode 1
#ifdef newcode
// main 1 second loop:
  while (1)
  {
    
//  handle ble connection
    handleBLEConnect();

//  handle ble commands
    if (Connected)
    {
      if (getBLECommand(&cmd))
      {
        executeBLECommand(cmd);
      }
    }

//  handle current state
    handleCurrentState();

// handle peripherals like cooling fan, charger enable, charger command and status led
    handlePeripherals();
        
// wait for next loop
    delay(1000);    // 1 second
  }
#endif  // newcode

    delay(500);
//#ifdef aaa
    wakeup_sleep();
    LTC6804_wrcfg(TOTAL_IC,tx_cfg);
//    print_config();
    wakeup_sleep();
    error = LTC6804_rdcfg(TOTAL_IC,rx_cfg);
    if (!Connected && ble.isConnected())
    {
      Serial.println("Connected");
      ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
      ble.setMode(BLUEFRUIT_MODE_DATA);
      Connected = 1;
    }
    else if (Connected && !ble.isConnected())
    {
      Serial.println("Not Connected");
      ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
      Connected = 0;
    }
    if (Connected)
    {
      while ( ble.available() )
      {
        int c = ble.read();
    
        Serial.print((char)c);
    
        // Hex output too, helps w/debugging!
        Serial.print(" [0x");
        if (c <= 0xF) Serial.print(F("0"));
        Serial.print(c, HEX);
        Serial.print("] ");
      }

      sprintf(temp, "c0 %6.4f\n", cell_codes[0][0]*0.0001);
      Serial.println("Sending");
      Serial.println(temp);
      if(ble.print(temp))
      {
        Serial.println("Sent");
      }
      else
      {
        Serial.println("Not Sent");
      }
    }
    if (error == -1)
    {
     Serial.println("1 A PEC error was detected in the received data");
    }
#define more 1
#ifdef more    
//    print_rxconfig();
//    wakeup_sleep();
    LTC6804_adcv();
    delay(3);
//    Serial.println("cell conversion completed");
//    Serial.println();
    wakeup_sleep();
    wakeup_sleep();
    error = LTC6804_rdcv(0, TOTAL_IC,cell_codes, NULL); // Set to read back all cell voltage registers
    if (error == -1)
    {
       Serial.println("2 A PEC error was detected in the received data");
    }
    print_cells();
    wakeup_sleep();
    LTC6804_adax();
    delay(3);
//    Serial.println("aux conversion completed");
//    Serial.println();
    wakeup_sleep();
    error = LTC6804_rdaux(0,TOTAL_IC,aux_codes, NULL); // Set to read back all aux registers
    if (error == -1)
    {
      Serial.println("3 A PEC error was detected in the received data");
    }
    print_aux();
#endif
//#endif  
  
//  if (Serial.available())           // Check for user input
//    {
//      uint32_t user_command;
//      user_command = read_int();      // Read the user command
//      Serial.println(user_command);
//      run_command(user_command);
//    }
}


/*!*****************************************
  \brief executes the user inputted command
  
  Menu Entry 1: Write Configuration \n
   Writes the configuration register of the LTC6804. This command can be used to turn on the reference 
   and increase the speed of the ADC conversions. 
   
 Menu Entry 2: Read Configuration \n
   Reads the configuration register of the LTC6804, the read configuration can differ from the written configuration.
   The GPIO pins will reflect the state of the pin

 Menu Entry 3: Start Cell voltage conversion \n
   Starts a LTC6804 cell channel adc conversion.

 Menu Entry 4: Read cell voltages
    Reads the LTC6804 cell voltage registers and prints the results to the serial port.
 
 Menu Entry 5: Start Auxiliary voltage conversion
    Starts a LTC6804 GPIO channel adc conversion.

 Menu Entry 6: Read Auxiliary voltages6118
    Reads the LTC6804 axiliary registers and prints the GPIO voltages to the serial port.
 
 Menu Entry 7: Start cell voltage measurement loop
    The command will continuously measure the LTC6804 cell voltages and print the results to the serial port.
    The loop can be exited by sending the MCU a 'm' character over the serial link.
 
*******************************************/
void run_command(uint32_t cmd)
{
  int8_t error = 0;
  
  char input = 0;
  switch(cmd)
  {
   
  case 1:
    wakeup_sleep();
    LTC6804_wrcfg(TOTAL_IC,tx_cfg);
    print_config();
    break;
    
  case 2:
    wakeup_sleep();
    error = LTC6804_rdcfg(TOTAL_IC,rx_cfg);
    if (error == -1)
    {
     Serial.println("A PEC error was detected in the received data");
    }
    print_rxconfig();
    break;

  case 3:
    wakeup_sleep();
    LTC6804_adcv();
    delay(3);
    Serial.println("cell conversion completed");
    Serial.println();
    break;
    
  case 4:
    wakeup_sleep();
    error = LTC6804_rdcv(0, TOTAL_IC,cell_codes, NULL); // Set to read back all cell voltage registers
    if (error == -1)
    {
       Serial.println("A PEC error was detected in the received data");
    }
    print_cells();
    break;
    
  case 5:
    wakeup_sleep();
    LTC6804_adax();
    delay(3);
    Serial.println("aux conversion completed");
    Serial.println();
    break;
    
  case 6:
    wakeup_sleep();
    error = LTC6804_rdaux(0,TOTAL_IC,aux_codes, NULL); // Set to read back all aux registers
    if (error == -1)
    {
      Serial.println("A PEC error was detected in the received data");
    }
    print_aux();
    break;
  
  case 7:
    Serial.println("transmit 'm' to quit");
    wakeup_sleep();
    LTC6804_wrcfg(TOTAL_IC,tx_cfg);
    while (input != 'm')
    {
      if (Serial.available() > 0)
      {
        input = read_char();
      }
      wakeup_idle();
      LTC6804_adcv();
      delay(10);
      wakeup_idle();
      error = LTC6804_rdcv(0, TOTAL_IC,cell_codes, NULL);
      if (error == -1)
      {
       Serial.println("A PEC error was detected in the received data");
      }
      print_cells();
      delay(500);
    }
    print_menu();
    break;  
 
  default:
     Serial.println("Incorrect Option");
     break; 
  }
}

/*!***********************************
 \brief Initializes the configuration array
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

/*!*********************************
  \brief Prints the main menu 
***********************************/
void print_menu()
{
  Serial.println("Please enter LTC6804 Command");
  Serial.println("Write Configuration: 1");
  Serial.println("Read Configuration: 2");
  Serial.println("Start Cell Voltage Conversion: 3");
  Serial.println("Read Cell Voltages: 4");
  Serial.println("Start Aux Voltage Conversion: 5");
  Serial.println("Read Aux Voltages: 6");
  Serial.println("loop cell voltages: 7");
  Serial.println("Please enter command: ");
   Serial.println();
}



/*!************************************************************
  \brief Prints cell coltage codes to the serial port
 *************************************************************/
void print_cells()
{

  
  for (int current_ic = 0 ; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(" IC ");
    Serial.print(current_ic+1,DEC);
    Serial.print(" ");
    for(int i=0; i<12; i++)
    {
//      Serial.print(" C");
//      Serial.print(i+1,DEC);
//      Serial.print(":");
      Serial.print(cell_codes[current_ic][i]*0.0001,4);
      Serial.print(",");
    }
     Serial.println(); 
  }
//    Serial.println(); 
}

/*!****************************************************************************
  \brief Prints GPIO voltage codes and Vref2 voltage code onto the serial port
 *****************************************************************************/
void print_aux()
{
  
  for(int current_ic =0 ; current_ic < TOTAL_IC; current_ic++)
  {
    Serial.print(" IC ");
    Serial.print(current_ic+1,DEC);
    Serial.print(" ");
    for(int i=0; i < 5; i++)
    {
//      Serial.print(" GPIO-");
//      Serial.print(i+1,DEC);
//      Serial.print(":");
      Serial.print(aux_codes[current_ic][i]*0.0001,4);
      Serial.print(",");
    }
     Serial.print(" Vref2");
     Serial.print(":");
     Serial.print(aux_codes[current_ic][5]*0.0001,4);
     Serial.println();

//    for(int i=0; i < 5; i++)
//    {
//      Serial.print(" Aux-");
//      Serial.print(i+1,DEC);
//      Serial.print(":");
//      Serial.print(aux_codes[current_ic][i+6]*0.0001,4);
//      Serial.print(",");
//    }
//   Serial.println();
  }
//  Serial.println(); 
}
/*!******************************************************************************
 \brief Prints the configuration data that is going to be written to the LTC6804
 to the serial port.
 ********************************************************************************/
void print_config()
{
  int cfg_pec;
  
  Serial.println("Written Configuration: ");
  for (int current_ic = 0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(" IC ");
    Serial.print(current_ic+1,DEC);
    Serial.print(": ");
    Serial.print("0x");
    serial_print_hex(tx_cfg[current_ic][0]);
    Serial.print(", 0x");
    serial_print_hex(tx_cfg[current_ic][1]);
    Serial.print(", 0x");
    serial_print_hex(tx_cfg[current_ic][2]);
    Serial.print(", 0x");
    serial_print_hex(tx_cfg[current_ic][3]);
    Serial.print(", 0x");
    serial_print_hex(tx_cfg[current_ic][4]);
    Serial.print(", 0x");
    serial_print_hex(tx_cfg[current_ic][5]);
    Serial.print(", Calculated PEC: 0x");
    cfg_pec = pec15_calc(6,&tx_cfg[current_ic][0]);
    serial_print_hex((uint8_t)(cfg_pec>>8));
    Serial.print(", 0x");
    serial_print_hex((uint8_t)(cfg_pec));
    Serial.println(); 
  }
   Serial.println(); 
}

/*!*****************************************************************
 \brief Prints the configuration data that was read back from the 
 LTC6804 to the serial port.
 *******************************************************************/
void print_rxconfig()
{
  Serial.println("Received Configuration ");
  for (int current_ic=0; current_ic<TOTAL_IC; current_ic++)
  {
    Serial.print(" IC ");
    Serial.print(current_ic+1,DEC);
    Serial.print(": 0x");
    serial_print_hex(rx_cfg[current_ic][0]);
    Serial.print(", 0x");
    serial_print_hex(rx_cfg[current_ic][1]);
    Serial.print(", 0x");
    serial_print_hex(rx_cfg[current_ic][2]);
    Serial.print(", 0x");
    serial_print_hex(rx_cfg[current_ic][3]);
    Serial.print(", 0x");
    serial_print_hex(rx_cfg[current_ic][4]);
    Serial.print(", 0x");
    serial_print_hex(rx_cfg[current_ic][5]);
    Serial.print(", Received PEC: 0x");
    serial_print_hex(rx_cfg[current_ic][6]);
    Serial.print(", 0x");
    serial_print_hex(rx_cfg[current_ic][7]);
    Serial.println(); 
  }
   Serial.println(); 
}

void serial_print_hex(uint8_t data)
{
    if (data< 16)
    {
      Serial.print("0");
      Serial.print((byte)data,HEX);
    }
    else
      Serial.print((byte)data,HEX);
}
