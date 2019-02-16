#include <PID_v1.h>

#include <mcp_can.h>

#include <Adafruit_NeoPixel.h>

//
//  cold temp is at A0
//  hot temp is at A1
//  pwm is at  12 wired to 6..
//  pump is at A3
// command button is at 5

double Setpoint, Input, Output;

double Kp=2.1, Ki=0.15, Kd=0.1;

PID heaterPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(2, 13, NEO_GRB + NEO_KHZ800);

MCP_CAN CAN(A5);


// temp table
typedef struct temptable {
  uint32_t  baseTemp;   // starting temp
  uint32_t  baseCounts;   // starting counts
} _temptable;

static _temptable temp[] = {  {250,   0},
                {212,     74},
                {190,     99},
                {180,     106},
                {170,     124},
                {160,    144},
                {150,    179},
                {140,    206},
                {130,    244},
                {32,    768},
                {0,     1024},
                {-1,    -1}
};


#define HMODEOFF 0
#define HMODEON  1

//#define PIDCLAMP 80 // about 20% for now max might be 50%
#define PIDCLAMP 200 // about 20% for now max might be 50%

int heaterMode = HMODEOFF;
void setup() 
{
  // init can bus
  // start the can bus interface
  CAN.begin(CAN_125KBPS);

  // init pwm
//  pinMode (12, OUTPUT);
  pinMode (12, INPUT);
  pinMode (5, INPUT);
  digitalWrite(5, HIGH);
  pinMode(A3, OUTPUT);

  pinMode(13, OUTPUT);   // neopix

  // init pid
  heaterPID.SetMode(AUTOMATIC);
  heaterPID.SetOutputLimits(0, PIDCLAMP);
  heaterPID.SetControllerDirection(REVERSE);
  
  // init temp
  
  // set mode to off
  heaterMode = HMODEOFF;
//  heaterMode = HMODEON;

  // for debugging
  Serial.begin(115200);

  // pump off
  digitalWrite(A3, LOW);

 REG_GCLK_GENDIV = GCLK_GENDIV_DIV(1) |          // Divide the 48MHz clock source by divisor 3: 48MHz/3=16MHz
                    GCLK_GENDIV_ID(4);            // Select Generic Clock (GCLK) 4
  while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

  REG_GCLK_GENCTRL = GCLK_GENCTRL_IDC |           // Set the duty cycle to 50/50 HIGH/LOW
                     GCLK_GENCTRL_GENEN |         // Enable GCLK4
                     GCLK_GENCTRL_SRC_DFLL48M |   // Set the 48MHz clock source
                     GCLK_GENCTRL_ID(4);          // Select GCLK4
  while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

  // Enable the port multiplexer for the 4 PWM channels: timer TCC0 outputs
  PORT->Group[g_APinDescription[6].ulPort].PINCFG[g_APinDescription[6].ulPin].bit.PMUXEN = 1;

  // Connect the TCC0 timer to the port outputs - port pins are paired odd PMUO and even PMUXE
  // F & E specify the timers: TCC0, TCC1 and TCC2
  PORT->Group[g_APinDescription[2].ulPort].PMUX[g_APinDescription[2].ulPin >> 1].reg = PORT_PMUX_PMUXO_F | PORT_PMUX_PMUXE_F;
  PORT->Group[g_APinDescription[6].ulPort].PMUX[g_APinDescription[6].ulPin >> 1].reg = PORT_PMUX_PMUXO_F | PORT_PMUX_PMUXE_F;

  // Feed GCLK4 to TCC0 and TCC1
  REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN |         // Enable GCLK4 to TCC0 and TCC1
                     GCLK_CLKCTRL_GEN_GCLK4 |     // Select GCLK4
                     GCLK_CLKCTRL_ID_TCC0_TCC1;   // Feed GCLK4 to TCC0 and TCC1
  while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

  // Dual slope PWM operation: timers countinuously count up to PER register value then down 0
  REG_TCC0_WAVE |= TCC_WAVE_POL(0xF) |         // Reverse the output polarity on all TCC0 outputs
                    TCC_WAVE_WAVEGEN_DSBOTH;    // Setup dual slope PWM on TCC0
  while (TCC0->SYNCBUSY.bit.WAVE);               // Wait for synchronization

  // Each timer counts up to a maximum or TOP value set by the PER register,
  // this determines the frequency of the PWM operation:
  // 400 = 20kHz; 800= 10 kHz; 4000 = 2 kHz
//  REG_TCC0_PER = 4500;      // Set the frequency of the PWM on TCC0 to X khz
  REG_TCC0_PER = 400;      // Set the frequency of the PWM on TCC0 to X khz
  while(TCC0->SYNCBUSY.bit.PER);

  // The CCBx register value corresponds to the pulsewidth in microseconds (us)
//  REG_TCC0_CCB0 = 2200;       // TCC0 CCB0 - 50% duty cycle on D2
//  while(TCC0->SYNCBUSY.bit.CCB0);
//  REG_TCC0_CCB1 = 2200;       // TCC0 CCB1 - 50% duty cycle on D5
//  while(TCC0->SYNCBUSY.bit.CCB1);
//  REG_TCC0_CCB2 = 2200;       // TCC0 CCB2 - 50% duty cycle on D6
//  while(TCC0->SYNCBUSY.bit.CCB2);
//  REG_TCC0_CCB3 = 2200;       // TCC0 CCB3 - 50% duty cycle on D7
//  while(TCC0->SYNCBUSY.bit.CCB3);

  // Divide the 16MHz signal by 1 giving 16MHz (62.5ns) TCC0 timer tick and enable the outputs
  REG_TCC0_CTRLA |= TCC_CTRLA_PRESCALER_DIV1 |    // Divide GCLK4 by 1
                    TCC_CTRLA_ENABLE;             // Enable the TCC0 output
  while (TCC0->SYNCBUSY.bit.ENABLE);              // Wait for synchronization


}

