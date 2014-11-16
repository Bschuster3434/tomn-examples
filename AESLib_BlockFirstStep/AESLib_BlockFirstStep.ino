//include headerfile and library for AES
#include <AESLib.h>

void setup(){
  Serial.begin(9600);
}

void loop(){

  Serial.println("---------------------");

  uint8_t key[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
  char data[] = "ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF12345    67890ABCDEF1234567890ABCDEF1234567890";//128 byte

  Serial.print("BeforeEncode:");
  Serial.println(data);

  aes128_enc_single(key, data);
  Serial.print("After Encode:");
  Serial.println(data);
  aes128_dec_single(key, data);
  Serial.print("After Decode:");
  Serial.println(data);

  unsigned long sendcount = 0;
  uint8_t iv[] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

  unsigned long t = micros();
  int a = analogRead(0);  //pinConnectedToNothing
  long r = random(0x8fff);
  sendcount += 1;
  memcpy(iv, &sendcount, 4);
  memcpy(iv+6, &t, 4);
  memcpy(iv+10, &a, 2);
  memcpy(iv+12, &r, 4);

  aes128_cbc_enc(key, iv, data, 128);
  Serial.print("After Block Encode:");
  Serial.println(data);

  aes128_cbc_dec(key, iv, data, 128);
  Serial.print("After Block Decode:");
  Serial.println(data);




  delay(5000);
  
  
  
}
