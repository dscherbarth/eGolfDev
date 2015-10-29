// display strings
char *rdataHdr[] = {"ROTR", "SRPM", "ACCL", "BVLT", "BAMP", "RAWF", "RAWV", "TEMP"};
extern int rdataPos;    // starting display position

void backlightWhite()
{
  digitalWrite (0, 0);
}

void backlightRed()
{
  digitalWrite (0, 1);
}

void backlightYellow()
{
  digitalWrite (0, 0);
}

void dispRemoteData ()
{
  char temp[21];
  int i;
  
  
  if (numDataRcved > 0)
  {
    // erase dynamic display
    // display data and appropriate headers
    if (dataMsg)
    {
      for (i=0; i<3; i++)
      {
        sprintf(temp, "%04d", dataArray[i+rdataPos]);
        lcd.setCursor (i*5, 1);
        lcd.print (rdataHdr[i+rdataPos]);
        lcd.setCursor (i*5, 2);
        lcd.print (temp);
      }
  
      // display dac reflection choice
      lcd.setCursor (16, 0);
      if (dataArray[11] >= 0)
      {
        strcpy_P(tbuf, (char *)pgm_read_word(&(dacrArray[dataArray[11]]))); // strings are in flash
        lcd.print (tbuf);
      }
      else
      {
        lcd.print ("----");
      }
      
      
      // display fault data
      switch (dataArray[8])  // fault state
      {
        case 0:
          switch (dataArray[10])  // run state
          {
          case STARTING:
            sprintf (temp, "RR");    // run request
            break;
          
          case RUNNING:
            sprintf (temp, "RU");    // running
            break;
          
          case STOPPING:
            sprintf (temp, "SR");    // stop request
            break;
          
          case STOPPED:
            sprintf (temp, "SP");    // stopped
            break;
          
          default:
            sprintf (temp, "--");
  //          sprintf (tempf, "%02d", dataItems);
            break;
          }
          backlightWhite();
          break;
          
        case 1:
          sprintf(temp, "OT");    // over temp
          backlightRed();
          break;
        case 2:
          sprintf(temp, "OV");    // over voltage
          backlightRed();
          break;
        case 3:
          sprintf(temp, "UV");    // under voltage
          backlightRed();
          break;
        case 4:
          sprintf(temp, "OC");    // over current
          backlightRed();
          break;
        case 5:
          sprintf(temp, "FA");    // fast abort
          backlightRed();
          break;
      }
      
      lcd.setCursor(18,2);
      lcd.print(temp);
    
      // update battery percent display
      sprintf (temp, "%03d%%", dataArray[9]);
//      sprintf (temp, "%03d%%", upd);
      lcd.setCursor(6,0);
      lcd.print(temp);
    }
    else
    {
      lcd.setCursor(0,2);
      if (msgindex == MSGBLANK)
      {
        lcd.print(blank);
      }
      else
      {
        lcd.print(blank);
        lcd.setCursor(0,2);
        strcpy_P(tbuf, (char *)pgm_read_word(&(messageArray[msgindex]))); // messages are in flash
        lcd.print(tbuf);
      }
    }
  }
  else  // no remote data
  {
    if (dataMsg)
    {
      // blank the fields
      lcd.setCursor (0, 2);
      lcd.print ("                    ");
      lcd.setCursor(6,0);
      lcd.print("    ");
    }
  }
}

