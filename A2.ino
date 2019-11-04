#include <Servo.h>
#include "DCSerial.h"

#include <string.h>
#include <Adafruit_MCP4725.h>
Adafruit_MCP4725 dac;

#define POLYNOMIAL 0xD8 // 11011

#define defaultFreq 1700  // dac speed (Hz)

#define SERVO_PIN 9

Servo servo;
unsigned servo_angle[] = {45, 90, 135};
char A2toPC2[1];
char PC2toA2[1];
long seq = 0;
long fcs;
long flag = B100111;
int state = 1;

int f[] = {50, 100, 150, 200};
int delays[4];
int Ncycle[] = {25, 50, 75, 100};
int j, l, data2bit;
long data;
const uint16_t S_DAC[4] = {2047, 4095, 2047, 0};
long timeOut;
long ctrl[3] = {B100, B101, B100}; //start, data2, ack
long data2;
bool setTimeOut = false;

#define debug 1

#define Ampmax  100
#define Ampmin  -60
#define errorThreshold 12 //50 //40
#define POLYNOMIAL 0xD8

long seq_next;
unsigned long stopTime;
int   vol, count = 0;
bool  Start = false;
int   maxx = 0, prev = 0;
bool  isUp = false;
int   freq = 0;

//-------------------CRC---------------------------//
#define POLYNOMIAL 0xD8 // 11011

int freqToData() {
  Serial.print("freq : ");
  Serial.println(freq);
  for (int i = 0; i < 9; i++) {
    if (freq < Ncycle[i] + errorThreshold && freq > Ncycle[i] - errorThreshold) {
      freq = 0;
      Start = false;
      Serial.print("\niiiii: ");
      Serial.println(i);
      return i;
    }
  }
  return -1;
}

int getDataFm() {
  while (1) {
                Serial.println(analogRead(A0)-analogRead(A1));
                Serial.print("500 ");
                Serial.print("150 ");
                Serial.print("-70 ");
                Serial.print("-500 ");

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

uint8_t crcNaive(uint8_t const message)
{
  uint8_t  remainder;
  remainder = message;
  for (uint8_t bit = 17; bit > 0; --bit)
  {
    if (remainder & 0x80)
    {
      remainder ^= POLYNOMIAL;
    }
    remainder = (remainder << 1);
  }
  // Serial.print("CRC: ");
  // Serial.println(remainder, BIN);
  return (remainder >> 4);

}
char Receiver() {
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
      if ((data & B1111) == crcNaive(checkCRC)) {
        if ((data >> 11)&B1 == 0)seq_next = 1;
        else seq_next = 0;
        //sendACK(seq_next);
        state = 0;
        return (data >> 4)&B1111111;
      }
    }
  }
//return (char) - 1;
}
//void sendACK(long numSeq) {
//  long frameACK = (flag << 12 | ((((numSeq << 7) | B0111100) << 4) | (crcNaive((numSeq << 7) | B0111100))));
//  sendFrame(frameACK);
//}

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

void Sender(char datain) {
  long data2 = datain;
  fcs = (seq << 7) | data2;
  //--------------------crc----------------
  uint8_t dd = crcNaive(fcs);
  //-----------------Sending---------------
  sendFrame(flag << 8 | ((fcs << 4) | dd));
  //Serial.println("\nend");
  //timeOut = micros() + 8000000;
  // setTimeOut = true;
  dac.setVoltage(0, false);        // for don't send
  delayMicroseconds(1000);

  state = 0;
  if (seq == 0)seq = 1;
  else seq = 0;
}

void setup() {
  Serial.begin(BAUD_RATE);
  servo.attach(SERVO_PIN);
  servo.write(servo_angle[1]);

  *A2toPC2 = CONN_START;
  Serial.write(A2toPC2, 1);

  /* test */
 // *A2toPC2 = 0x4C;  // 3pic
  *A2toPC2 = 0x5E;  // 1pic small
  //*A2toPC2 = 0x5E;  // 1pic small
  //*A2toPC2 = 0x5E;  // 1pic small
}
void sendACK(long numSeq) {
  long frameACK = (flag << 12 | ((((numSeq << 7) | B0111100) << 4) | (crcNaive((numSeq << 7) | B0111100))));
  sendFrame(frameACK);
}

void loop() {

  *A2toPC2 = 0x4C; //Receiver();
  //Serial.readBytes(A2toPC2, 1);
  char control = getControl(*A2toPC2);

  if (control == CONTROL_TREEPIC ||
      control == CONTROL_ONEPIC) {
 //   sendACK(seq_next);
    //while (!Serial.available()) {}
    Serial.write(A2toPC2, 1);

    unsigned turn3_tube;
    if (control == CONTROL_TREEPIC) {
      turn3_tube = 3;
    } else if (control == CONTROL_ONEPIC) {
      turn3_tube = 1;
    }

    for (int i = 0; i < turn3_tube; ++i) {
      Serial.readBytes(PC2toA2, 1);
      char control = getControl(*PC2toA2);
      if (control == CONTROL_SERVO) {
        char servo_ack = getAngle(*PC2toA2);
        servo.write(servo_angle[servo_ack]);
        delay(500);
        Serial.write(SERVO_ACK);
      }
    }

    Serial.readBytes(PC2toA2, 1);
    Sender(*PC2toA2) ;

  }
}
