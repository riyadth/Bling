#include <ctype.h>
#include <Adafruit_NeoPixel_ZeroDMA.h>



// Colors
#define OFF     Adafruit_NeoPixel::Color(0,0,0)         //actually a dim white
#define BLUE    Adafruit_NeoPixel::Color(0, 0, 128)
#define RED     Adafruit_NeoPixel::Color(128, 0, 0)
#define YELLOW  Adafruit_NeoPixel::Color(128, 128, 0)
#define GREEN   Adafruit_NeoPixel::Color(0, 128, 0)
#define PURPLE  Adafruit_NeoPixel::Color(128,0,128)

// Configuration for our LED strip and Arduino
#define NUM_LEDS 150
#define PIN 4

#define FADE_MAX_BRIGHTNESS     100
#define FLASH_TIME_INTERVAL     100

// Commands
#define COMMAND_RESET           '8'
#define COMMAND_OFF             '0'
#define COMMAND_COLOR_PURPLE    'a'

// Effects
enum {
  SOLID,
  FAST_BLINK,
  SLOW_BLINK,
  FADE,
  SPARTRONICS_FADE,
  COGS,
  FLASH
};

uint32_t blingColor = OFF;
int blingEffect = SOLID;
int blingBrightness = 100;

// allocate our pixel memory, set interface to match our hardware
// Use the new DMA NeoPixel library for the SAMD21 MCU on the Trinket M0
Adafruit_NeoPixel_ZeroDMA pixels(NUM_LEDS, PIN, NEO_GRB+NEO_KHZ800);

// "Magic" delay macro to exit a function if a new command is available
// Execute the delay 10ms at a time, and check for available bytes
#define INTERRUPTABLE_DELAY(x)  {                               \
  int timeInterval = (x);                                       \
  while (timeInterval > 0) {                                    \
    if (checkForCommand()) {                                    \
      return;                                                   \
    }                                                           \
    delay(min(10, timeInterval));                               \
    timeInterval = timeInterval - min(10, timeInterval);        \
  }                                                             \
}

// A global variable to hold the current command to be executed
char currentCommand = '\0';

// Check for a new command byte, and return true if one is found
bool checkForCommand(void)
{
  bool result = false;
  char newCommand;

  // Preset our "new command" variable to be the same as the current command
  newCommand = currentCommand;
  // If there are any characters available on the serial port, read them
  while (Serial.available() > 0)
  {
    char tempCommand;
    tempCommand = Serial.read();
    if (isalnum(tempCommand))
    {
      Serial.print("Got command: ");
      Serial.print(tempCommand);
      Serial.println("");
      newCommand = tempCommand;
    }
  }

  // Only accept the command if it is alpha-numeric, and if it is different
  // from the current command being executed.
  if (isalnum(newCommand) && (newCommand != currentCommand))
  {
    result = true;
    currentCommand = newCommand;
  }

  return result;
}

// Fill the strip with a single color
void fillStrip(uint32_t c)
{
   for (int i=0; i<NUM_LEDS; i++)
   {
      pixels.setPixelColor(i, c);
   }
   pixels.show();
}

/*******************************************************************************
 * Functions to execute the bling "actions"
 ******************************************************************************/

// Solid illumination of a single color
void solid(int color, int brightness)
{
  Serial.println("solid()");
  pixels.setBrightness(brightness);
  fillStrip(color);
  while (1)
  {
    INTERRUPTABLE_DELAY(10);
  }
}

// Flash of light for a short period of time, then off (runs once only)
void flash(int timemInterval, int color, int brightness)
{
  Serial.println("flash()");
  pixels.setBrightness(brightness);
  fillStrip(color);
  INTERRUPTABLE_DELAY(timeInterval);
  currentCommand = COMMAND_RESET;
}

// Blink a single color on and off at the given time interval
void blink(int timeInterval, int color, int brightness)
{
  Serial.println("blink()");
  pixels.setBrightness(brightness);
  while (1)
  {
    fillStrip(color);
    INTERRUPTABLE_DELAY(timeInterval);
    fillStrip(OFF);
    INTERRUPTABLE_DELAY(timeInterval);
  }
}

// Fade a color in and out
void fade(int timeInterval, int color, int maxBrightness)
{
  bool brightening = true;
  int brightness = 0;

  Serial.println("fade()");
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
void spartronics_fade(int timeInterval, int color1, int color2, int maxBrightness)
{
  Serial.println("spartronics_fade()");
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


// Classic Spartronics Cogs
void cogs(int timeInterval, int color1, int color2)
{
  Serial.println("cogs");
  while (1)
  {
    INTERRUPTABLE_DELAY(timeInterval);
  }
}


/*******************************************************************************
 * Arduino setup() and loop()
 ******************************************************************************/

void setup() {
  Serial.begin(9600);
  pixels.begin(); // initialize neopixel library
  fillStrip(OFF);
  Serial.println("Hello LEDS!");
  blingColor = OFF;
  blingEffect = SOLID;
}

void loop()
{
  int effectDelay = 10; // Default effect delay

  // Take the current command and set the parameters
  switch (currentCommand)
  {
    // Bling effects
    case '5':
      blingEffect = SPARTRONICS_FADE;
      effectDelay = 10;
      break;
    case '6':
      blingEffect = FADE;
      effectDelay = 10;
      break;
    case '7':
      blingEffect = FAST_BLINK;
      effectDelay = 150;
      break;
    case '9':
      blingEffect = SLOW_BLINK;
      effectDelay = 500;
      break;
    case 'f':
      blingEffect = FLASH;
      blingColor = RED;
      effectDelay = 750;
      break;
    case 'c':
      blingEffect = COGS;
      effectDelay = 250;
      break;

    // Bling colors
    case 'a':
      blingColor = PURPLE;
      break;
    case COMMAND_OFF:
      blingColor = OFF;
      break;
    case '1':
      blingColor = BLUE;
      break;
    case '2':
      blingColor = YELLOW;
      break;
    case '3':
      blingColor = RED;
      break;
    case '4':
      blingColor = GREEN;
      break;

    // Reset everything
    case '8':
      //resets all effects
      blingColor = OFF;
      blingEffect = SOLID;
      effectDelay = 10;
      blingBrightness = 100;
      pixels.setBrightness(blingBrightness);
      break;

    default:
      // Unknown command!
      currentCommand = '\0';
      blingColor = OFF;
      blingEffect = SOLID;
      effectDelay = 10;
      blingBrightness = 100;
      pixels.setBrightness(blingBrightness);
  }

  // Now we run the blingEffect selected until the next command is sent
  switch (blingEffect)
  {
    case SOLID:
      solid(blingColor, blingBrightness);
      break;
    case FLASH:
      flash(effectDelay, blingColor, blingBrightness);
      break;
    case SLOW_BLINK:
    case FAST_BLINK:
      blink(effectDelay, blingColor, blingBrightness);
      break;
    case FADE:
      fade(effectDelay, blingColor, FADE_MAX_BRIGHTNESS);
      break;
    case SPARTRONICS_FADE:
      spartronics_fade(effectDelay, YELLOW, BLUE, FADE_MAX_BRIGHTNESS);
      break;
    case COGS:
      cogs(effectDelay, YELLOW, BLUE);
      break;
    default:
      // Unknown bling effect
      solid(OFF, 0);
  }
}
/* end of loop func */
