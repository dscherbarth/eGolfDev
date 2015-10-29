static long logStartTime;
static int logcount = 0;
static int snapcount = 0;

//static File LogData;
static File SnapDataF;

#ifdef log
void LogSelectedDataStart()
{
  char startname[30] = "logdata.csv";
  
  // name is limited to 8.3 length format...
  sprintf (startname, "LOG%d.csv", logcount);
  logcount++;

  CAN.Disable();
  
  SD.remove (startname);
  LogData = SD.open (startname, FILE_WRITE);
  logStartTime = (long)millis();

  CAN.Enable();

}

void LogSelectedData()
{
  char temp[100];
  int i, num;


  sprintf(temp, "%08ld, ", (long)millis() - logStartTime);
  for (i=0; i<dataItems; i++)
  {
    sprintf(&temp[i*6 + 10], "%04d, ", dataArray[i]);
  }
  sprintf(&temp[i*6 + 8], "\n");
  CAN.Disable();

  num = LogData.write ((const uint8_t *)temp, i*6+10);
  
  CAN.Enable();

}

void LogSelectedDataStop()
{
  CAN.Disable();
  LogData.flush();
  LogData.close();
  CAN.Enable();
}
#endif
extern int snapReady;

void SnapStart()
{
  char startname[30];
  char *rv;
  
  // name is limited to 8.3 length format...
  rv = "xx";
  if (snapRes == 0) rv = "10";
  if (snapRes == 1) rv = "5";
  if (snapRes == 2) rv = "1";
  if (snapRes == 3) rv = "D5";
  sprintf (startname, "SNP%sk%d.csv", rv, snapcount);
  snapcount++;

  CAN.Disable();
  
  SD.remove (startname);
  SnapDataF = SD.open (startname, FILE_WRITE);

  CAN.Enable();
  snapReady = 0;
}

void SnapData(uint16_t i0, uint16_t i1, uint16_t pos, uint16_t a, uint16_t b, uint16_t c)
{
  char temp[50];
  int i, num;


  sprintf(temp, "%d, %d, %d, %d, %d, %d\n", i0, i1, pos, a, b, c);
  CAN.Disable();

  num = SnapDataF.write ((const uint8_t *)temp, strlen(temp));
  
  CAN.Enable();

}

void SnapStop()
{
  CAN.Disable();
  SnapDataF.flush();
  SnapDataF.close();
  CAN.Enable();
  snapReady = 1;
}


