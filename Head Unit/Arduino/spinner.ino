int rdataPos = 0;    // starting display position
static long lastEncoder = 0;
static long flastEncoder = 0;
extern int btnAction;    // ctrl strt/stop, log on/off, dac target  
extern int AccelVal;

void doSpin() {
  /* If pinA and pinB are both high or both low, it is spinning
   * forward. If they're different, it's going backward.
   *
   */
  if (digitalRead(A2) == digitalRead(3)) {
    encoder0Pos++;
  } else {
    encoder0Pos--;
  }

}

void parmInc (int Action)
{
  switch (Action)
  {
    case 3:
      pidDp++;
      break;
      
    case 4:
      pidDi++;
      break;
      
    case 5:
      pidQp++;
      break;
      
    case 6:
      pidQi++;
      break;

    case 7:
      pidSp++;
      break;
      
    case 8:
      if (pidSi < 99) pidSi++;
      break;
  }
}

void parmDec (int Action)
{
  switch (Action)
  {
    case 3:
      if (--pidDp < 0) pidDp = 0;
      break;
      
    case 4:
      if (--pidDi < 0) pidDi = 0;
      break;
      
    case 5:
      if (--pidQp < 0) pidQp = 0;
      break;
      
    case 6:
      if (--pidQi < 0) pidQi = 0;
      break;

    case 7:
      if (--pidSp < 0) pidSp = 0;
      break;
      
    case 8:
      if (--pidSi < 0) pidSi = 0;
      break;
  }
}

extern int snapRes;
void checkSpinner (void)
{
  if (btnAction > 2 && btnAction < 9)  // this is a tuning param
  {
    if (flastEncoder != (encoder0Pos/13))
      {
        if ((encoder0Pos/13) > flastEncoder)
        {
          parmInc (btnAction);
        }
        else
        {
          parmDec (btnAction);
        }
      }
      flastEncoder = (encoder0Pos/13);
  }
  else  // dac selection or data pos or snap res
  {
    if (lastEncoder != (encoder0Pos/71))
      {
        if ((encoder0Pos/71) > lastEncoder)
        {
          if (btnAction == 2)
          {
            dacRIndex++;
          }
          else if (btnAction == 9)
          {
            snapRes++;
          }
          else if (btnAction == 10)
          {
            accelVal += 10;
            if (accelVal > 4000) accelVal = 4000;
          }
          else
          {
            rdataPos++;
          }
        }
        else
        {
          if (btnAction == 2)
          {
            dacRIndex--;
          }
          else if (btnAction == 9)
          {
            snapRes--;
          }
          else if (btnAction == 10)
          {
            accelVal -= 10;
            if (accelVal < 0) accelVal = 0;
          }
          else
          {
            rdataPos--;
          }
      }
      if (dacRIndex > MAXDACR) dacRIndex = MAXDACR;
      if (dacRIndex < 0) dacRIndex = 0;
      if (rdataPos > 5) rdataPos = 5;
      if (rdataPos < 0) rdataPos = 0;
      if (snapRes > 3) snapRes = 3;
      if (snapRes < 0) snapRes = 0;
      lastEncoder = (encoder0Pos/71);
    }
  }
}


