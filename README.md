# LIN_Interface
Arduino LIN Interface

This Project will use an Arduino to communicate with an HELLA IBS 200X Batteriesensor. The current software version is able to read the most importand parameter from the sensor and print it as a json String over the serial interface or show it on a TFT Display.

The project uses a LIN breakout board from skpan electronics: http://skpang.co.uk/catalog/linbus-breakout-board-p-1417.html, and an ST7735S based 1,8" TFT display.

This project uses ideas and code snipets from: https://github.com/gandrewstone/LIN

The Hardware configuration is as follows.

*******************************************************************
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
