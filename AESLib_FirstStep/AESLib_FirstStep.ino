//include headerfile and library for AES
#include <AESLib.h>

void setup(){
  Serial.begin(9600);
}

void loop(){
  uint8_t key[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
  char data[] = "ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF12345    67890ABCDEF1234567890ABCDEF1234567890";//128 byte
  aes128_enc_single(key, data);
  Serial.print("Before Enc:");
  Serial.println(data);
  aes128_dec_single(key, data);
  Serial.print("After Decode:");
  Serial.println(data);
  delay(15000);
}
