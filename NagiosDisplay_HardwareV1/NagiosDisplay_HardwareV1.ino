

// TODO:
// Add a watchdog - auto reset if it can't contact the server.  (Resolves DHCP and other issues.)



//General Stuff
unsigned long lastSet = 0;          // When did we last update?
unsigned long refresh = 10000;      // How long between refreshes  (10 seconds)

#define panelWidth 3
#define panelHeight 12

int panelPreviousState[panelWidth][panelHeight];

#define _panelIndicatorWidth 80
#define _panelIndicatorHeight 15
#define _panelOffsetWidth 0
#define _panelOffsetHeight 140

#define _statusWindowStarts 123
#define _statusWidth 20

// RED    - F800 - 1111 1000 0000 0000
// BLUE   - 001F - 0000 0000 0001 1111
// GREEN  - 07E0 - 0000 0111 1110 0000
// GRAY1  - 3AE7 - 0011 1001 1110 0111
// GRAY2  - 18E3 - 0001 1000 1110 0011
// YELLOW - FFE0 - 1111 1111 1110 0000

#define ILI9340_GRAY 0x3AE7

#define _meterWidth 4
#define _meterHeight 2
#define _meterWidthPixels  60
#define _meterHeightPixels 60

int meterPreviousState[_meterWidth][_meterHeight][3];
// For the definition of meterColors - see below
float meterLengths[3] = { .44, .35, .25 };


//NeoPixel
#include <Adafruit_NeoPixel.h>

#define NEOPIN 4
#define NEOLEDCOUNT 8
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEOLEDCOUNT, NEOPIN, NEO_GRB + NEO_KHZ800);

// Color Map
int ledColors [4][3] = {
  { 0,   255, 0   },  // GREEN
  { 255, 255, 0   },  // YELLOW
  { 255, 0,   0   },  // RED
  { 0,   0,   255 }   // BLUE
};
int ledBrightness = 5;

// TFT Display
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"

// This uses SPI - So, SCLK, MISO, MOSI and the following pins
#define _cs 3
#define _dc 9
#define _rst 8

Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _rst);

// Has to be here because of needing the include file
uint16_t meterColors[3] = { ILI9340_GREEN, ILI9340_YELLOW, ILI9340_RED };


// Ethernet Shield
// I am going to use DHCP and NOT specifiy a MAC address
#include <SPI.h>
#include <Ethernet.h>

// TODO - I'd like to make this something repeatable - EEPROM?
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF };

// This is the TCP connection
EthernetClient client;

// Where's the monitor server?  (This is pointing to a proxy to strip HTTPS!)
char ledServer[] = "iq-colo.sneaky.net";
int ledPort = 58471;










void setup() {
  
  Serial.begin(9600);
  while(!Serial);
  
  // Bring up the display
  tft.begin();
  tft.setRotation(2);  // Pins at the top
  tft.fillScreen(ILI9340_BLACK);
  
  // Bring up the NeoPixel string
  tft.println(F("Starting NeoPixel string."));
  strip.begin();
  strip.show();
  tft.println(F("Done."));
  
  // Bring up the Ethernet Interface
  tft.println(F("Starting Ethernet.."));
  if ( Ethernet.begin( mac ) == 0 ) {
    // TODO - WATCHDOG RESET!
    tft.println(F("Failed to configure Ethernet using DHCP."));
    while(1);
  }
  tft.println(F("Ethernet initialized."));
  tft.print(F("IP Address: "));
  tft.println( Ethernet.localIP() );
  
  tft.print(F("Free RAM: "));
  tft.println(getFreeRam(), DEC);
  
  // Leave the initialization messgaes up for 10 seconds and move on
  delay(2000);
  tft.fillScreen(ILI9340_BLACK);
  tft.setCursor(0, 0);

  // Initialize status of panel display
  for ( int x = 0; x < panelWidth; x++ ) {
    for ( int y = 0; y < panelHeight; y++ ) {
      panelPreviousState[x][y] = 254;
      drawIndicator( x, y, "", 255 );
    }
  }
  
  for ( int x = 0; x < _meterWidth; x++ ) {
    for ( int y = 0; y < _meterHeight; y++ ) {
      for ( int c = 0; c < 3; c++ ) {
        meterPreviousState[x][y][c] = 300;
      }
      drawMeterFrame( x, y );
    }
  }
  
}

