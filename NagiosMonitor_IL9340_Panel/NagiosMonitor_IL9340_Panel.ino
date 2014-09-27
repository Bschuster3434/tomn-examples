
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"

#include <Wire.h>
#include<RTClib.h>
#include <Adafruit_NeoPixel.h>

#define _sclk 13
#define _miso 12
#define _mosi 11
#define _cs 3
#define _dc 9
#define _rst 8

Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _rst);
#define NEOPIN 4
Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, NEOPIN, NEO_GRB + NEO_KHZ800);


RTC_DS1307 RTC;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

EthernetUDP udp;
EthernetClient client;

char ledserver[] = "iq-colo.sneaky.net";

// HACK!
// char server[] = "pool.ntp.org";
IPAddress timeServer(209,43,20,114); // time-a.timefreq.bldrdoc.gov NTP server

unsigned int localPort = 8888;      // local port to listen for UDP packets
const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 
unsigned long secsSince1900;
unsigned long secsLastSet;
unsigned long secsLastLED;
unsigned long secsDelay = 600;
int counter;
int testing=0;
int oldTesting=360;
unsigned long offset = -14400;

int state[16][4] = {
 { 0, 0, 0, 0 }, 
 { 0, 0, 0, 0 }, 
 { 0, 0, 0, 0 }, 
 { 0, 0, 0, 0 }, 
 { 0, 0, 0, 0 }, 
 { 0, 0, 0, 0 }, 
 { 0, 0, 0, 0 }, 
 { 0, 0, 0, 0 }, 
 { 0, 0, 0, 0 }, 
 { 0, 0, 0, 0 }, 
 { 0, 0, 0, 0 }, 
 { 0, 0, 0, 0 }, 
 { 0, 0, 0, 0 }, 
 { 0, 0, 0, 0 }, 
 { 0, 0, 0, 0 }, 
 { 0, 0, 0, 0 }, 
};

int oldState[4][16] = {
 { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}, 
 { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}, 
 { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}, 
 { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}, 
};

void setup() {
  Serial.begin(9600);
  while(!Serial); 
  
  strip.begin();
  strip.show();
  
  tft.begin();

  tft.setRotation(3);

  tft.fillScreen(ILI9340_BLACK);

  Wire.begin();
  RTC.begin();
  
  if ( !RTC.isrunning()) {
    tft.println("RTC is NOT running!"); 
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  if ( Ethernet.begin(mac) == 0 ) {
    tft.println("Failed to configure Ethernet using DHCP.");
    for(;;);
  }
  
  udp.begin(localPort);

//  drawIndicator( 0, 0, "0x0", ILI9340_MAGENTA );
//  drawIndicator( 1, 0, "1x0 1", ILI9340_GREEN );
//  drawIndicator( 2, 0, "2x0 12", ILI9340_RED );
//  drawIndicator( 3, 0, "3x0 123", ILI9340_GREEN );

//  drawIndicator( 0, 1, "0x1 1234", ILI9340_GREEN );
//  drawIndicator( 1, 1, "1x1 12345", ILI9340_YELLOW );
//  drawIndicator( 2, 1, "2x1", ILI9340_GREEN );
//  drawIndicator( 3, 1, "3x1", ILI9340_GREEN );

//  drawIndicator( 0, 2, "0x2", ILI9340_GREEN );
//  drawIndicator( 1, 2, "1x2", ILI9340_GREEN );
//  drawIndicator( 2, 2, "2x2", ILI9340_BLUE );
//  drawIndicator( 3, 2, "3x2", ILI9340_GREEN );

//  drawIndicator( 0, 3, "0x3", ILI9340_CYAN );
//  drawIndicator( 1, 3, "1x3", ILI9340_GREEN );
//  drawIndicator( 2, 3, "2x3", ILI9340_RED );
//  drawIndicator( 3, 3, "3x3", ILI9340_GREEN );

//  drawIndicator( 0, 6, "prdzacaadbc01", ILI9340_GREEN );

//  drawIndicator( 0, 15, "stgzaoraetl01", ILI9340_GREEN );

}

String working;

void loop() {
  
  working = "Updating";
  drawIndicator( 0, 0, working, 2 );
  
  for (int column = 0; column < 4; column++ ) {
   for (int row = 0; row < 16; row++) {
     
     String name;
     String colString = "Col:";
     String rowString = " Row:";
     name = colString + column + rowString + row;
     
     uint16_t color = ILI9340_WHITE;
     
     if (!( column == 0 && row == 0 )) drawIndicator( column, row, name, state[row][column] );
    
   } 
  }
 
 state[random(0,16)][random(0,4)] = random(1,4);

 if ( random(0,20) == 0 ) {
    for (int column = 0; column < 4; column++ ) {
      for (int row = 0; row < 16; row++) {
       // It's backward!  (column is row... LAZY!)
       state[row][column] = 0;   
      }
    }
 }

   

 drawIndicator( 0, 0, working, 0 );
 
 delay(5000);
}


#define _indicatorWidth 80
#define _indicatorHeight 15

// X = horiz, Y = vert positions

void drawIndicator ( int x, int y, String title, int value ) {
  
  uint16_t color = ILI9340_WHITE;
  
  switch ( value ) {
   case 0:
    color = ILI9340_GREEN;
    break;
   case 1:
    color = ILI9340_YELLOW;
    break;
   case 2:
    color = ILI9340_RED;
    break;
   case 3:
    color = ILI9340_CYAN;
    break;
   default:
    color = ILI9340_BLACK;
  }

  if ( oldState[x][y] == value ) return;
  
  Serial.print("Running ");
  Serial.print(x);
  Serial.print(" ");
  Serial.println(y);
  

  tft.drawRect ( x * _indicatorWidth, y * _indicatorHeight, _indicatorWidth, _indicatorHeight, ILI9340_WHITE );
  tft.fillRect ( x * _indicatorWidth+1, y * _indicatorHeight+1, _indicatorWidth-2, _indicatorHeight-2, color );
  tft.setCursor(  x * _indicatorWidth + _indicatorWidth * .5+1 - 6* title.length() * .5 , y * _indicatorHeight + _indicatorHeight/2 - 4 );
  tft.setTextColor ( ILI9340_BLACK );
  tft.print( title );
  
  oldState[x][y] = value;

}



