

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"

#include <Wire.h>
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

String working;

// It was 106
#define _meterHeight 60
#define _meterWidth  60


void setup() {
  Serial.begin(9600);
  while(!Serial); 
  
  strip.begin();
  strip.show();
  
  tft.begin();

  // It was 3
  tft.setRotation(2);

  tft.fillScreen(ILI9340_BLACK);

  Wire.begin();

  if ( Ethernet.begin(mac) == 0 ) {
    tft.println("Failed to configure Ethernet using DHCP.");
    for(;;);
  }
  
  udp.begin(localPort);

  drawMeterFrame( 0, 0, "DW" );
  drawMeterFrame( _meterWidth, 0, "DA" );
  drawMeterFrame( 2*_meterWidth, 0, "BI" );
  drawMeterFrame( 3*_meterWidth, 0, "BI" );

  drawMeterFrame( 0, _meterHeight, "MW" );
  drawMeterFrame( _meterWidth, _meterHeight, "MA" );
  drawMeterFrame( 2*_meterWidth, _meterHeight, "WS" );
  drawMeterFrame( 3*_meterWidth, _meterHeight, "WS" );

  tft.setCursor(6, 123);
  tft.setTextColor(ILI9340_WHITE);
  tft.setTextSize(2);
  tft.print("2014-09-05 23:00");
}


void loop() {
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;     

  //Every 10 minutes, send a new request to the NTP server
  if ( (secsSince1900-secsLastSet > secsDelay) || secsLastSet == 0 ) {
    sendNTPpacket(timeServer); // send an NTP packet to a time server
  }
  
  if ( ( !client.connected() && ((secsSince1900-secsLastSet > 10 )) || secsLastSet == 0 ) ) {
      if (client.connect(ledserver, 58471)) {
        // Make a HTTP request:
        client.println("GET /LEDStatus HTTP/1.1");
        client.println("Host: iq-colo.sneaky.net");
        client.println("Connection: close");
        client.println();
      
        secsLastSet = secsSince1900;
        
        String received;

        for ( int c = 0 ; c < 8 ; c++ ) {
         strip.setPixelColor( c, 0, 0, 0); 
        }
        
        // If we have anything.. take it in and send it to serial.. for now.
        while(client.connected() || client.available()) {
          if ( client.available() ) {
            char c = client.read();
            //Serial.print(c);
            received += c;
            
            if ( c == '\n' ) {
             
             int equals = received.indexOf('=');
             
             if ( equals != -1 && received.length() < 5 && received.length() > 0 ) {
               
               String ledString = received.substring(0,equals);
               int l = ledString.toInt();
               
               String valueString = received.substring(equals+1);
               int v = valueString.toInt();
               
               if ( v == 0 ) strip.setPixelColor( l, 0, 255, 0 );
               if ( v == 1 ) strip.setPixelColor( l, 255, 255, 0 );
               if ( v == 2 ) strip.setPixelColor( l, 255, 0, 0 );
               if ( v == 3 ) strip.setPixelColor( l, 0, 0, 255 );

             }
             received = "";
             
            }
          }
        }
        client.stop();

      } 
      else {
        // kf you didn't get a connection to the server:
        Serial.println("connection failed");
      } 
  }


  if (  udp.parsePacket() ) {
   // We've received a packet, read the data from it
   udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

   //the timestamp starts at byte 40 of the received packet and is four bytes,
   // or two words, long. First, esxtract the two words:

   unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
   unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
   // combine the four bytes (two words) into a long integer
   // this is NTP time (seconds since Jan 1 1900):
   unsigned long secsNTP = highWord << 16 | lowWord;
   // Save off the last time we actually set the time.    
   secsLastSet = secsNTP - seventyYears;
   
   counter++;
   

  }
  
  // Rotation - testing - displayTimeSmall(now);

  // delay(100);
  
  drawMeter( 0, 0, oldTesting, 270-oldTesting, -1, testing, 270-testing, 30, "10", "100", "1k" );
  drawMeter( _meterWidth, 0, -1, oldTesting, 40, 20, testing, 40, "10", "100", "1k" );
  drawMeter( 2*_meterWidth, 0, -1, -1, oldTesting, 50, 75, testing, "10", "100", "1k" );

  drawMeter( 0, _meterHeight, -1, oldTesting, -1, 100, testing, -1, "10", "100", "1k" );
  float ram = 2048-getFreeRam();
  drawMeter( _meterWidth, _meterHeight, oldTesting, ram*270/2048, -1, testing, -1, -1, "10", "%RAM", "1k" );
  drawMeter( 2*_meterWidth, _meterHeight, 0, -1, -1, ram*270/2048, -1, -1, "%RAM", "100", "1k" );
  
  
  //strip.setPixelColor( 0, 255, 0, 0 );
  //strip.setPixelColor( 1, 0, 255, 0 );
  //strip.setPixelColor( 2, 0, 0, 255 );
  //strip.setPixelColor( 3, 255, 255, 0);
  //strip.setPixelColor( 4, 255, 255, 255);
  //strip.setPixelColor( 5, 200, 0, 0);
  //strip.setPixelColor( 6, 150, 0, 0);
  //strip.setPixelColor( 7, 100, 0, 0);
  
  strip.setBrightness(5);
//  if ( testing < 20 ) { strip.setBrightness(255); }
  
  strip.show();


  
    working = "Updating";
  drawIndicator( 0, 0, working, 2 );
  
  for (int column = 0; column < 3; column++ ) {
   for (int row = 0; row < 12; row++) {
     
     String name;
     String colString = "Col:";
     String rowString = " Row:";
     name = colString + column + rowString + row;
     
     uint16_t color = ILI9340_WHITE;
     
     if (!( column == 0 && row == 0 )) drawIndicator( column, row, name, state[row][column] );
    
   } 
  }
 
 state[random(0,12)][random(0,3)] = random(1,4);

 if ( random(0,20) == 0 ) {
    for (int column = 0; column < 3; column++ ) {
      for (int row = 0; row < 10; row++) {
       // It's backward!  (column is row... LAZY!)
       state[row][column] = 0;   
      }
    }
 }

   

 drawIndicator( 0, 0, working, 0 );
  
  
  












  
  oldTesting = testing;
  testing++;
  if ( testing > 270 ) testing = 0;
  
  delay(10000);
}




