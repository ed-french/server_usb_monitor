#include <Arduino.h>

/*******************************************************************
    A touch screen test for the ESP32 Cheap Yellow Display.

    https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display

    If you find what I do useful and would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/

    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

// Make sure to copy the UserSetup.h file into the library as
// per the Github Instructions. The pins are defined in there.

// ----------------------------
// Standard Libraries
// ----------------------------

#include <SPI.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <XPT2046_Touchscreen.h>
// A library for interfacing with the touch screen
//
// Can be installed from the library manager (Search for "XPT2046")
//https://github.com/PaulStoffregen/XPT2046_Touchscreen

#include <TFT_eSPI.h>
// A library for interfacing with LCD displays
//
// Can be installed from the library manager (Search for "TFT_eSPI")
//https://github.com/Bodmer/TFT_eSPI


// ----------------------------
// Touch Screen pins
// ----------------------------

// The CYD touch uses some non default
// SPI pins

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// ----------------------------

SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

TFT_eSPI tft = TFT_eSPI();

const int max_lines=30;
const int line_max_length=52;

char screenlines[max_lines][line_max_length+1]={0};
String next_line;
int next_line_no=0;

void setup() {
  Serial.begin(115200);

  // Start the SPI for the touch screen and init the TS library
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(1);

  // Start the tft display and set it to black
  tft.init();
  tft.setRotation(1); //This is the display in landscape
  tft.invertDisplay(1);

  // Clear the screen before writing to it
  tft.fillScreen(TFT_BLACK);

  int x = 320 / 2; // center of display
  int y = 100;
  int fontSize = 2;
  // tft.drawCentreString("Touch Screen to Fart", x, y, fontSize);
}

void printTouchToSerial(TS_Point p) {
  Serial.print("Pressure = ");
  Serial.print(p.z);
  Serial.print(", x = ");
  Serial.print(p.x);
  Serial.print(", y = ");
  Serial.print(p.y);
  Serial.println();
}

void printTouchToDisplay(TS_Point p) {

  // Clear screen first
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  int x = 320 / 2; // center of display
  int y = 100;
  int fontSize = 2;

  String temp = "Pressure = " + String(p.z);
  tft.drawCentreString(temp, x, y, fontSize);

  y += 16;
  temp = "X = " + String(p.x);
  tft.drawCentreString(temp, x, y, fontSize);

  y += 16;
  temp = "Y = " + String(p.y);
  tft.drawCentreString(temp, x, y, fontSize);
}

void test_serial()
{
  char buf[2];
  while (true) {
    uint8_t c=Serial.readBytes(buf,1);
    if (buf[0]!=0) {
      Serial.printf("%0x ",buf[0]);
    }
    buf[0]=0;
  }
}

int read_line(char * buf, int maxlen,int timeout_ms) {
  /*
    buf is where the result will go
    maxlen is 1 less than the length of buf or less
    timeout_ms is when you'll get a partial string back as it is

    returns -1 if nothing was found by timeout
    returns a zero length if a CR or LF was found without any content

    Note control characters will be dumped
    Either a CR, LF or any both will trigger a return of what we've got
  */
  int chars_found=0;
  int endtime=millis()+timeout_ms;
  char new_bit[2];
  // Fill the buf with zeros, so we can return a valid
  // char array at any time!
  for (int i=0;i<maxlen+1;i++) {
    buf[i]=0;
  }
  while (true)
  {
    

    
    // Deal with timing out by returning a zero-length string
    if (millis()>endtime)
    {
      if (chars_found==0) return -1; // We timed out with no content
      // Return what we've built so far
      return chars_found;
    }

    while (millis()<endtime && !Serial.available())
    {
      delay(1);
    }

    new_bit[0]=0;
    // Read in a next character
    Serial.readBytes(new_bit,1);

    uint8_t next_byte=new_bit[0];

    if (next_byte==0) continue; // Just ignore when there's nothing back from the port
    


    // Deal with ordinary characters

    endtime=millis()+timeout_ms; // reset the timeout

    if (next_byte>31) {
      buf[chars_found]=next_byte;
      // Check if we're full
      chars_found++;
      if (chars_found>=maxlen) return chars_found;
      continue;
    }

    // Deal with CR and LF

    if (next_byte==0x0a || next_byte==0x09)
    {
      delay(1); // Just make sure we have a few ms for some extra LF or CR to arrive
      uint8_t peeked=Serial.peek();
      if ((next_byte==0x0a && peeked==0x09)  || (next_byte==0x09 && peeked==0x0a)) {
        Serial.readBytes(new_bit,1); // Throw away the byte if it's a pair
      }
      if (chars_found==0)
      {
        buf[0]=0;
        return 0;
      }
      return chars_found;

    }

    //  Loop never ends here!





  }

}

void test_read_line() {

  char temp[50];
  while(true) {
    int count=read_line(temp,49,2000);
    Serial.printf("%d:>>> %s\n",count,temp);
  }
}

void loop() {

  char line_buf[line_max_length+1];
  int len=read_line(line_buf,line_max_length,1000);


  if (len==-1) return; // timed out without finding any content


  strncpy(screenlines[next_line_no],line_buf,line_max_length);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(0,0);
  tft.setTextSize(1);
  for (int i=0;i<max_lines;i++)
  {
    if (i<next_line_no) tft.setTextColor(TFT_DARKCYAN);
    if (i==next_line_no) tft.setTextColor(TFT_WHITE);
    if (i>next_line_no) tft.setTextColor(TFT_MAGENTA);

    if (i==next_line_no)
    {
      tft.print(">");
    } else {
      tft.print(" ");
    }

    tft.println(screenlines[i]);
  }
  next_line_no++;
  if (next_line_no==max_lines) {
    next_line_no=0;
  }

  
  delay(100);

}