int logActive = 0;
int btnAction = 0;
int dataArray [13];    // data as received
int dataItems = 0;         // number of items in the array

int snapTaken = 0;
int snapReady = 1;
int snapRes   = 1;  // multiples of 100uS per sample 1 is 10kHz, 20 is 500Hz
extern int snapCnt;

int accelVal = 0;  // holds val to send on cmd

void dispBtnState (void)
{
  lcd.setCursor(0, 3);
  lcd.print("            ");
  lcd.setCursor(0, 3);
  switch (btnAction)
  {
    case 0:    // controller actions
      // update start/stop/fault message
      if (startState == 2 && dataArray[8] == 0)
      {
        // already cleared
        startState = 0;
      }
      
      switch (startState)
      {
        case 0:
        lcd.print("CRTL Start");
        break;
        
        case 1:
        lcd.print("CRTL Stop ");
        break;
        
        case 2:
        lcd.print("CLR Fault ");
        break;
      }    
      break;
      
    case 1:    // logger actions
      if (SDOK)    // only start/stop logging if the sd card initialized ok
      {
        if (!logActive)
        {
           lcd.print("LOG Start");
        }
        else
        {
           lcd.print("LOG Stop ");
         }
      }
      else
      {      
         lcd.print("---------");
      }
      break;

    case 2:    // dac reflector settings
        strcpy_P(tbuf, (char *)pgm_read_word(&(dacrArray[dacRIndex]))); // strings are in flash
        lcd.print(tbuf);
      break;

    case 3:    // pid
        sprintf (tbuf, "Slp Fac %d.%d", pidDp/10, pidDp%10);
        lcd.print(tbuf);
      break;
    case 4:    // pid
        sprintf (tbuf, "Pid Kp .%02d", pidDi);
        lcd.print(tbuf);
      break;
    case 5:    // pid
        sprintf (tbuf, "Pid Ki .%02d", pidQp);
        lcd.print(tbuf);
      break;
    case 6:    // pid
        sprintf (tbuf, "Dcl Fac .%02d", pidQi);
        lcd.print(tbuf);
      break;
    case 7:    // pid
        sprintf (tbuf, "CLim %d", pidSp);
        lcd.print(tbuf);
      break;
    case 8:    // pid
        sprintf (tbuf, "dq Fac .%02d", pidSi);
        lcd.print(tbuf);
      break;
      
    case 9:    // snapshot
        if (snapReady)
        {
          if (snapTaken)
          {
             lcd.print("Get Snap ");
          }
          else
          {
            switch (snapRes)
            {
            case 0:
                lcd.print("Take S 10kHz");
                break;
            case 1:
                lcd.print("Take S 5kHz ");
                break;
            case 2:
                lcd.print("Take S 1kHz ");
                break;
            case 3:
                lcd.print("Take S .5kHz");
                break;
            }
          }
        }
        else
        {
          sprintf (tbuf, "Rcv Sn %d", snapCnt);
          lcd.print(tbuf);
        }
      break;
      
    case 10:  // accel sim
        sprintf (tbuf, "Accel %d", accelVal);
        lcd.print(tbuf);
        break;
  }
}

void loggerAction(void)
{
    if (SDOK)    // only start/stop logging if the sd card initialized ok
    {
      // first press
      if (!logActive)
      {
        LogSelectedDataStart();
        logActive = 1;
      }
      else
      {
        // stop the logging
        LogSelectedDataStop();
        logActive = 0;
      }
    }
}

void controllerAction()
{
   if (startState == 0)  // controller stopped, only can start
   {
     // set start cmd
     ctrlCmd = HEADCMDSTART;
   }
   else if (startState == 1) // started, only can stop
   {
     // set stop cmd
     ctrlCmd = HEADCMDSTOP;
   }
   else if (startState == 2) // faulted, only can clear
   {
     // set clear cmd
     ctrlCmd = HEADCMDCLEAR;
   }
   message1.id = 356;
   message1.data[0] = ctrlCmd;
   CAN.send_message (&message1);
}

void dacReflectorSet (void)
{
  // save dac reflector index and send to the MKIII controller
   message1.id = 357;
   message1.data[0] = dacRIndex;
   CAN.send_message (&message1);
}

void accelAct (void)
{
  // send the selected rpm cmd
   message1.id = 382;
   message1.data[0] = accelVal & 0xff;
   message1.data[1] = (accelVal >> 8) & 0xff;
   CAN.send_message (&message1);

   lcd.setCursor(0, 3);
   lcd.print ("Sending Jog");
   delay (1000);
}

void parmAdjust (int btnAction)
{
    int val;
    
    
  // save the appropriate parameter
  switch (btnAction)
  {
      case 3:
        val = pidDp;
        break;
      case 4:
        val = pidDi;
        break;
      case 5:
        val = pidQp;
        break;
      case 6:
        val = pidQi;
        break;
      case 7:
        val = pidSp;
        break;
      case 8:
        val = pidSi;
        break;
  }
   message1.id = 366 + btnAction - 3;
   message1.data[0] = val & 0xff;
   message1.data[1] = val >> 8;
   CAN.send_message (&message1);
}

void snapAct()
{
  if (snapReady)
  {
    if (snapTaken)
    {
      // create an open the file
      SnapStart();
      
      // fetch it
     message1.id = 381;
     CAN.send_message (&message1);
      
      snapTaken = 0;
    }
    else
    {
      // ask for it
     message1.id = 380;
     message1.data[0] = snapRes;
     CAN.send_message (&message1);
      
      snapTaken = 1;

      // display a wait message
      lcd.setCursor(0, 3);
      lcd.print ("Collecting");

      // and wait
      switch (snapRes) 
      {
        case 0:
          delay(500);
          break;
        case 1:
          delay(750);
          break;
        case 2:
          delay(3500);
          break;
        case 3:
          delay(5500);
          break;
      }

    }
  }
}

void checkButtonCmd()
{
  static int b1on = 0;
  static int b2on = 0;
  static long  btn1Time = 0;
  static long  btn2Time = 0;
  

  // button 1 is for activating the current selection
  if (digitalRead (A0))
  {
    // state is off
    b1on = 0;
  }
  else
  {
    if (!b1on && (millis() - btn1Time > 300))  // has to be on at least .1 sec
    {
      btn1Time = millis();

      switch (btnAction)
      {
      case 0:
        // controller cmd
        controllerAction();
        break;
        
      case 1:
        // log start/stop
 //       loggerAction();
        break;
        
      case 2:
        // dac reflector selection
        dacReflectorSet();
        break;
        
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
        parmAdjust (btnAction);
        break;

      case 9:
        // snapshot selection
        snapAct();
        break;
      
      case 10:
        // accel sim with spinner
        accelAct();
        break;
      } 
    }
  }
  
//  if (logActive)
//  {
//    LogSelectedData();
//  }
  
  // button 2 cycles through selectable actions
  if (digitalRead(A1))
  {
    // state is off
    b2on = 0;
  }
  else
  {
    // state is on
    if (!b2on)
    {
      if (millis() - btn2Time > 300)  // has to be on at least 1 sec
      {
        b2on = 1;
        btn2Time = millis();
        if (++btnAction == NUMBTNSEL) btnAction = 0;  // cycle
        if (btnAction >= 3 && btnAction <= 8)  // tuning params
        {
          pidDp = tpidDp; pidDi = tpidDi;
          pidQp = tpidQp; pidQi = tpidQi;
          pidSp = tpidSp; pidSi = tpidSi;
        }
      }
    }
  }
}