void drawMeterFrame ( int x, int y, char* title ) {

 tft.fillRect(x, y, _meterWidth, _meterHeight, ILI9340_BLUE );
 tft.drawRect(x, y, _meterWidth, _meterHeight, ILI9340_WHITE );
 tft.fillCircle( x + _meterWidth/2, y + _meterHeight/2, _meterWidth*.48, ILI9340_BLACK );  // This one may throw off the radius.. if it's not "square"!
 tft.drawCircle( x + _meterWidth/2, y + _meterHeight/2, _meterWidth*.48, ILI9340_WHITE );  // This one may throw off the radius.. if it's not "square"!
 tft.fillRect(x+_meterWidth/2+2, y+_meterHeight/2+2, _meterWidth/2-2, _meterHeight/2-2, ILI9340_BLUE );
 tft.drawRect(x+_meterWidth/2+2, y+_meterHeight/2+2, _meterWidth/2-2, _meterHeight/2-2, ILI9340_WHITE );
 
 tft.setCursor( x+_meterWidth/2+13, y+_meterHeight/2+7);
 tft.setTextColor(ILI9340_WHITE);
 tft.setTextSize(1);
 tft.print( title );
 
  
  
  
  
  
  
  
  
  
  
  
}


void drawMeter( int x, int y, int oldRed, int oldGreen, int oldYellow, int red, int green, int yellow, char* scaleRed, char* scaleGreen, char* scaleYellow ) {
 
 float alfa1;
  
 // x is the horizontal upper left corner
 // y is the verticle upper left corner 

 tft.fillCircle( x + _meterWidth/2, y + _meterHeight/2, 3, ILI9340_WHITE );

  if ( oldGreen != green ) {
    drawMeterLine( x, y, oldGreen, .44, ILI9340_BLACK );
    drawMeterLine( x, y, green, .44, ILI9340_GREEN );
    tft.setCursor( x+_meterWidth/2+16, y+_meterHeight/2+26);
    tft.setTextColor(ILI9340_GREEN, ILI9340_BLUE );
    tft.setTextSize(1);
    //tft.print( scaleGreen );
    //tft.print( "   " );
  }
  if ( oldRed != red ) {
    drawMeterLine( x, y, oldRed, .35, ILI9340_BLACK );
    drawMeterLine( x, y, red, .35, ILI9340_RED );
    tft.setCursor( x+_meterWidth/2+16, y+_meterHeight/2+26+8);
    tft.setTextColor(ILI9340_RED, ILI9340_BLUE );
    tft.setTextSize(1);
    //tft.print( scaleRed );
    //tft.print( "   " );
  }
  if ( oldYellow != yellow ) {
    drawMeterLine( x, y, oldYellow, .25, ILI9340_BLACK );
    drawMeterLine( x, y, yellow, .25, ILI9340_YELLOW );
    tft.setCursor( x+_meterWidth/2+16, y+_meterHeight/2+26+8+8);
    tft.setTextColor(ILI9340_YELLOW, ILI9340_BLUE );
    tft.setTextSize(1);
    //tft.print( scaleYellow );
    //tft.print( "   " );
  }

}

void drawMeterLine( int x, int y, int number, float length, uint16_t color ) {
  float alfa1;

  if ( number > 270 ) number = 270;
  if ( number < 0 ) number = 0;

  alfa1 = 2*3.14*(360-number)/360; // convert ungle from degree in radian  
  tft.drawLine(x + _meterWidth/2,y + _meterHeight/2, x+(_meterWidth/2)+(_meterWidth*length)*sin(alfa1), y+(_meterHeight/2)+(_meterHeight*length)*cos(alfa1), color); 
}

  



// send an NTP request to the time server at the given address 
unsigned long sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE); 
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp: 		   
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer,NTP_PACKET_SIZE);
  udp.endPacket(); 
}

int getFreeRam(void)
{
  extern int  __bss_end;
  extern int  *__brkval;
  int free_memory;
  if((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval);
  }

  return free_memory;
}

#define _indicatorWidth 80
#define _indicatorHeight 15
#define _offsetWidth 0
#define _offsetHeight 140

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
  

  tft.drawRect ( x * _indicatorWidth + _offsetWidth, y * _indicatorHeight + _offsetHeight, _indicatorWidth , _indicatorHeight, ILI9340_WHITE );
  tft.fillRect ( x * _indicatorWidth+1 + _offsetWidth, y * _indicatorHeight+1 + _offsetHeight, _indicatorWidth-2, _indicatorHeight-2, color );
  tft.setCursor(  x * _indicatorWidth + _offsetWidth + _indicatorWidth * .5+1 - 6* title.length() * .5 , y * _indicatorHeight + _offsetHeight + _indicatorHeight/2 - 4 );
  tft.setTextSize(1);
  tft.setTextColor ( ILI9340_BLACK );
  tft.print( title );
  
  oldState[x][y] = value;

}


