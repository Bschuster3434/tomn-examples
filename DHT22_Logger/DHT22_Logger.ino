/*

 Udp NTP Client
 
 Get the time from a Network Time Protocol (NTP) time server
 Demonstrates use of UDP sendPacket and ReceivePacket 
 For more on NTP time servers and the messages needed to communicate with them, 
 see http://en.wikipedia.org/wiki/Network_Time_Protocol
 
 created 4 Sep 2010 
 by Michael Margolis
 modified 9 Apr 2012
 by Tom Igoe
 
 This code is in the public domain.

 */

#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>


#include <SPI.h>         
#include <Ethernet.h>
#include <EthernetUdp.h>

// pin 8 - PWM to the backlight (EL)
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
Adafruit_PCD8544 lcd0 = Adafruit_PCD8544(7, 6, 5, 4, 3);

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {  
  0xDE, 0xAD, 0x4E, 0xEF, 0xAE, 0xED };

unsigned int localPort = 8888;      // local port to listen for UDP packets

IPAddress timeServer(209,43,20,114); // time.nist.gov NTP server

const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 

unsigned long secsSince1900;

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;
EthernetClient client;

#include <DHT22.h>
#define DHT22_PIN 2

// Setup a DHT22 instance
DHT22 myDHT22(DHT22_PIN);

char server[] = "data.sparkfun.com";    // name address for data.sparkFun (using DNS)

/////////////////
// Phant Stuff //
/////////////////
const String publicKey = "v0QKZ6aLgRibO4qA1LXp";
const String privateKey = "aP0XeZwz2bfGxVYmZe5k";
const byte NUM_FIELDS = 3;
const String fieldNames[NUM_FIELDS] = {"temp", "humidity", "eventtype"};
String fieldData[NUM_FIELDS];




void setup() 
{

  lcd0.begin();
  lcd0.clearDisplay();
  lcd0.setContrast(25);
  analogWrite(8,130);
  lcd0.setTextColor(1,0);

  lcd0.setCursor(0,0);

  lcd0.println("UDP NTP Client");
  lcd0.println();
  lcd0.println("Await Serial");
  lcd0.display();

 
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
//   while (!Serial) {
//    ; // wait for serial port to connect. Needed for Leonardo only
//  }

  Serial.println("Starting up and aquiring IP Address");
  lcd0.setCursor(0,0);
  lcd0.print(F("Aquiring IP      "));


  // start Ethernet and UDP
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }
 
  lcd0.clearDisplay();
  lcd0.setCursor(0,0);
  lcd0.println(F("Starting UDP  client"));
  lcd0.println();
  lcd0.print(Ethernet.localIP());
  lcd0.display();
  delay(5000);
  lcd0.clearDisplay();

  fieldData[2] = String(F("OfficeStats"));

  Udp.begin(localPort);
}

void loop()
{

  // Reest the clock 37 seconds after every 10th minute - Lower server laod.  :)  
  if ( !(secsSince1900 % 60) || secsSince1900 == 0 ) {
    
    lcd0.setCursor(0,0);
    lcd0.println("Sending reques");
    lcd0.display();

    sendNTPpacket(timeServer); // send an NTP packet to a time server

    lcd0.setCursor(0,0);
    lcd0.println("Await response");
    lcd0.display();

    // wait to see if a reply is available
    delay(500);  


    if ( Udp.parsePacket() ) {  
      lcd0.setCursor(0,0);
      lcd0.println("Parsing       ");
      lcd0.display();
      // We've received a packet, read the data from it
      Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

      //the timestamp starts at byte 40 of the received packet and is four bytes,
      // or two words, long. First, esxtract the two words:

      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      secsSince1900 = highWord << 16 | lowWord;  
      Serial.print("Seconds since Jan 1 1900 = " );
      Serial.println(secsSince1900);               

      lcd0.setCursor(0,0);
      lcd0.println("              ");
      lcd0.display();
    }
  }




//  lcd0.setCursor(0,0);
//  lcd0.print("                ");
  lcd0.setCursor(0,0);



    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;     
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears - ( 5*3600);  // GMT - 4!
    // print Unix time:
    Serial.println(epoch);                               

  lcd0.setCursor(0,5*8);
  lcd0.print("Time: ");

    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');  
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':'); 
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch %60); // print the second

    lcd0.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    lcd0.print(':');  
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      lcd0.print('0');
    }
    lcd0.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    lcd0.print(':'); 
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      lcd0.print('0');
    }
    lcd0.print(epoch %60); // print the second

  // Read the DHC22 every 5 seconds
  if ( !(secsSince1900 % 3 ) ) {
    lcd0.setCursor(0,0);
    lcd0.println("DHT22 Read    ");
    lcd0.display();
    
    lcd0.setCursor(0*6,1*8);
    lcd0.setTextSize(4);
    myDHT22.readData();
    lcd0.print(((int)(myDHT22.getTemperatureF()*100))/100);
    lcd0.print("F");
    lcd0.display();
    
    lcd0.setTextSize(1);

    lcd0.setCursor(0,0);
    lcd0.println("              ");
    lcd0.display();
  }
  if ( !(secsSince1900 % 10 ) ) {
   
    myDHT22.readData();

    lcd0.setCursor(0,0);
    lcd0.println("Sending data  ");
    lcd0.display();
    
    fieldData[0] = String((int)(myDHT22.getTemperatureF()*10));
    fieldData[1] = String((int)myDHT22.getHumidity());
    
    postData();
    
    lcd0.setCursor(0,0);
    lcd0.println("              ");
    lcd0.display();
  }
  
  // wait ten seconds before asking for the time again
  delay(1000); 
  secsSince1900++;
  lcd0.display();
}


// Send data up to sparkfun
void postData()
{
  Serial.println(F("Posting data to data.sparkfun.com"));

  // Make a TCP connection to remote host
  if (client.connect(server, 80))
  {
    // Post the data! Request should look a little something like:
    // GET /input/publicKey?private_key=privateKey&light=1024&switch=0&name=Jim HTTP/1.1\n
    // Host: data.sparkfun.com\n
    // Connection: close\n
    // \n
    client.print("GET /input/");
    client.print(publicKey);
    client.print("?private_key=");
    client.print(privateKey);
    for (int i=0; i<NUM_FIELDS; i++)
    {
      client.print("&");
      client.print(fieldNames[i]);
      client.print("=");
      client.print(fieldData[i]);

      Serial.print(fieldNames[i]);
      Serial.print("=");
      Serial.println(fieldData[i]);
    }
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();
  }
  else
  {
    Serial.println(F("Connection failed"));
  } 

  // Check for a response from the server, and route it
  // out the serial port.
  while (client.connected())
  {
    if ( client.available() )
    {
      char c = client.read();
      Serial.print(c);
    }      
  }
  Serial.println();
  client.stop();
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
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket(); 
}










