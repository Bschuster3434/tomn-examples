
// Bring in the Ethernet Stuff
#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xDE, 0x00, 0xBE, 0xEF, 0xFE, 0xEF };

// This is the TCP connection
EthernetClient client;

// Where's the monitor server?  (This is pointing to a proxy to strip HTTPS!)
char ledServer[] = "iq-colo.sneaky.net";
int ledPort = 58471;

// Pull in the button library
#include <Bounce2.h>

#define easyPin 2
#define audioPin 3

int ledState = LOW;

Bounce debouncer = Bounce();

void setup() {
  
  Serial.begin(9600);
  
  Serial.println("Setting up the pins.");
  // Setup the input pin
  pinMode(easyPin, INPUT);
  digitalWrite(easyPin, HIGH);
  
  // Setup the debounce code
  debouncer.attach(easyPin);
  debouncer.interval(50);
  
  // Setup the audio output pin
  pinMode(audioPin, OUTPUT);
  digitalWrite(audioPin, LOW);
  
  // Start up the Ethernet interface
  Serial.println(F("Starting Ethernet.."));
  if ( Ethernet.begin( mac ) == 0 ) {
    Serial.println(F("Failed to configure Ethernet using DHCP."));
    while(1);
  }
  Serial.print("Ethernet on-line.  IP Address: ");
  Serial.print(Ethernet.localIP());
}

void loop() {
  boolean stateChanged = debouncer.update();
  int state = debouncer.read();
  
  // Detect the falling edge
   if ( stateChanged && state == LOW ) {
       Serial.println("Sending page");
       sendButton();
       Serial.println("Back");
   }
}

void sendButton() {
  
  String received;

  if ( client.connect( ledServer, ledPort) ) {
    // Make the request
    client.println(F("GET /ButtonPush HTTP/1.1"));
    client.print(F("Host: "));
    client.println(ledServer);
    client.println(F("Connection: close"));
    client.println();

    while ( client.connected() || client.available() )  {
      if ( client.available() ) {
        char c = client.read();
        Serial.print( c );
        
        received += c;
        
        // If it was successful, then tell the world just how easy it was.
        if ( received == "SUCCESS" ) {
          Serial.println("Firing audio.");
          digitalWrite( audioPin, HIGH ) ;
          delay( 5000 );
          Serial.println("Going back low on the output.");
          digitalWrite( audioPin, LOW );
          Serial.println("Done with Audio.");
        }
        
        // Make sure we don't get more then one line at a time.
        if ( c == '\n' ) { received = ""; }

      }
    }
  }
  client.stop();
}
