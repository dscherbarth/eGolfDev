int upd = 0;
void init_SPI_CS(void)
{
  pinMode(CANCS,OUTPUT);    /* Initialize the CAN bus card SPI CS Pin*/
  digitalWrite(CANCS, HIGH); /* Turns the CAN bus card communication off*/
}  

void canUpd()
{
  getCANData();
  
  upd++;
}

tCAN message1;

unsigned long timestamp=0;
extern int dataArray [];    // data as received
extern int dataItems;         // number of items in the array

int tp0 = 123, tp1 = 456, tp2 = 789;
int tpa = 123, tpb = 456, tpc = 789;
int h0 = 423, h1 = 856;
int snapCnt = 0;

void getCANData(void)
{
  char temp[30];
  
  
  // try to get everything pending
  while (CAN.get_message(&message1, &timestamp))
      {
      numDataRcved = 5;
        if ((message1.id - 256) >= 0 && (message1.id - 256) < 13)
        {
          dataMsg = 1;
  
          dataArray [ message1.id - 256 ] = message1.data[0];
          dataArray [ message1.id - 256 ] += ((uint32_t)message1.data[1] << 8L);
          dataArray [ message1.id - 256 ] += ((uint32_t)message1.data[2] << 16L);
          dataArray [ message1.id - 256 ] += ((uint32_t)message1.data[3] << 24L);
          dataItems = 12;
    
          // update state
          if ((message1.id - 256) == 10 && dataArray[10] == RUNNING)
          {
            startState = 1;  // running
          }
          if ((message1.id - 256) == 10 && dataArray[10] == STOPPED)
          {
            startState = 0;  // stopped
          }
          // update fault state
          if (dataArray[8] != 0)
          {
            startState = 2;
          }
        }
        else if (message1.id - 256 == PIDD)
        {
          tpidDp = message1.data[0];
          tpidDp += ((uint32_t)message1.data[1] << 8L);
          tpidDi = message1.data[4];
          tpidDi += ((uint32_t)message1.data[5] << 8L);
        }
        else if (message1.id - 256 == PIDQ)
        {
          tpidQp = message1.data[0];
          tpidQp += ((uint32_t)message1.data[1] << 8L);
          tpidQi = message1.data[4];
          tpidQi += ((uint32_t)message1.data[5] << 8L);
        }
        else if (message1.id - 256 == PIDS)
        {
          tpidSp = message1.data[0];
          tpidSp += ((uint32_t)message1.data[1] << 8L);
          tpidSi = message1.data[4];
          tpidSi += ((uint32_t)message1.data[5] << 8L);
        }
        else if (message1.id - 256 == MSGID)  // this is a text message
        {
          dataMsg = 0;
          msgindex = message1.data[0];
          if (msgindex < 0 || msgindex > MSGBLANK)
          {
            msgindex = MSGBLANK;
          }
        }
        else if (message1.id == 406)
        {
          // snap header
          h0 = message1.data[0];
          h0 += ((uint32_t)message1.data[1] << 8L);
          h1 = message1.data[4];
          h1 += ((uint32_t)message1.data[5] << 8L);

          // save header data to the file
          SnapData (h0, h1, 777, 0, 0, 0);

          snapCnt = 0;
          
          // ask for the next one
          message1.id = 381;
          CAN.send_message (&message1);
        }
        else if (message1.id == 407)
        {
          // snap data
          tp0 = message1.data[0];
          tp0 += ((uint32_t)message1.data[1] << 8L);
          tp1 = message1.data[2];
          tp1 += ((uint32_t)message1.data[3] << 8L);
          tp2 = message1.data[4];
          tp2 += ((uint32_t)message1.data[5] << 8L);

        }
        else if (message1.id == 408)
        {
          // snap data
          tpa = message1.data[0];
          tpa += ((uint32_t)message1.data[1] << 8L);
          tpb = message1.data[2];
          tpb += ((uint32_t)message1.data[3] << 8L);
          tpc = message1.data[4];
          tpc += ((uint32_t)message1.data[5] << 8L);

          // save to the file
          SnapData (tp0, tp1, tp2, tpa, tpb, tpc);
          
          snapCnt++;
          
          // ask for the next one
          message1.id = 381;
          CAN.send_message (&message1);
        }
        else if (message1.id == 409)
        {
          // snap done
          SnapStop();
        }
      }
}


