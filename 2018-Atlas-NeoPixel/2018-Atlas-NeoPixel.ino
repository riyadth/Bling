#include <Adafruit_NeoPixel.h>
#include <ctype.h>

// Configuration for our LED strip and Arduino
#define NUM_LEDS 150
#define PIN 11

#define FADE_MAX_BRIGHTNESS     100
#define FLASH_TIME_INTERVAL     100

// allocate our pixel memory, set interface to match our hardware
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// Colors
const uint32_t OFF     = pixels.Color(0,0,0);
const uint32_t RED     = pixels.Color(128, 0, 0);
const uint32_t GREEN   = pixels.Color(0, 128, 0);
const uint32_t BLUE    = pixels.Color(0, 0, 128);
const uint32_t YELLOW  = pixels.Color(128, 128, 0);
const uint32_t PURPLE  = pixels.Color(128,0,128);


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
unsigned long effectDelay = 10;
int blingBrightness = 100;




// "Magic" delay macro to exit a function if a new command is available
// Execute the delay 10ms at a time, and check for available bytes
#define INTERRUPTABLE_DELAY(x)  {                               \
  unsigned long timeIntervalCounter = (x);                      \
  while (timeIntervalCounter != 0) {                            \
    if (timeIntervalCounter > 10) {                             \
      delay(10);                                                \
      timeIntervalCounter = timeIntervalCounter - 10;           \
    } else {                                                    \
      delay(timeIntervalCounter);                               \
      timeIntervalCounter = 0;                                  \
    }                                                           \
    if (checkForCommand() == true) {                            \
      return;                                                   \
    }                                                           \
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
    if (isprint(tempCommand))
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
   for (unsigned i=0; i<pixels.numPixels(); i++)
   {
      pixels.setPixelColor(i, c);
   }
   pixels.show();
}

/*******************************************************************************
 * Functions to execute the bling "actions"
 ******************************************************************************/

// Solid illumination of a single color
void solid(uint32_t color, uint8_t brightness)
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
void flash(unsigned long timeInterval, uint32_t color, uint8_t brightness)
{
  Serial.println("flash()");
  pixels.setBrightness(brightness);
  fillStrip(color);
  delay(timeInterval);
  Serial.println("Done with flash");
  currentCommand = COMMAND_RESET;
}

// Blink a single color on and off at the given time interval
void blink(unsigned long timeInterval, uint32_t color, uint8_t brightness)
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
void fade(unsigned long timeInterval, uint32_t color, uint8_t maxBrightness)
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
void spartronics_fade(unsigned long timeInterval, uint32_t color1, uint32_t color2, uint8_t maxBrightness)
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
void cogs(unsigned long timeInterval, uint32_t color1, uint32_t color2)
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
      effectDelay = 1500;
      break;
    case 'f':
      blingEffect = FLASH;
      blingColor = RED;
      effectDelay = 100;
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

    // Adjust brightness
    // FIXME: Does not work... Not isalnum()
    // Also, needs to be repeatable...
    // Could set currentCommand to lastCommand..
    // Alternative: Fixed brightnesses 0..9?
    case '+':
      blingBrightness = blingBrightness + 10;
      if (blingBrightness > 255)
      {
        blingBrightness = 255;
      }
      break;
    case '-':
      blingBrightness = blingBrightness - 10;
      if (blingBrightness < 0)
      {
        blingBrightness = 0;
      }
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
      Serial.print("Unknown command: ");
      Serial.println(currentCommand);
      currentCommand = '\0';
      blingColor = OFF;
      blingEffect = SOLID;
      effectDelay = 10;
      blingBrightness = 100;
      pixels.setBrightness(blingBrightness);
      pixels.clear();
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
      Serial.print("Unknown effect: ");
      Serial.println(blingEffect);
      solid(OFF, 0);
  }
}
/* end of loop func */
