#include <Adafruit_NeoPixel.h>
#include <ctype.h>
#include <setjmp.h>

// Configuration for our LED strip and Arduino
#define NUM_LEDS 64
#define PIN 11

// Time in milliseconds for the flash of light
#define FLASH_TIME_INTERVAL     250

#define DEFAULT_BRIGHTNESS      32

// allocate our pixel memory, set interface to match our hardware
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// Colors
const uint32_t NONE    = pixels.Color(  0,   0,   0);
const uint32_t RED     = pixels.Color(128,   0,   0);
const uint32_t GREEN   = pixels.Color(  0, 128,   0);
const uint32_t BLUE    = pixels.Color(  0,   0, 128);
const uint32_t YELLOW  = pixels.Color(128, 128,   0);
const uint32_t ORANGE  = pixels.Color(128,  32,   0);
const uint32_t WHITE   = pixels.Color(255, 255, 255);

// Animations
enum {
  OFF = 0,
  DISABLED,             // ORANGE (static)
  INTAKE_UP,            // COGS BLUE+YELLOW
  INTAKE_DOWN,          // RED CRAWLER
  LAUNCH,               // BRIGHT FLASH THEN OFF

  ANIMATION_COUNT       // The number of animations
};


// A global variable to hold the current command to be executed
uint8_t currentCommand = DISABLED;

// The breadcrumb to get us back to the beginning of the loop
jmp_buf env;

#define INTERRUPTABLE_DELAY _delay

// Our magic delay function
void _delay(uint16_t timeout)
{
  while (timeout != 0)
  {
    delay(1);
    timeout--;

    // If there are any characters available on the serial port, read them
    while (Serial.available() > 0)
    {
      char commandCharacter;
      commandCharacter = Serial.read();
      if (isprint(commandCharacter) && (commandCharacter >= '0'))
      {
        uint8_t command = commandCharacter - '0';
        if (command < ANIMATION_COUNT)
        {
          // Found a valid command!
          if (command != currentCommand)
          {
            // It's a new command!
            currentCommand = command;
            // Jump back to the top of the loop
            longjmp(env, 1);
          }
        }
      }
    }
  }
}

// Fill the strip with a single color
void fillStrip(uint32_t c)
{
   for (unsigned i=0; i<pixels.numPixels(); i++)
   {
      pixels.setPixelColor(i, c);
   }
   pixels.show();
}

// Wait forever until a new command comes in
void wait_for_command(void)
{
  while (1) {
    INTERRUPTABLE_DELAY(1);
  }
}

/*******************************************************************************
 * Functions to execute the bling "actions"
 ******************************************************************************/

// Solid illumination of a single color
void solid(uint32_t color, uint8_t brightness)
{
  pixels.setBrightness(brightness);
  fillStrip(color);
  while (1)
  {
    INTERRUPTABLE_DELAY(10);
  }
}

// Flash of light for a short period of time, then off (runs once only)
void flash(unsigned long timeInterval, uint32_t color, uint8_t brightness)
{
  pixels.setBrightness(brightness);
  fillStrip(color);
  delay(timeInterval);
  fillStrip(NONE);
}

// Blink a single color on and off at the given time interval
void blink(unsigned long timeInterval, uint32_t color, uint8_t brightness)
{
  pixels.setBrightness(brightness);
  while (1)
  {
    fillStrip(color);
    INTERRUPTABLE_DELAY(timeInterval);
    fillStrip(NONE);
    INTERRUPTABLE_DELAY(timeInterval);
  }
}

// Fade a color in and out
void fade(unsigned long timeInterval, uint32_t color, uint8_t maxBrightness)
{
  bool brightening = true;
  int brightness = 0;

  // Set the strip to the desired color to begin with
  fillStrip(color);

  while (1)
  {
    pixels.setBrightness(brightness);
    pixels.show();
    if (brightening)
    {
      if (brightness >= maxBrightness)  // At maximum brightness?
      {
        brightening = false;    // Time to switch to dimming
      }
      else
      {
        brightness++;           // Increase the brightness for next time
      }
    }
    else // dimming
    {
      if (brightness <= 0)      // At minimum brightness?
      {
        brightening = true;     // Time to switch to brightening
      }
      else
      {
        brightness--;           // Reduce the brightness for next time
      }
    }
    INTERRUPTABLE_DELAY(timeInterval);
  }
}