void set_igbt_pwm(int pwm)
{
  REG_TCC0_CCB2 = pwm;
  while(TCC0->SYNCBUSY.bit.CCB2);
}

/******************************************************************************
** Function name:   tempTab
**
** Descriptions:    return temp in degF from adc counts
**
** parameters:      adc counts
** Returned value:    temp
**
******************************************************************************/
int tempTab (int adc)
{
  int tempIndex;
  int rtemp, itdif;
  float vperc, tdif;


  // search the table
  for (tempIndex = 0; temp[tempIndex].baseTemp != -1; tempIndex++)
    {
      if (adc >= temp[tempIndex].baseCounts && adc <temp[tempIndex+1].baseCounts)
      {
        // found the segment, calculate the voltage to return
        rtemp = temp[tempIndex].baseTemp;
        vperc = (float)(adc - temp[tempIndex].baseCounts) / (float)(temp[tempIndex+1].baseCounts - temp[tempIndex].baseCounts);
        tdif = (float)((temp[tempIndex].baseTemp - temp[tempIndex+1].baseTemp));
        tdif *= vperc;
        itdif = (int)tdif;
        rtemp -= itdif;
        return rtemp;
      }
    }

  // not found! zero should be safe
  return 0;
}

int ctmp, htmp;
float cftmp = 0;
float hftmp = 0;
void handleCAN()
{
  // see if we have a can bus request
  unsigned char len = 0;
  unsigned char buf[18];
  int i;
  
  // multiple messages might be queued up 
  for(i=0; i<10; i++)
  {
    if (CAN_MSGAVAIL == CAN.checkReceive())
    {
      CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf

      if (544 == CAN.getCanId())
      {

        // handle collecting and returning heater status
        if (1 == buf[0])
        {
          // send the collected data
          buf[0] = heaterMode;
          buf[1] = (char)ctmp;
          buf[2] = (char)(Output/10);
          CAN.sendMsgBuf(545,0,8,buf);
        }
        // handle on/off commands
        else if(2 == buf[0])
        {
          // on
          heaterMode = HMODEON;
        }
        else if(3 == buf[0]) // monitor
        {
          // off
          heaterMode = HMODEOFF;
        }
      }
    }
  }

}