void loop() {
  
  // Look to see if it's time to contact the server for an update
  // Are we currently connected?
  // Has it been greater then refresh seconds since our last attempt?
  // Have we made an attempt yet?
  if ( ( !client.connected() && ( millis() - lastSet > refresh ) )  || !lastSet ) {
    requestUpdate();
  }
  
  delay(1000);
}

void requestUpdate() {
  
  if ( client.connect( ledServer, ledPort) ) {
    // Make the request
    client.println(F("GET /LEDStatus HTTP/1.1"));
    client.print(F("Host: "));
    client.println(ledServer);
    client.println(F("Connection: close"));
    client.println();
    
    // Record that we sent this
    lastSet = millis();
    
    String received;
    
    // Clear the strip (but don't update it.)
    for ( int c = 0; c < NEOLEDCOUNT; c++ ) strip.setPixelColor( c, 0, 0, 0 );
    
    while ( client.connected() || client.available() )  {
      if ( client.available() ) {
        char c = client.read();
        received += c;
        
        if ( c == '\n' ) {
          int equals = received.indexOf('=');
          
          if ( equals != -1 && received.length() < 30 && received.length() > 0 ) {

            // Find out if this is an LED!
            if ( received.indexOf('L') == 0 ) {

              String ledString = received.substring(1,equals);
              int led = ledString.toInt();
            
              String valueString = received.substring(equals+1);
              int value = valueString.toInt();
            
              strip.setPixelColor ( led, ledColors[value][0], ledColors[value][1], ledColors[value][2]);
            }
            
            // Is it a status line?
            if ( received.indexOf('S') == 0 ) {
              updateStatusLine(received.substring(2));
            }
            
            // Is it an indicator?
            // I=00,00,HostName,0
            if ( received.indexOf('I') == 0 ) {
              received = received.substring(2);
              
              String xString = received.substring(0,2);
              int x = xString.toInt();

              String yString = received.substring(3,5);
              int y = yString.toInt();
              
              int c = received.indexOf(',', 6);
              String hostname = received.substring(6, c);

              String vString = received.substring(c+1,c+2);
              int v = vString.toInt();
              
              drawIndicator( x, y, hostname, v );
              
            }
            
            // Is it a graph?
            // G=03,01,8000,DW,050,100,220
            if ( received.indexOf('G') == 0 ) {
              received = received.substring(2);
              
              String xString = received.substring(0,2);
              int x = xString.toInt();

              String yString = received.substring(3,5);
              int y = yString.toInt();
              
              String scaleString = received.substring(6, 10);
              char scale[5];
              scaleString.toCharArray( scale, 5);
              String titleString = received.substring(11,13);
              char title[3];
              titleString.toCharArray( title, 3);
              int data[3];
              
              for ( int c = 0; c < 3; c++ ) {
                String xString = received.substring(14+c*4,17+c*4);
                data[c] = xString.toInt();
              }
              drawMeter( x, y, data, title, scale);

            }

          }
          received = "";
        }
      }
    }
    client.stop();
    
  } else {
    // Connection failed.
  }
  
  strip.setBrightness( ledBrightness );
  strip.show();
    
}

void drawIndicator ( int x, int y, String title, int value ) {
  
  if ( panelPreviousState[x][y] == value ) return;
  
  uint16_t color = ILI9340_WHITE;
  
  // Green - info up.
  // Yellow - service is warning
  // Red - service is critical
  // Cyan - Unknown
  // Magenta - HOST is down
  switch (value) {
    case 0:
      color = ILI9340_GREEN;
      break;
    case 1:
      // This was YELLOW, but I want it to stand out more.
      // YELLOW  - FFE0 - 1111 1111 1110 0000
      // TYELLOW - FC60 - 1111 1100 0110 0000
      color = 0xfc60;
      break;
    case 2:
      color = ILI9340_RED;
      break;
    case 3:
      color = ILI9340_CYAN;
      break;
    case 4:
      color = ILI9340_MAGENTA;
      break;
    default:
      color = ILI9340_BLACK;
  }

  tft.drawRect ( x * _panelIndicatorWidth + _panelOffsetWidth, y * _panelIndicatorHeight + _panelOffsetHeight, _panelIndicatorWidth , _panelIndicatorHeight, ILI9340_GRAY );
  tft.fillRect ( x * _panelIndicatorWidth+1 + _panelOffsetWidth, y * _panelIndicatorHeight+1 + _panelOffsetHeight, _panelIndicatorWidth-2, _panelIndicatorHeight-2, color );
  tft.setCursor(  x * _panelIndicatorWidth + _panelOffsetWidth + _panelIndicatorWidth * .5+1 - 6* title.length() * .5 , y * _panelIndicatorHeight + _panelOffsetHeight + _panelIndicatorHeight/2 - 4 );
  tft.setTextSize(1);
  if ( color == ILI9340_BLACK ) {
    tft.setTextColor ( ILI9340_WHITE );
  } else {
    tft.setTextColor ( ILI9340_BLACK );
  }
  tft.print( title );
  
  panelPreviousState[x][y] = value;

}