// Fade between two alternating colors
void spartronics_fade(unsigned long timeInterval, uint32_t color1, uint32_t color2, uint8_t maxBrightness)
{
  bool brightening = true;
  int brightness = 0;
  bool color1_is_current = true;;

  // Set the strip to the desired color to begin with
  fillStrip(color1);

  while (1)
  {
    pixels.setBrightness(brightness);
    pixels.show();
    if (brightening)
    {
      if (brightness >= maxBrightness)  // At maximum brightness?
      {
        brightening = false;    // Time to switch to dimming
      }
      else
      {
        brightness++;           // Increase the brightness for next time
      }
    }
    else // dimming
    {
      if (brightness <= 0)      // At minimum brightness?
      {
        // Switch to the alternate color
        if (color1_is_current)
        {
          fillStrip(color2);
          color1_is_current = false;
        }
        else
        {
          fillStrip(color1);
          color1_is_current = true;
        }

        brightening = true;     // Time to switch to brightening
      }
      else
      {
        brightness--;           // Reduce the brightness for next time
      }
    }
    INTERRUPTABLE_DELAY(timeInterval);
  }
}


void crawler(uint16_t timeInterval, uint32_t color, uint8_t length)
{
  // From the middle of the top, start illuminating pixels towards the ends
  // After length pixels have been added to each side, turn old pixels off
  // Pixels continue to scroll off of ends, then the scheme reverses

  uint8_t tempLength;
  uint8_t headUp;
  uint8_t headDown;
  uint8_t tailUp;
  uint8_t tailDown;

  while (1)
  {
    // Turn off the strip
    fillStrip(NONE);

    tempLength = length;
    headUp = (NUM_LEDS/2);
    headDown = headUp - 1;
    for (uint8_t i=0; i<length; i++)
    {
      pixels.setPixelColor(headUp++, color);
      pixels.setPixelColor(headDown--, color);
      pixels.show();
      INTERRUPTABLE_DELAY(timeInterval);
    }
    tailUp = (NUM_LEDS/2);
    tailDown = tailUp - 1;
    while (headUp < NUM_LEDS)
    {
      pixels.setPixelColor(headUp++, color);
      pixels.setPixelColor(headDown--, color);
      pixels.setPixelColor(tailUp++, OFF);
      pixels.setPixelColor(tailDown--, OFF);
      pixels.show();
      INTERRUPTABLE_DELAY(timeInterval);
    }
    while (tailUp < NUM_LEDS)
    {
      pixels.setPixelColor(tailUp++, OFF);
      pixels.setPixelColor(tailDown--, OFF);
      pixels.show();
      INTERRUPTABLE_DELAY(timeInterval);
    }
    // Reverse!
    tempLength = length;
    headUp = NUM_LEDS - 1;
    headDown = 0;
    for (uint8_t i=0; i<length; i++)
    {
      pixels.setPixelColor(headUp--, color);
      pixels.setPixelColor(headDown++, color);
      pixels.show();
      INTERRUPTABLE_DELAY(timeInterval);
    }
    tailUp = NUM_LEDS - 1;
    tailDown = 0;
    while (headUp >= (NUM_LEDS/2))
    {
      pixels.setPixelColor(headUp--, color);
      pixels.setPixelColor(headDown++, color);
      pixels.setPixelColor(tailUp--, OFF);
      pixels.setPixelColor(tailDown++, OFF);
      pixels.show();
      INTERRUPTABLE_DELAY(timeInterval);
    }
    while (tailUp >= (NUM_LEDS/2))
    {
      pixels.setPixelColor(tailUp--, OFF);
      pixels.setPixelColor(tailDown++, OFF);
      pixels.show();
      INTERRUPTABLE_DELAY(timeInterval);
    }
  }
}

// Number of LEDs per "cog" on the animation
#define COG_SIZE 5

// A counter from 0 to (2*COG_SIZE)-1
uint8_t cogOffset=0;

