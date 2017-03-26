/*

LIN Interface for Hella BS 200X Sensor - Batteriemonitor

This project uses ideas and code snipets from https://github.com/gandrewstone/LIN .

Copyright: Frank Schöniger, frank@schoeniger.info

Lizenz: Apache 2.0

Hardware Configuration:

LINBUS Breakout - skpang.co.uk

PIN LIN     |   PIN Arduino Nano
----------------------------------
GND         |  GND
CS          |  2
FAULT       |  VCC +5V
TxD         |  3
RxD         |  4
VCC         |  Vcc +5V

********************************************************************
Connector 2 on LIN Breakout:

UBat --> + Akku
LIN  --> IBS PIN 2
GND  --> GNC Connector Sensor not Akku !!
********************************************************************
Hella IBS 200X Sensor

PIN1: --> + Akku
PIN2: --> LIN Bus
********************************************************************
ST7735S Display:

PIN Display  |  PIN Arduin Nano
--------------------------------------
LED          |  VCC +5V
SCK          |  13
SDA          |  11
A0 (DC)      |  9
Reset RST    |  8
CS           |  10
GND          |  GND
VCC          |  VCC +5V

*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <SoftwareSerial.h>

#define TFT_PIN_CS   10 // Arduino-Pin an Display CS   
#define TFT_PIN_DC   9  // Arduino-Pin an DC
#define TFT_PIN_RST  8  // Arduino Reset-Pin

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_PIN_CS, TFT_PIN_DC, TFT_PIN_RST);  // Display library setup

// LIN serial Interface

int serSpeed = 19200;  // speed LIN
int breakDuration = 13; // number of bits break signal

int txPin1 = 3;        // TX Pin LIN serial
int rxPin1 = 4;        // RX Pin LIN serial
int linCSPin = 2;       // CS Pin

SoftwareSerial linSerial(rxPin1, txPin1); // RX, TX

// GUI Config

int xakk = 5;
int yakk = 20;
int xtex = 55;
int ytex = 5;
int xamp = 110;
int yamp = 13;


// Configuration

boolean outputSerial = true;    // true if json output to serial, false if only display
boolean outputLCD = true;       // if true LCD output
boolean debug = true;           // Debug output UART

// Global Variables

byte LinMessage[9] = {0};
byte LinMessageA[200] = {0};
boolean linSerialOn = 0;

void setup() {

  pinMode(linCSPin, OUTPUT);               // CS Signal LIN Tranceiver
  digitalWrite(linCSPin, HIGH);

  linSerial.begin(serSpeed);
  linSerialOn = 1;

  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  tft.setRotation(3);
  tft.fillScreen(ST7735_BLACK);
  init_screen();

}

void loop() {

  int soc;
  int soh;
  float Ubatt;
  float Ibatt;
  float Btemp;
  float AvCap;
  int remTime;


  LinMessageA[0] = 0x01;
  while (bitRead(LinMessageA[0], 0) > 0) {
    readFrame(0x27);
  }

  // Read Frame IBS_FRM 2 - Current Values

  readFrame(0x28);

  Ubatt = (float((LinMessageA[4] << 8) + LinMessageA[3])) / 1000;
  Ibatt = (float((long(LinMessageA[2]) << 16) + (long(LinMessageA[1]) << 8) + long(LinMessageA[0]) - 2000000L)) / 1000;
  Btemp = long(LinMessageA[5]) / 2 - 40;


  // Read Frame IBS_FRM 3 - Error Values

  readFrame(0x29);


  // Read Frame IBS_FRM 5

  readFrame(0x2B);


  soc = int(LinMessageA[0]) / 2;
  soh = int(LinMessageA[1]) / 2;

  // Read Frame IBS_FRM 6

  readFrame(0x2C);

  AvCap = (float((LinMessageA[3] << 8) + LinMessageA[2])) / 10;       //Dischargeable Capacity
  int Calib = bitRead(LinMessageA[5], 0);

  // Read Frame 4

  readFrame(0x2A);


  //soc = (AvCap/(80*soh/100))*100;         // Anzeige der eigentlichen Restkapazität im Battsymbol

  // Output json String to serial

  if (outputSerial) {
    Serial.begin(9600);
    delay(50);
    Serial.print("{\"current\":{\"ubat\":\"");
    Serial.print(Ubatt, 2);
    Serial.print("\",\"icurr\":\"");
    Serial.print(Ibatt, 3);
    Serial.print("\",\"soc\":\"");
    Serial.print(soc);
    Serial.print("\",\"time\":\"");
    Serial.print(remTime);
    Serial.print("\",\"avcap\":\"");
    Serial.print(AvCap, 1);
    Serial.print("\"},\"Akku\":{\"soh\":\"");
    Serial.print(soh);
    Serial.print("\",\"temp\":\"");
    Serial.print(Btemp);
    Serial.println("\"}}");
    Serial.end();
  }

  // Output LCD Display

  if (outputLCD) {

    drawAkku(soc, remTime, Calib);
    drawAmp(Ibatt);
    drawTxt(Ubatt, soh, AvCap, Btemp);

  }


  delay(2000);

}


// Read answer from Lin bus
void readFrame(byte mID) {

  memset(LinMessageA, 0, sizeof(LinMessageA));
  int ix = 0;
  byte linID = mID & 0x3F | addIDParity(mID);

  serialBreak();
  linSerial.flush();
  linSerial.write(0x55); // Sync
  linSerial.write(linID); // ID
  linSerial.flush();
  delay(800);
  if (linSerial.available()) {              // read serial
    while (linSerial.available()) {
      LinMessageA[ix] = linSerial.read();
      ix++;
      if (ix > 9) {
        break;
      }
    }
  }

  if (debug) {
    Serial.begin(9600);
    delay(50);
    Serial.print("ID: ");
    Serial.print(linID, HEX);
    Serial.print(" --> ");
    Serial.print(mID, HEX);
    Serial.print(": ");
    for (int ixx = 0; ixx < 9; ixx++) {
      Serial.print(LinMessageA[ixx], HEX);
      Serial.print(":");
    }
    Serial.println("  Lesen Ende");
    Serial.end();
  }

}

// Generate Break signal LOW on LIN bus
void serialBreak() {

  if (linSerialOn == 1) linSerial.end();
  pinMode(txPin1, OUTPUT);
  digitalWrite(txPin1, LOW); // send break
  delay(1000/ serSpeed * breakDuration); // duration break time pro bit in milli seconds * number of bit for break
  digitalWrite(txPin1, HIGH);
  delayMicroseconds(1000000 / serSpeed); // wait 1 bit
  linSerial.begin(serSpeed);
  linSerialOn = 1;
}

void sendMessage(byte mID, int nByte) {

  byte cksum = LINChecksum(nByte);
  byte linID = mID & 0x3F | addIDParity(mID);
  serialBreak();
  linSerial.write(0x55); // Sync
  linSerial.write(linID); // ID
  while (nByte-- > 0) linSerial.write(LinMessage[nByte]); // Message (array from 1..8)
  linSerial.write(cksum);
  linSerial.flush();
}



byte LINChecksum(int nByte) {

  uint16_t sum = 0;
  while (nByte-- > 0) sum += LinMessage[nByte];
  while (sum >> 8)
    sum = (sum & 255) + (sum >> 8);
  return (~sum);
}

byte addIDParity(byte linID) {
  byte p0 = bitRead(linID, 0)^bitRead(linID, 1)^bitRead(linID, 2)^bitRead(linID, 4);
  byte p1 = ~(bitRead(linID, 1)^bitRead(linID, 3)^bitRead(linID, 4)^bitRead(linID, 5));   // evtl. Klammer rum ???
  return ((p0 | (p1 << 1)) << 6);
}


void init_screen() {

  tft.fillScreen(ST7735_BLACK);

  // Akku
  int x = xakk; // 15
  int y = yakk; // 20

  tft.fillRect(x, y + 13, 40, 74, ST7735_WHITE);
  tft.fillRect(x + 10, y + 6, 20, 7, ST7735_WHITE);
  tft.fillRect(x + 15, y + 1, 10, 5, ST7735_WHITE);


  // Amperemeter
  x = xamp;
  y = yamp;

  tft.fillRect(x, y + 3, 10, 85, ST7735_WHITE);
  tft.drawLine(x + 10, y + 5, x + 20, y + 5, ST7735_WHITE);
  tft.drawLine(x + 10, y + 25, x + 20, y + 25, ST7735_WHITE);
  tft.drawLine(x + 10, y + 66, x + 20, y + 66, ST7735_WHITE);
  tft.drawLine(x + 10, y + 86, x + 20, y + 86, ST7735_WHITE);
  tft.drawLine(x + 10, y + 15, x + 15, y + 15, ST7735_WHITE);
  tft.drawLine(x + 10, y + 35, x + 15, y + 35, ST7735_WHITE);
  tft.drawLine(x + 10, y + 56, x + 15, y + 56, ST7735_WHITE);
  tft.drawLine(x + 10, y + 76, x + 15, y + 76, ST7735_WHITE);
  tft.drawLine(x + 10, y + 46, x + 25, y + 46, ST7735_WHITE);

  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setCursor(x + 30, y + 43);
  tft.print("0");

  // Werte
  x = xtex;
  y = ytex;

  tft.fillRect(x, y, 44, 24, ST7735_WHITE);
  tft.fillRect(x, y + 28, 44, 24, ST7735_WHITE);
  tft.fillRect(x, y + 56, 44, 24, ST7735_WHITE);
  tft.fillRect(x, y + 84, 44, 24, ST7735_WHITE);
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
  tft.setCursor(x + 9, y + 2);
  tft.print("UBat:");
  tft.setCursor(x + 12, y + 30);
  tft.print("SOH:");
  tft.setCursor(x + 4, y + 58);
  tft.print("Capac.");
  tft.setCursor(x + 7, y + 86);
  tft.print("Temp.");
}

void drawTxt(float u, int soh, float cap, float temp) {

  int x = xtex;
  int y = ytex;

  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
  tft.setCursor(x + 3, y + 13);
  tft.print(u, 2);
  tft.print("V");

  tft.setCursor(x + 12, y + 41);
  tft.print(soh);
  tft.print(" %");

  tft.setCursor(x + 4, y + 69);
  tft.print(cap, 1);
  tft.print("Ah");

  tft.setCursor(x + 4, y + 97);
  tft.print(temp, 1);
  tft.print(" C");

}

void drawAmp(float amp) {


  boolean laden = true;
  int x = xamp;
  int y = yamp;
  int ix;
  float strom;

  strom = amp;
  if (strom < 0) {
    strom = strom * -1;
    laden = false;
  }

  if (strom > 500) {
    return;
  }

  if (strom <= 2) {
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft.setCursor(x + 30, y + 20);
    tft.print("1  ");
    tft.setCursor(x + 30, y + 1);
    tft.print("2  ");
    tft.setCursor(x + 30, y + 62);
    tft.print("1  ");
    tft.setCursor(x + 30, y + 82);
    tft.print("2  ");
    ix = int(strom * 20);

    if (ix > 40) {
      ix = 40;
    }
    if (laden) {
      tft.fillRect(x, y + 3, 10, 85, ST7735_WHITE);
      tft.fillRect(x, y + 43 + ix, 10, 5, ST7735_GREEN);
    }  else {
      tft.fillRect(x, y + 3, 10, 85, ST7735_WHITE);
      tft.fillRect(x, y + 43 - ix, 10, 5, ST7735_RED);
    }
  } else if ((strom > 2) & (strom <= 20)) {
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft.setCursor(x + 30, y + 20);
    tft.print("10 ");
    tft.setCursor(x + 30, y + 1);
    tft.print("20 ");
    tft.setCursor(x + 30, y + 62);
    tft.print("10 ");
    tft.setCursor(x + 30, y + 82);
    tft.print("20 ");
    ix = int(strom * 2);
    if (ix > 40) {
      ix = 40;
    }
    if (laden) {
      tft.fillRect(x, y + 3, 10, 85, ST7735_WHITE);
      tft.fillRect(x, y + 43 + ix, 10, 5, ST7735_GREEN);
    }  else {
      tft.fillRect(x, y + 3, 10, 85, ST7735_WHITE);
      tft.fillRect(x, y + 43 - ix, 10, 5, ST7735_RED);
    }
  } else if (strom > 20) {
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft.setCursor(x + 30, y + 20);
    tft.print("100");
    tft.setCursor(x + 30, y + 1);
    tft.print("200");
    tft.setCursor(x + 30, y + 62);
    tft.print("100");
    tft.setCursor(x + 30, y + 82);
    tft.print("200");
    ix = int(strom * 0.2);
    if (ix > 40) {
      ix = 40;
    }
    if (laden) {
      tft.fillRect(x, y + 3, 10, 85, ST7735_WHITE);
      tft.fillRect(x, y + 43 + ix, 10, 5, ST7735_GREEN);
    }  else {
      tft.fillRect(x, y + 3, 10, 85, ST7735_WHITE);
      tft.fillRect(x, y + 43 - ix, 10, 5, ST7735_RED);
    }
  }

  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setCursor(x - 5, 112);
  tft.print(amp);
  tft.print(" A");

}


void drawAkku(int cap, int ti, int calib) {

  int x = xakk;
  int y = yakk;

  if (cap >= 90) {
    tft.fillRect(x + 4, y + 17, 32, 10, ST7735_GREEN);
    tft.fillRect(x + 4, y + 31, 32, 10, ST7735_GREEN);
    tft.fillRect(x + 4, y + 45, 32, 10, ST7735_GREEN);
    tft.fillRect(x + 4, y + 59, 32, 10, ST7735_GREEN);
    tft.fillRect(x + 4, y + 73, 32, 10, ST7735_GREEN);

  } else if (cap >= 70) {
    tft.fillRect(x + 4, y + 17, 32, 10, ST7735_WHITE);
    tft.fillRect(x + 4, y + 31, 32, 10, ST7735_GREEN);
    tft.fillRect(x + 4, y + 45, 32, 10, ST7735_GREEN);
    tft.fillRect(x + 4, y + 59, 32, 10, ST7735_GREEN);
    tft.fillRect(x + 4, y + 73, 32, 10, ST7735_GREEN);

  } else if (cap >= 50) {
    tft.fillRect(x + 4, y + 17, 32, 10, ST7735_WHITE);
    tft.fillRect(x + 4, y + 31, 32, 10, ST7735_WHITE);
    tft.fillRect(x + 4, y + 45, 32, 10, ST7735_YELLOW);
    tft.fillRect(x + 4, y + 59, 32, 10, ST7735_YELLOW);
    tft.fillRect(x + 4, y + 73, 32, 10, ST7735_YELLOW);

  } else if (cap >= 30) {
    tft.fillRect(x + 4, y + 17, 32, 10, ST7735_WHITE);
    tft.fillRect(x + 4, y + 31, 32, 10, ST7735_WHITE);
    tft.fillRect(x + 4, y + 45, 32, 10, ST7735_WHITE);
    tft.fillRect(x + 4, y + 59, 32, 10, ST7735_RED);
    tft.fillRect(x + 4, y + 73, 32, 10, ST7735_RED);

  } else {
    tft.fillRect(x + 4, y + 17, 32, 10, ST7735_WHITE);
    tft.fillRect(x + 4, y + 31, 32, 10, ST7735_WHITE);
    tft.fillRect(x + 4, y + 45, 32, 10, ST7735_WHITE);
    tft.fillRect(x + 4, y + 59, 32, 10, ST7735_WHITE);
    tft.fillRect(x + 4, y + 73, 32, 10, ST7735_RED);
  }

  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);

  if (calib == 0) {
    tft.setTextColor(ST7735_RED, ST7735_BLACK);
    tft.setCursor(x + 6, y + 92);
    tft.print(cap, 1);
    tft.print(" %");
  } else {
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft.setCursor(x + 6, y + 92);
    tft.print(cap, 1);
    tft.print(" %");
  }

  /* expected time - die passt aber noch nicht
    tft.setCursor(x + 4, 8);
    tft.print(ti);
    tft.print(" h    ");
  */
}


