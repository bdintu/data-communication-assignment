#include <string.h>
#include <Adafruit_MCP4725.h>
Adafruit_MCP4725 dac;

#define POLYNOMIAL 0xD8 // 11011

#define defaultFreq 1700  // dac speed (Hz)
int f[] = {50, 100, 150, 200};
int delays[4];
int Ncycle[] = {25, 50, 75, 100};
int j, l, data2bit, state = 0;
long data;
const uint16_t S_DAC[4] = {2047, 4095, 2047, 0};
long timeOut;
long flag = B100111;
long seq = 0;
long ctrl[3] = {B100, B101, B100}; //start, data2, ack
long data2;
long fcs;
bool setTimeOut = false;
int count3pic = 0;

#define debug 1

#define Ampmax  100
#define Ampmin  -60
#define errorThreshold 12 //50 //40
#define POLYNOMIAL 0xD8

unsigned long stopTime;
int   vol, count = 0;
bool  Start = false;
int   maxx = 0, prev = 0;
bool  isUp = false;
int   freq = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  dac.begin(0x64);        //set to default
  /* calculate sampling period (time) of sine[4] for each FSK Frequency */
  for (int i = 0; i < 4; i++) {
    delays[i] = (1000000 / f[i] - 1000000 / defaultFreq) / 4;
    //Serial.println(delays[i]);
  }
  Serial.flush();         // for clear buffer serial
  dac.setVoltage(0, false);        // for don't send
}

void loop() {
  // put your main code here, to run repeatedly:
  //if (Serial.available() > 0) {
 // if (!setTimeOut) {
    if (state == 0)Sender();
    else if (state == 1) Receiver();
 // } else {
 //   if (micros() > timeOut) {
  //    sendFrame(data2);
  //    Serial.println();
  //    state = 1;
  //    timeOut = micros() + 20000000;
  //  }
  }

void Sender() {
  fcs = seq;
  Serial.println("Enter control \n\t(0)Collect data2 \n\t(1)Send angle/size : ");
  while (Serial.available() == 0) {}
  char ct = Serial.read();
  Serial.println(ct);
  if (ct == '0') {
    fcs = (fcs << 3) | ctrl[0];
    data2 = B0000;
  }
  else if (ct == '1') {
    fcs = (fcs << 3) | ctrl[1];
    Serial.println("Size \n\t(0)Small \n\t(1)Medium \n\t(2)Large: ");
    while (Serial.available() == 0) {}
    char siz = Serial.read();
    Serial.println(siz);
    if (siz == '0')
      data2 = B0001;
    else if (siz == '1')
      data2 =  B0010;
    else if (siz == '2')
      data2 = B0011;
  }

  Serial.print("Data :");
  Serial.println(data2, BIN);
  fcs = (fcs << 4) | data2;
  Serial.print("FCS :");
  Serial.println(fcs, BIN);

  //--------------------crc----------------
  uint8_t dd = crcNaive(fcs);
  data2 = (flag << 12) | (fcs << 4) | crcNaive(fcs);
  Serial.println(data2, BIN);

  //-----------------Sending---------------
  sendFrame(data2);
  Serial.println("\nend");
  //timeOut = micros() + 20000000;
  //setTimeOut = true;
  dac.setVoltage(0, false);        // for don't send
  delayMicroseconds(10000);

  state = 1;
  if (seq == 0)seq = 1;
  else seq = 0;
}
//-------------------CRC---------------------------//
#define POLYNOMIAL 0xD8 // 11011

uint8_t crcNaive(uint8_t const message)
{
  uint8_t  remainder;
  remainder = message;
  for (uint8_t bit = 8; bit > 0; --bit)
  {
    if (remainder & 0x80)
    {
      remainder ^= POLYNOMIAL;
    }
    remainder = (remainder << 1);
  }
  return (remainder >> 4);

}
void  Receiver() {
  int data = 0;
  for (int i = 0; i < 9; i++) {
    int res = getDataFm();
    if (debug) {
      //Serial.print("Recieved Data : ");
      //Serial.println(res);
    }
    if (res != -1) {
      data |= res << i * 2;
      uint8_t checkCRC = (data >> 4)&B11111111;
      long seq_next;
      if ((data & B1111) == crcNaive(checkCRC)) {
        if ((data >> 11)&B1 == 0)seq_next = 1;
        else seq_next = 0;
//        if ((data >> 8)&B111 == B011) {
//          Serial.print("ACK ");
//          Serial.println(seq);
//          //setTimeOut = false;
//        }
//        else 
        if ((data >> 8)&B111 == B100) {
          //sendACK(seq_next);
          if (count3pic == 3) {
            state = 0;
            count3pic = 0;
          }
          else {
            if ((data >> 4)&B11 == B01)Serial.print("Size : small   ");
            else if ((data >> 4)&B11 == B10)Serial.print("Size : medium   ");
            else if ((data >> 4)&B11 == B11)Serial.print("Size : large   ");
            if ((data >> 6)&B11 == B00)Serial.println("Angle : -45 deg");
            else if ((data >> 6)&B11 == B01)Serial.println("Angle : 0 deg");
            else if ((data >> 6)&B11 == B10)Serial.println("Angle : 45 deg");
            count3pic++;
          }
        }
        else if ((data >> 8)&B111 == B101) {
         // sendACK(seq_next);
          int pixel = ((data >> 4)&B1111) * 4800;
          Serial.print("Pixel = ");
          Serial.println(pixel);
          state = 0;
        }
        Serial.println();
      } else {
        Serial.println("error");
       // sendACK(seq);
      }

    }
  }
}
void sendACK(long numSeq) {
  long frameACK = (flag << 12 | ((((numSeq << 7) | B0111100) << 4) | (crcNaive((numSeq << 7) | B0111100))));
  sendFrame(frameACK);
}

void sendFrame(long frame) {
  for (int numBit = 0; numBit < 17; numBit += 2) {
    data2bit = frame & 3;
    Serial.print(data2bit);
    for (j = 0; j < Ncycle[data2bit]; j++) {
      for (l = 0; l < 4; l++) {
        dac.setVoltage(S_DAC[l], false);
        //Serial.println(S_DAC[l]);
        delayMicroseconds(delays[data2bit]);
      }
    }
    frame >>= 2;
  }
}
int freqToData() {
  Serial.print("freq : ");
  Serial.println(freq);
  for (int i = 0; i < 4; i++) {
    if (freq < Ncycle[i] + errorThreshold && freq > Ncycle[i] - errorThreshold) {
      //      count4++;
      //      Serial.print(i);
      //      if (count4 == 4){
      //        Serial.println();
      //        count4 = 0;
      //      }
      freq = 0;
      Start = false;
      return i;
    }
  }
  return -1;
}

int getDataFm() {
  while (1) {
    //        Serial.println(analogRead(A0)-analogRead(A1));
    //        Serial.print("500 ");
    //        Serial.print("150 ");
    //        Serial.print("-70 ");
    //        Serial.print("-500 ");
    //        continue;
    vol = analogRead(A0) - analogRead(A1);
    if (Start == false) {
      if (vol > Ampmax) {
        Start = true;
        stopTime = micros() + 500000;
      }
      //Serial.println(count);
      count = 0;
    }
    else {
      if (micros() > stopTime) {
        isUp = false;
        Start = false;
        int res = freqToData();
        Serial.print("res : ");
        Serial.println(res);
        return res;
      }
      if (isUp) {
        if (vol < Ampmin) {
          isUp = false;
          freq++;
        }

      } else {
        if (vol > Ampmax) {
          isUp = true;
        }
      }
    }
  }
}