// Classic Spartronics Cogs
void cogs_init(uint32_t color1, uint32_t color2)
{
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if ((((uint8_t) (i / COG_SIZE)) & 1) == 0) {
      pixels.setPixelColor(i, color1);
    }
    else {
      pixels.setPixelColor(i, color2);
    }
  }
  pixels.show();
}

//moves every led up one, and inserts colorIn at leds[0]
void shiftUp(uint32_t colorIn) {
  for (int i = (NUM_LEDS - 1); i > 0; i--) {
    //The last led becomes the previous led's color
    pixels.setPixelColor(i, pixels.getPixelColor(i-1));
  }
  pixels.setPixelColor(0, colorIn);
}

//moves every led down one, and inserts colorIn at leds[(NUM_LEDS - 1)]
void shiftDown(uint32_t colorIn) {
  for (int i = 0; i < (NUM_LEDS - 1); i++) {
    //The first led becomes the next led's color
    pixels.setPixelColor(i, pixels.getPixelColor(i+1));
  }
  pixels.setPixelColor(NUM_LEDS-1, colorIn);
}

void moveCogsUp() {
  boolean changeColor = true;
  for (int i = 0; i < (COG_SIZE - 1); i++) {
    if (pixels.getPixelColor(i) != pixels.getPixelColor(i+1)) {
      changeColor = false;
    }
  }
  if (changeColor) {
    shiftUp(pixels.getPixelColor(COG_SIZE));
  }
  else {
    shiftUp(pixels.getPixelColor(0));
  }
}

void moveCogsDown() {
  boolean changeColor = true;
  for (int i = (NUM_LEDS - 1); i > (NUM_LEDS - (COG_SIZE)); i--) {
    if (pixels.getPixelColor(i) != pixels.getPixelColor(i-1)) {
      changeColor = false;
    }
  }
  if (changeColor) {
    shiftDown(pixels.getPixelColor(NUM_LEDS - COG_SIZE - 1));
  }
  else {
    shiftDown(pixels.getPixelColor(NUM_LEDS - 1));
  }
}

void rotatingCogsUp(uint32_t colorOne, uint32_t colorTwo) {
  pixels.show();
  for (int i = 0; i < 30; i++) {
    for (int c = 0; c < COG_SIZE; c++) {
      moveCogsUp();
      _delay(100);
      pixels.show();
    }
  }
}

void rotatingCogsDown(uint32_t colorOne, uint32_t colorTwo) {
  pixels.show();
  for (int i = 0; i < 30; i++) {
    for (int c = 0; c < COG_SIZE; c++) {
      moveCogsDown();
      _delay(100);
      pixels.show();
    }
  }
}

void cogs(uint32_t color1, uint32_t color2)
{
  cogs_init(color1, color2);
  while (1) {
    rotatingCogsUp(color1, color2);
    rotatingCogsDown(color1, color2);
  }
}

/*******************************************************************************
 * Arduino setup() and loop()
 ******************************************************************************/

void setup(void) {
  Serial.begin(9600);
  pixels.begin(); // initialize neopixel library
  pixels.setBrightness(DEFAULT_BRIGHTNESS);
  fillStrip(ORANGE);
}

void loop(void)
{
  // Save a breadcrumb of where to jump back to
  if (setjmp(env) != 0)
  {
    // If we jumped back (non-zero return value), then exit the loop()
    // and it will be re-run again, leaving a new breadcrumb.
    return;
  }

  // From here everything should be interruptable (to switch animations)
  // as long as the animation code uses the INTERRUPTABLE_DELAY function.

  // Take the current command and set the parameters
  switch (currentCommand)
  {
    // Bling effects
    case OFF:
      solid(NONE, DEFAULT_BRIGHTNESS);
      break;
    case DISABLED:
      solid(ORANGE, DEFAULT_BRIGHTNESS);
      break;
    case INTAKE_UP:
      cogs(BLUE, YELLOW);
      wait_for_command();
      break;
    case INTAKE_DOWN:
      crawler(10, RED, 30);
      break;
    case LAUNCH:
      flash(FLASH_TIME_INTERVAL, WHITE, 255);
      currentCommand = 0;
      wait_for_command();
      break;
    default:
      solid(YELLOW, 128);
  }
}
/* end of loop func */
