/*
 Fade
 
 This example shows how to fade an LED on pin 9
 using the analogWrite() function.
 
 This example code is in the public domain.
 */

int led = 9;           // the pin that the LED is attached to
int brightness = 0;    // how bright the LED is
int desiredBrightness = 0;
int fadeAmount = 10;    // how many points to fade the LED by
int count = 0;
String commandString;

// the setup routine runs once when you press reset:
void setup()  { 
  // declare pin 9 to be an output:
  pinMode(led, OUTPUT);
  // Setup the serial port
  Serial.begin(9600);
} 

// the loop routine runs over and over again forever:
void loop()  { 
  
  char character;
  
  // Get Werial Stuff
  if ( Serial.available()) {
    character = Serial.read();
  
    if ( character == 'Z' ) {
      char temp[5];
      commandString.toCharArray(temp, 5);
      desiredBrightness = (100-atof(temp))/100*255; 
      if ( desiredBrightness < 0 ) { desiredBrightness = 0; }
      if ( desiredBrightness > 255 ) { desiredBrightness = 255; }
      commandString = "";
    } else {
      if ( character != ' ' ) { commandString.concat(character); }
    }
  }
  
  
  // set the brightness of pin 9:
  analogWrite(led, brightness);    

  // change the brightness for next time through the loop:
  brightness = brightness + fadeAmount;
 
  
  // reverse the direction of the fading at the ends of the fade: 
  //if (brightness == 0 || brightness == 255) {
  //  fadeAmount = -fadeAmount ; 
  //}

  if ( brightness < desiredBrightness ) {
    fadeAmount = 1;
  } else if ( brightness > desiredBrightness ) {
    fadeAmount = -1;
  } else {
    fadeAmount = 0;
  } 
  
  // wait for 30 milliseconds to see the dimming effect    
  delay(20);                            
}

