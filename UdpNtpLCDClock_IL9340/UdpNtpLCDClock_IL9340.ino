

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"

#include <Wire.h>
#include<RTClib.h>

#define _sclk 13
#define _miso 12
#define _mosi 11
#define _cs 3
#define _dc 9
#define _rst 8

Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _rst);

RTC_DS1307 RTC;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

EthernetUDP udp;

char server[] = "pool.ntp.org";

// HACK!
IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov NTP server

unsigned int localPort = 8888;      // local port to listen for UDP packets
const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 
unsigned long secsSince1900;



void setup() {
  Serial.begin(9600);
  while(!Serial); 
  
  tft.begin();

  tft.setRotation(3);

  tft.fillScreen(ILI9340_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9340_WHITE, ILI9340_BLACK);
  tft.setTextSize(2);
  tft.println("      NTP Clock");
  tft.setTextSize(1);

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
  // print the Ethernet board/shield's IP address:
  tft.print("My IP address: ");
  tft.println(Ethernet.localIP());
  
  udp.begin(localPort);

}


void loop() {

  //Every 10 minutes, send a new request to the NTP server
  if ( !(secsSince1900 % 600) || secsSince1900 == 0 ) {
    tft.setCursor(0,25*8);
    tft.print("Sending             ");
    sendNTPpacket(timeServer); // send an NTP packet to a time server
  }

  if (  udp.parsePacket() ) {
    tft.setCursor(0,25*8);
    tft.print("Received");
    // We've received a packet, read the data from it
    udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    secsSince1900 = highWord << 16 | lowWord;  
    tft.setCursor(0,12*8);
    tft.print("Seconds since Jan 1 1900 = " );
    tft.println(secsSince1900);               
   
    // Every valid NTP packet - set the RTC!
    const unsigned long seventyYears = 2208988800UL;     
    unsigned long epoch = secsSince1900 - seventyYears - ( 4*3600);  // GMT - 4!
    RTC.adjust(DateTime(epoch));
    tft.setCursor(8*6, 25*8);
    tft.print(" - Adjusted");

  }
  
  
  
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;     
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears - ( 4*3600);  // GMT - 4!

  tft.setCursor(0,5*8);
  tft.setTextSize(1);
  tft.println("Time: ");
  tft.setTextSize(6);


    tft.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    tft.print(':');  
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      tft.print('0');
    }
    tft.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    tft.print(':'); 
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      tft.print('0');
    }
    tft.println(epoch %60); // print the second

  

  
  
  tft.setTextSize(1);

  DateTime now = RTC.now();

  tft.setCursor(0,20*8);
  tft.setTextSize(2);
  
  tft.print(now.year(), DEC);
  tft.print('/');
  tft.print(now.month(), DEC);
  tft.print('/');
  tft.print(now.day(), DEC);
  tft.print(' ');
  tft.print(now.hour(), DEC);
  tft.print(':');
  tft.print(now.minute(), DEC);
  tft.print(':');
  tft.print(now.second(), DEC);
  tft.println();
 
  tft.setTextSize(1);
  tft.print(" since 1970 = ");
  tft.print(now.unixtime());
  tft.print("s = ");
  tft.print(now.unixtime() / 86400L);
  tft.println("d");

  
  
  
  
  
  delay(1000);
  secsSince1900++;  
  
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


