

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

  drawMeterFrame( 0, 0, "DW" );
  drawMeterFrame( 106, 0, "DA" );
  drawMeterFrame( 212, 0, "BI" );

  drawMeterFrame( 0, 106, "MW" );
  drawMeterFrame( 106, 106, "MA" );
  drawMeterFrame( 212, 106, "WS" );

}


void loop() {
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;     

  //Every 10 minutes, send a new request to the NTP server
  if ( (secsSince1900-secsLastSet > secsDelay) || secsLastSet == 0 ) {
    sendNTPpacket(timeServer); // send an NTP packet to a time server
  }
  
  if ( ( !client.connected() && ((secsSince1900-secsLastSet > 10 )) || secsLastSet == 0 ) ) {
    Serial.print("Attempting to connect to the server");
      if (client.connect(ledserver, 58471)) {
        Serial.println("connected");
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
             
             Serial.print("Got new Line:");
             Serial.print(received);
             
             int equals = received.indexOf('=');
             
             if ( equals != -1 && received.length() < 5 && received.length() > 0 ) {
               
               String ledString = received.substring(0,equals);
               int l = ledString.toInt();
               Serial.print("LED:");
               Serial.println( l );
               
               String valueString = received.substring(equals+1);
               int v = valueString.toInt();
               Serial.print("Value:");
               Serial.println( v );
               
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
   
   // Every valid NTP packet - set the RTC!
   RTC.adjust(DateTime(secsNTP - seventyYears));

  }
  
  DateTime now = RTC.now();
  // displayTime( now + offset );
  // displayTimeSmall( now );
  secsSince1900 = now.secondstime() + 946684800;
  displayTimeSmall(now);

  // delay(100);
  
  drawMeter( 0, 0, oldTesting, 270-oldTesting, -1, testing, 270-testing, 30, "10", "100", "1k" );
  drawMeter( 106, 0, -1, oldTesting, 40, 20, testing, 40, "10", "100", "1k" );
  drawMeter( 212, 0, -1, -1, oldTesting, 50, 75, testing, "10", "100", "1k" );

  drawMeter( 0, 106, -1, oldTesting, -1, 100, testing, -1, "10", "100", "1k" );
  drawMeter( 106, 106, oldTesting, -1, -1, testing, -1, -1, "10", "100", "1k" );
  drawMeter( 212, 106, -1, -1, -1, 0, 0, 0, "10", "100", "1k" );
  
  
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
  
  oldTesting = testing;
  testing++;
  if ( testing > 270 ) testing = 0;
  
}




void drawMeterFrame ( int x, int y, char* title ) {

 tft.fillRect(x, y, 106, 106, ILI9340_BLUE );
 tft.drawRect(x, y, 106, 106, ILI9340_WHITE );
 tft.fillCircle( x + 106/2, y + 106/2, 106*.48, ILI9340_BLACK );
 tft.drawCircle( x + 106/2, y + 106/2, 106*.48, ILI9340_WHITE );
 tft.fillRect(x+106/2+2, y+106/2+2, 106/2-2, 106/2-2, ILI9340_BLUE );
 tft.drawRect(x+106/2+2, y+106/2+2, 106/2-2, 106/2-2, ILI9340_WHITE );
 
 tft.setCursor( x+106/2+17, y+106/2+10);
 tft.setTextColor(ILI9340_WHITE);
 tft.setTextSize(2);
 tft.print( title );
 
  
}

void drawMeter( int x, int y, int oldRed, int oldGreen, int oldYellow, int red, int green, int yellow, char* scaleRed, char* scaleGreen, char* scaleYellow ) {
 
 float alfa1;
  
 // x is the horizontal upper left corner
 // y is the verticle upper left corner 

 tft.fillCircle( x + 106/2, y + 106/2, 3, ILI9340_WHITE );

  if ( oldGreen != green ) {
    drawMeterLine( x, y, oldGreen, .44, ILI9340_BLACK );
    drawMeterLine( x, y, green, .44, ILI9340_GREEN );
    tft.setCursor( x+106/2+16, y+106/2+26);
    tft.setTextColor(ILI9340_GREEN, ILI9340_BLUE );
    tft.setTextSize(1);
    tft.print( scaleGreen );
    tft.print( "   " );
  }
  if ( oldRed != red ) {
    drawMeterLine( x, y, oldRed, .35, ILI9340_BLACK );
    drawMeterLine( x, y, red, .35, ILI9340_RED );
    tft.setCursor( x+106/2+16, y+106/2+26+8);
    tft.setTextColor(ILI9340_RED, ILI9340_BLUE );
    tft.setTextSize(1);
    tft.print( scaleRed );
    tft.print( "   " );
  }
  if ( oldYellow != yellow ) {
    drawMeterLine( x, y, oldYellow, .25, ILI9340_BLACK );
    drawMeterLine( x, y, yellow, .25, ILI9340_YELLOW );
    tft.setCursor( x+106/2+16, y+106/2+26+8+8);
    tft.setTextColor(ILI9340_YELLOW, ILI9340_BLUE );
    tft.setTextSize(1);
    tft.print( scaleYellow );
    tft.print( "   " );
  }

}

void drawMeterLine( int x, int y, int number, float length, uint16_t color ) {
  float alfa1;

  if ( number > 270 ) number = 270;
  if ( number < 0 ) number = 0;

  alfa1 = 2*3.14*(360-number)/360; // convert ungle from degree in radian  
  tft.drawLine(x + 106/2,y + 106/2, x+(106/2)+(106*length)*sin(alfa1), y+(106/2)+(106*length)*cos(alfa1), color); 
}








void displayTime(DateTime rightNow) {
  
  tft.setTextSize(1);

  tft.setCursor(0,6*8);
  tft.setTextSize(5);
  
  tft.print(rightNow.year(), DEC);
  tft.print('/');
  if ( rightNow.month() < 10 ) tft.print("0");
  tft.print(rightNow.month(), DEC);
  tft.print('/');
  if ( rightNow.day() < 10 ) tft.print("0");
  tft.print(rightNow.day(), DEC);
  tft.print(' ');

  tft.setCursor(6*5,12*8);

  if ( rightNow.hour() < 10 ) tft.print("0");
  tft.print(rightNow.hour(), DEC);
  tft.print(':');
  if ( rightNow.minute() < 10 ) tft.print("0");
  tft.print(rightNow.minute(), DEC);
  tft.print(':');
  if ( rightNow.second() < 10 ) tft.print("0");
  tft.print(rightNow.second(), DEC);
 
}

void displayTimeSmall(DateTime rightNow) {
  
  tft.setTextSize(1);

  tft.setCursor(3*6*1,27*8);
  tft.setTextSize(2);
  tft.setTextColor( ILI9340_WHITE, ILI9340_BLACK );
  
  tft.print(rightNow.year(), DEC);
  tft.print('/');
  if ( rightNow.month() < 10 ) tft.print("0");
  tft.print(rightNow.month(), DEC);
  tft.print('/');
  if ( rightNow.day() < 10 ) tft.print("0");
  tft.print(rightNow.day(), DEC);
  tft.print(' ');

  if ( rightNow.hour() < 10 ) tft.print("0");
  tft.print(rightNow.hour(), DEC);
  tft.print(':');
  if ( rightNow.minute() < 10 ) tft.print("0");
  tft.print(rightNow.minute(), DEC);
  tft.print(':');
  if ( rightNow.second() < 10 ) tft.print("0");
  tft.print(rightNow.second(), DEC);
  
  tft.print("UTC");
 
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