// code for:
//  read temp
//  get can request
//  pid output
//  handle alarm
int counter = 0; 
int lastCommandstate = 0;
int commandstate = 0;
void loop() 
{
  
  strip.setPixelColor(0, strip.Color(0,0,0));
  strip.setPixelColor(1, strip.Color(0,0,0));
  while(1)
  {
    // get temps
    ctmp = tempTab(1024 - analogRead(A0));
    cftmp = cftmp * .95 + (float)ctmp * .05;
    ctmp = cftmp;
//    Serial.print("Cold Temp "); Serial.print(tmp); Serial.println();
    htmp = tempTab(1024 - analogRead(A1));
    hftmp = hftmp * .95 + (float)htmp * .05;
    htmp = hftmp;
//    Serial.print("Hot Temp "); Serial.print(tmp); Serial.println();
//    Serial.print("command "); Serial.print(digitalRead(5)); Serial.println();

    // set output
//    set_igbt_pwm(200);

    // get command state
    if(digitalRead(5) == 0 && lastCommandstate == 1)
    {
      lastCommandstate = 0;
      if(commandstate == 0)
      {
        commandstate = 1;
      }
      else
      {
        commandstate = 0;
      }
    }
    if(digitalRead(5) == 1) lastCommandstate = 1;
    
    // handle current state
    if (commandstate == 0) 
    {
      strip.setPixelColor(1, strip.Color(0,0,0)); // output color off
      strip.setPixelColor(0, strip.Color(0,0,32)); // off color
      digitalWrite(A3, 0); // pump off

      // zero the output
      heaterPID.SetMode(MANUAL);
      heaterPID.SetMode(AUTOMATIC);
      Input = 0;
      Setpoint = 130;
    
      heaterPID.Compute();
      set_igbt_pwm(0);
    }
    else 
    {
      int red, green, blue;
      if (Output < 100) red = 0; else red = (Output - 100);
      if (Output > 100) blue = 0; else blue = 100 - Output;
      if (Output < 50 || Output > 150) green = 0; else green = (Output-50);
//Serial.print(Output); Serial.print(" "); 
//Serial.print(red); Serial.print(" "); 
//Serial.print(green); Serial.print(" "); 
//Serial.print(blue); Serial.print(" "); 
//Serial.println();
      strip.setPixelColor(1, strip.Color(red,green,blue)); // output color
      strip.setPixelColor(0, strip.Color(64,64,0)); // on color
      digitalWrite(A3, 1); // pump on

      // pid the output
      Input = ctmp;
      Setpoint = 60;
    
      heaterPID.Compute();
//Serial.print(ctmp); Serial.print(" "); Serial.print(Output); 
//Serial.print(" "); Serial.print(analogRead(A0)); Serial.println();
      // apply the control value
      set_igbt_pwm(Output);
    }
    strip.show();

    // next loop
    delay(50);
  }
  
  // can bus check for input
  handleCAN();

  // get temperature
  ctmp = tempTab(1024 - analogRead(A0));
  cftmp = cftmp * .95 + (float)ctmp * .05;
  ctmp = cftmp;
  
  // execute PID
  if(heaterMode == HMODEON)
  {
    // pump on
    digitalWrite(A3, HIGH);

    Input = ctmp;
    Setpoint = 130;
    
    heaterPID.Compute();

    // limit output

    // apply the control value
    set_igbt_pwm(Output);
//    analogWrite(12, (char)Output);   // pwm on the heater set to pid control

  }
  else
  {
    // temp control off
    Output = 0;
    set_igbt_pwm(0);
//    analogWrite(12, 0);   // pwm on the heater IGBT set to zero

    // pump off
    digitalWrite(A3, LOW);
  }
  
  // update neopixel status display
  // 
  // clear the colors
  for(int i=0; i<16; i++)
  {
    strip.setPixelColor(i, strip.Color(0,0,0));
  }
  if(heaterMode == HMODEON)
  {
    strip.setPixelColor(0, strip.Color(0,0,64));
    strip.setPixelColor(1, strip.Color(Output,Output,0));
  }
  else
  {
    strip.setPixelColor(0, strip.Color(0,64,0));
    strip.setPixelColor(1, strip.Color(0,0,0));
  }
  strip.show();

  // do some waiting
  delay(50);
}
