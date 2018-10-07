/*
   Fracture - Winter Lights
   Demo code to show the lights in action

   Author: Avinash Raja Amarchand
   Email: aramarchand@mun.ca
*/

// ShiftPWM uses timer1 by default. To use a different timer, before '#include <ShiftPWM.h>', add
// #define SHIFTPWM_USE_TIMER2  // for Arduino Uno and earlier (Atmega328)
#define SHIFTPWM_USE_TIMER3 // for Arduino Micro/Leonardo (Atmega32u4)

// Clock and data pins are pins from the hardware SPI, you cannot choose them yourself.
// Data pin is MOSI (Uno and earlier: 11, Leonardo: ICSP 4, Mega: 51, Teensy 2.0: 2, Teensy 2.0++: 22)
// Clock pin is SCK (Uno and earlier: 13, Leonardo: ICSP 3, Mega: 52, Teensy 2.0: 1, Teensy 2.0++: 21)

// You can choose the latch pin yourself.
const int ShiftPWM_latchPin = 8;

// ** uncomment this part to NOT use the SPI port and change the pin numbers. This is 2.5x slower **
#define SHIFTPWM_NOSPI
const int ShiftPWM_dataPin = 5;
const int ShiftPWM_clockPin = 6;

// If your LED's turn on if the pin is low, set this to true, otherwise set it to false.
const bool ShiftPWM_invertOutputs = false;

// You can enable the option below to shift the PWM phase of each shift register by 8 compared to the previous.
// This will slightly increase the interrupt load, but will prevent all PWM signals from becoming high at the same time.
// This will be a bit easier on your power supply, because the current peaks are distributed.
const bool ShiftPWM_balanceLoad = false;

#include "ShiftPWM.h" // include ShiftPWM.h after setting the pins!

//****************************************************************************************************
//^^^^^^^Settings above this line should suit hardware used^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//****************************************************************************************************

// Function prototypes (telling the compiler these functions exist).
void setZero(void);
void setMax(void);
void oneByOne(void);
void inOutAll(void);
void printInstructions(void);
void printStatus(void);

// Here you set the number of brightness levels, the update frequency and the number of shift registers.
// These values affect the load of ShiftPWM.
// Choose them wisely and use the PrintInterruptLoad() function to verify your load.
unsigned char maxBrightness = 255;
unsigned char pwmFrequency = 75;
unsigned const int numRegisters = 1; //<<<------ Set the number of shift registers used
unsigned const int numOutputs = numRegisters * 8;
unsigned int numRGBLeds = numRegisters * 8 / 3;
unsigned int fadingMode = 0; //start with all LED's off.

unsigned long startTime = 0; // start time for the chosen fading mode

struct lightInfo
{
  unsigned int brightness;
  boolean power;
} lights[numOutputs];

void setup()
{
  while (!Serial)
  {
    delay(100);
  }
  Serial.begin(9600);

  // Sets the number of 8-bit registers that are used.
  ShiftPWM.SetAmountOfRegisters(numRegisters);

  // SetPinGrouping allows flexibility in LED setup.
  // If your LED's are connected like this: RRRRGGGGBBBBRRRRGGGGBBBB, use SetPinGrouping(4).
  ShiftPWM.SetPinGrouping(1); //This is the default, but I added here to demonstrate how to use the funtion

  ShiftPWM.Start(pwmFrequency, maxBrightness);

  for (int i = 0; i < numOutputs; i++)
  {
    lights[i].brightness = 0;
    lights[i].power = false;
  }

  printInstructions();
}

