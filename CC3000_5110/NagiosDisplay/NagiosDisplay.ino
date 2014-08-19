/* ********************************************************

  This will dipslay output from Nagios and NewRelic
  
  
    ******************************************************* */

// Frist thing, we need the SD card for configuration information
#include <SD.h>

// This is the config file we will read
const char configFileName[] = "config.txt";

// Pull in the cc3000 requirements
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include "utility/debug.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed

#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.







void setup() {

  // For now, we will display debug and other status information on the serial port
  Serial.begin(9600);
  while (!Serial) {
    ;  // Wait for the serial port - apparently only needed for Leonardo?
  }
  
  Serial.print("Opening SD card..");
  pinMode(10, OUTPUT);
  if ( !SD.begin(4)) {
    Serial.println("Failed to open the SD card.");
    return;
  }
  Serial.println("Card open.");
  
//  if (!cc3000.begin())
//  {
//    Serial.println(F("Couldn't begin()! Check your wiring?"));
//    while(1);
//  }
  
  // Optional SSID scan
  // listSSIDResults();

  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);

  Serial.print(F("\nAttempting to connect to ")); Serial.println(configReadString("ssid"));
  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  Serial.print(F("\nAttempting to connect to ")); Serial.println(configReadString("passphrase"));
  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  Serial.print(F("\nAttempting to connect to ")); Serial.println(configReadString("host"));
  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  Serial.print(F("\nAttempting to connect to ")); Serial.println(configReadString("path"));
  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  Serial.print(F("\nAttempting to connect to ")); Serial.println(configReadString("ssid"));
  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  Serial.print(F("\nAttempting to connect to ")); Serial.println(configReadString("ssid"));
  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  Serial.print(F("\nAttempting to connect to ")); Serial.println(configReadString("passphrase"));
  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  Serial.print(F("\nAttempting to connect to ")); Serial.println(configReadString("host"));
  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);



  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(configReadString("ssid"));
  char waste1[20];
  char waste2[20];
  configReadString("ssid").toCharArray(waste1,100);
  configReadString("passphrase").toCharArray(waste2,100);
  
  Serial.print("Here:");
  Serial.println(waste1);
  Serial.print("There:");
  Serial.println(waste2);
  
  Serial.println("Connecting..");


  if (!cc3000.connectToAP(waste1, waste2, WLAN_SEC_WPA2)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

}



void loop() {
  
  // Serial.println("Starting configReadString");
  // configReadString();
  // Serial.println("Done with configReadString");
  
  // delay(10000);
  
}

// Grab a particular key/value from the config file
String configReadString( char *findKey) {
 File myFile;
 char line[80];
 char *key;
 char *value;
 int n;
 
 delay(100); // Give it some time to charge up?? 
 
 myFile = SD.open( configFileName );
 
 if ( !myFile) {
   Serial.println("Error opening config file");
   while(1);  //Go to sleep now.
 }
 
 while ( n < sizeof(line) ) {
   line[n] = 0;
   n++;
 }
 
 while (n = readLine( myFile, line, sizeof(line)) > 0 ) {
   String buffer = String(line);
   int pipeChar = buffer.indexOf('|');
   String realKey = buffer.substring(0,pipeChar);
   String realValue = buffer.substring(pipeChar+1);
   
   if ( realKey == findKey ) {
     myFile.close();
     return (realValue);
   }
   
   while ( n < sizeof(line) ) {
     line[n] = 0;
     n++;
   }
 }
 
 myFile.close();
}

// Read an entire line at once.  (\n terminated.)
int readLine ( File& myFile, char *buffer, int length) {
  int  n;
  char c;
  
  // While there is data available and we can read it and we are shorter then our supplied length.. read.
  while ( myFile.available() && n < length ) {
    c = myFile.read();
    // If we have a new line then return
    if ( c == '\n' ) {
      return(1);
    }
    buffer[n] = c;
    n++;
  }
  // Either we ran out of file or buffer - so bail and tell the caller.
  return(0);
}