void updateStatusLine ( String statusText ) {

  tft.setCursor(0, _statusWindowStarts);
  tft.setTextColor(ILI9340_WHITE, ILI9340_BLACK);
  tft.setTextSize(2);
  tft.print(statusText);
  for ( int c = statusText.length() ; c < _statusWidth ; c++ ) tft.print(" ");

}

void drawMeterFrame ( int x, int y ) {
  
  int xPosition = x * _meterWidthPixels;
  int yPosition = y * _meterHeightPixels;

  tft.fillRect(xPosition, yPosition, _meterWidthPixels, _meterHeightPixels, ILI9340_BLUE );
  tft.drawRect(xPosition, yPosition, _meterWidthPixels, _meterHeightPixels, ILI9340_WHITE );
  tft.fillCircle( xPosition + _meterWidthPixels/2, yPosition + _meterHeightPixels/2, _meterWidthPixels*.48, ILI9340_BLACK );  // This one may throw off the radius.. if it's not "square"!
  tft.drawCircle( xPosition + _meterWidthPixels/2, yPosition + _meterHeightPixels/2, _meterWidthPixels*.48, ILI9340_WHITE );  // This one may throw off the radius.. if it's not "square"!
  tft.fillRect(xPosition+_meterWidthPixels/2+2, yPosition+_meterHeightPixels/2+2, _meterWidthPixels/2-2, _meterHeightPixels/2-2, ILI9340_BLUE );
  tft.drawRect(xPosition+_meterWidthPixels/2+2, yPosition+_meterHeightPixels/2+2, _meterWidthPixels/2-2, _meterHeightPixels/2-2, ILI9340_WHITE );
 
  
}

void drawMeter ( int x, int y, int values[], char *title, char *scale ) {
  
  int xPosition = x * _meterWidthPixels;
  int yPosition = y * _meterHeightPixels;
  int c;
  
  // It's not that easy.  If ANY value changes, they all have to be re-drawn
  int redraw = 0;
  
  for ( c = 0; c < 3; c++ ) {
    if ( meterPreviousState[x][y][c] != values[c] ) {
      redraw = 1;
    }
  }

  if ( !redraw ) return;
  
  // Erase the old lines
  for ( c = 0; c < 3; c++ ) {
    drawMeterLine( x, y, meterPreviousState[x][y][c], meterLengths[c], ILI9340_BLACK );
  }

  // Now go draw the new lines
  for ( c = 0; c < 3; c++ ) {
    drawMeterLine( x, y, values[c], meterLengths[c], meterColors[c] );
    meterPreviousState[x][y][c] = values[c];
  }
  
  tft.fillCircle( xPosition + _meterWidthPixels/2, yPosition + _meterHeightPixels/2, 3, ILI9340_WHITE );

  tft.setCursor( xPosition+_meterWidthPixels/2+5, yPosition+_meterHeightPixels/2+6 );
  tft.setTextColor(ILI9340_WHITE, ILI9340_BLUE );
  tft.setTextSize( 1 );
  tft.print( title );

  tft.setCursor( xPosition+_meterWidthPixels/2+5, yPosition+_meterHeightPixels/2+16 );
  tft.setTextColor( ILI9340_WHITE, ILI9340_BLUE );
  tft.setTextSize( 1 );
  tft.print( scale );
  
}

void drawMeterLine( int x, int y, int number, float length, uint16_t color ){
  
  float alfa1;
  
  int xPosition = x * _meterWidthPixels;
  int yPosition = y * _meterHeightPixels;
  
  if ( number > 270 ) number = 270;
  if ( number < 0 ) number = 0;
  
  alfa1 = 2*3.14*(360-number)/360;
  tft.drawLine( xPosition + _meterWidthPixels/2, yPosition + _meterHeightPixels/2, 
                xPosition+(_meterWidthPixels/2)+(_meterWidthPixels*length)*sin(alfa1), 
                yPosition+(_meterHeightPixels/2)+(_meterHeightPixels*length)*cos(alfa1), 
                color );
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