void loop()
{
  if (Serial.available())
  {
    if (Serial.peek() == 'l')
    {
      // Print information about the interrupt frequency, duration and load on your program
      ShiftPWM.PrintInterruptLoad();
    }
    else if (Serial.peek() == 'm')
    {
      // Print instructions again
      printInstructions();
    }
    else if (Serial.peek() == 's')
    {
      // Print status
      printStatus();
    }
    else
    {
      fadingMode = Serial.parseInt(); // read a number from the serial port to set the mode
      Serial.print("Mode set to ");
      Serial.print(fadingMode);
      Serial.print(": ");
      startTime = millis();
      switch (fadingMode)
      {
      case 0:
        Serial.println("All LED's off");
        break;
      case 1:
        Serial.println("All LED's on");
        break;
      case 2:
        Serial.println("Fade in and out one by one");
        break;
      case 3:
        Serial.println("Fade in and out all LED's");
        break;
      default:
        Serial.println("Unknown mode!");
        break;
      }
    }
    while (Serial.read() >= 0)
    {
      ; // flush remaining characters
    }
  }
  switch (fadingMode)
  {
  case 0:
    // Turn all LED's off.
    setZero();
    break;
  case 1:
    // Turn all LED's on.
    setMax();
    break;
  case 2:
    oneByOne();
    break;
  case 3:
    inOutAll();
    break;
  default:
    Serial.println("Unknown Mode!");
    delay(1000);
    break;
  }

  // Handle Lights
  for (int i = 0; i < numOutputs; i++)
  {
    if (lights[i].power)
    {
      ShiftPWM.SetOne(i, lights[i].brightness);
    }
    else
    {
      ShiftPWM.SetOne(i, 0);
    }
  }
}

void setZero(void)
{
  for (int i = 0; i < numOutputs; i++)
  {
    lights[i].power = false;
  }
  ShiftPWM.SetAll(0);
}

void setMax(void)
{
  for (int i = 0; i < numOutputs; i++)
  {
    lights[i].power = true;
    lights[i].brightness = 255;
  }
  ShiftPWM.SetAll(255);    
}

void oneByOne(void)
{ // Fade in and fade out all outputs one at a time
  unsigned char brightness;
  unsigned long fadeTime = 500;
  unsigned long loopTime = numOutputs * fadeTime * 2;
  unsigned long time = millis() - startTime;
  unsigned long timer = time % loopTime;
  unsigned long currentStep = timer % (fadeTime * 2);

  int activeLED = timer / (fadeTime * 2);

  if (currentStep <= fadeTime)
  {
    brightness = currentStep * maxBrightness / fadeTime; ///fading in
  }
  else
  {
    brightness = maxBrightness - (currentStep - fadeTime) * maxBrightness / fadeTime; ///fading out;
  }
  //ShiftPWM.SetAll(0);
  setZero();
  //ShiftPWM.SetOne(activeLED, brightness);
  lights[activeLED].brightness = brightness;
  lights[activeLED].power = true;
}

void inOutAll(void)
{ // Fade in all outputs
  unsigned char brightness;
  unsigned long fadeTime = 2000;
  unsigned long time = millis() - startTime;
  unsigned long currentStep = time % (fadeTime * 2);

  if (currentStep <= fadeTime)
  {
    brightness = currentStep * maxBrightness / fadeTime; ///fading in
  }
  else
  {
    brightness = maxBrightness - (currentStep - fadeTime) * maxBrightness / fadeTime; ///fading out;
  }
  //ShiftPWM.SetAll(brightness);
  for (int i = 0; i < numOutputs; i++)
  {
    lights[i].brightness = brightness;
    lights[i].power = true;
  }
}

void printInstructions(void)
{
  Serial.println("---- Fracture - Winter Light Demo ----");
  Serial.println("");

  Serial.println("Type 'l' to see the load of the ShiftPWM interrupt (the % of CPU time the AVR is busy with ShiftPWM)");
  Serial.println("");
  Serial.println("Type any of these numbers to set the demo to this mode:");
  Serial.println("  0. All LED's off");
  Serial.println("  1. All LED's on");
  Serial.println("  2. Fade in and out one by one");
  Serial.println("  3. Fade in and out all LED's");
  Serial.println("");
  Serial.println("Type 'm' to see this info again");
  Serial.println("Type 's' to see light network status");
  Serial.println("");
  Serial.println("----");
}

void printStatus(void)
{
  for (int i = 0; i < numOutputs; i++)
  {
    Serial.print("Light:");
    Serial.print(i + 1);
    Serial.print(" ,Brightness:");
    Serial.print(lights[i].brightness);
    Serial.print(" ,Power:");
    Serial.println(lights[i].power);
  }
  Serial.println();
}

