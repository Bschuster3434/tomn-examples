#include <AESLib.h>

#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008

#define UDP_TX_PACKET_MAX_SIZE 150

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

unsigned int localPort = 8888;      // local port to listen on
IPAddress ip(206, 246, 158, 179);

uint8_t packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,

EthernetUDP Udp;

void setup() {
  Serial.begin(9600);

  // start the Ethernet and UDP:
  Ethernet.begin(mac);
  Udp.begin(localPort);


}

void loop() {

//  uint8_t key[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
  uint8_t key[] = "1234567890123456";

  char data[] = "ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF12345    67890ABCDEF1234567890ABCDEF12345";//128 byte
  
  unsigned long sendcount = 0;
  uint8_t iv[] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

  unsigned long t = micros();
  int a = analogRead(0);
  long r = random(0x8fff);
  sendcount += 1;
  memcpy(iv, &sendcount, 4);
  memcpy(iv+6, &t, 4);
  memcpy(iv+10, &a, 2);
  memcpy(iv+12, &r, 4);

  Serial.print("Maximum UDP packet: ");
  Serial.println(UDP_TX_PACKET_MAX_SIZE);
  
  Serial.print("Size of data:");
  Serial.println(sizeof(data));

  aes128_cbc_enc(key, iv, data, 128);
  Serial.print("After Block Encode:");
  Serial.println(data);
  
  memcpy(packetBuffer, &iv, 16);
  memcpy(packetBuffer+16, &data, sizeof(data));
  
  Udp.beginPacket(ip, 12345);
  Udp.write(packetBuffer, 16+sizeof(data));
  Udp.endPacket();


  Serial.println("Sent!");
  delay(1000);
  
}




