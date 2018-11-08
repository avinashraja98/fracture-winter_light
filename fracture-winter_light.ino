/*
   Fracture - Winter Lights
   Demo code to show the lights in action
   Author: Avinash Raja Amarchand
   Email: aramarchand@mun.ca
*/

// ShiftPWM uses timer1 by default. To use a different timer, before '#include <ShiftPWM.h>', add
#define SHIFTPWM_USE_TIMER2  // for Arduino Uno and earlier (Atmega328)
//#define SHIFTPWM_USE_TIMER3 // for Arduino Micro/Leonardo (Atmega32u4)                      //<------- Have to be set according to the hardware used (Default: TIMER2 was enabled)

// Clock and data pins are pins from the hardware SPI, you cannot choose them yourself.
// Data pin is MOSI (Uno and earlier: 11, Leonardo: ICSP 4, Mega: 51, Teensy 2.0: 2, Teensy 2.0++: 22)
// Clock pin is SCK (Uno and earlier: 13, Leonardo: ICSP 3, Mega: 52, Teensy 2.0: 1, Teensy 2.0++: 21)

// You can choose the latch pin yourself.
const int ShiftPWM_latchPin = 8;

// ** uncomment this part to NOT use the SPI port and change the pin numbers. This is 2.5x slower **     //<------- I uncommented these three lines to make it work with the particular shift register I had. Can be commented to use max speed. (Default: Was commented)
//#define SHIFTPWM_NOSPI
//const int ShiftPWM_dataPin = 5;
//const int ShiftPWM_clockPin = 6;

// If your LED's turn on if the pin is low, set this to true, otherwise set it to false.
const bool ShiftPWM_invertOutputs = false;

// You can enable the option below to shift the PWM phase of each shift register by 8 compared to the previous.
// This will slightly increase the interrupt load, but will prevent all PWM signals from becoming high at the same time.
// This will be a bit easier on your power supply, because the current peaks are distributed.
const bool ShiftPWM_balanceLoad = false;

#include <ShiftPWM.h> // include ShiftPWM.h after setting the pins!

//****************************************************************************************************
//^^^^^^^Settings above this line should suit hardware used^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//****************************************************************************************************

// Function prototypes (telling the compiler these functions exist).
void setZero(void);
void setMax(void);
void fadeInOneByOne(void);
void fadeInOneByOne(int[]);
void oneByOne(void);
void inOutAll(void);
void scrambleArray(int*, int, int);
void printInstructions(void);
void printStatus(void);

// Here you set the number of brightness levels, the update frequency and the number of shift registers.
// These values affect the load of ShiftPWM.
// Choose them wisely and use the PrintInterruptLoad() function to verify your load.
unsigned char maxBrightness = 255;
unsigned char pwmFrequency = 75;               //<------- Reduce this number to prevent timing issues and random crashes. Uses less CPU when reduced. (Default: 75)
unsigned const int numRegisters = 4;           //<------- Set the number of shift registers used
unsigned const int numOutputs = numRegisters * 8;
unsigned int numRGBLeds = numRegisters * 8 / 3;
unsigned int fadingMode = 0; //start with all LED's off.

unsigned long startTime = 0; // start time for the chosen fading mode
int ledCount = 1;

struct lightInfo // The structure that saves brightness information of the lights.
{
  unsigned int brightness;
} lights[numOutputs];

int customList[] = {4, 2, 3, 5, 8, 6, 1}; // This list is set to control the order in which the lights will light up. (Conditions: should be in the range of the outputs, ie if the number of shift registers are 3, the list should contain numbers between [1,24] inclding the edges)

// Variables to control random light selection
int currentLED[numOutputs];
int previous = 0;
int count = 0;

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

  // Used to generate random values every time the sketch is run.
  randomSeed(analogRead(0));

  // Populates the random sequence array with the number of outputs present.
  for (int i = 0; i < numOutputs; i++)
    currentLED[i] = i;

  // Scrambles the array to get a random sequence
  scrambleArray(currentLED, 0, numOutputs);

  // Initialize all lights to brightness 0
  for (int i = 0; i < numOutputs; i++)
  {
    lights[i].brightness = 0;
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
        case 4:
          Serial.println("Fade in one by one");
          break;
        case 5:
          Serial.println("Fade in one by one (customList)");
          break;
        case 6:
          Serial.println("Fade in one by one (Random)");
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
    case 4:
      fadeInOneByOne();
      break;
    case 5:
      fadeInOneByOne(customList);
      break;
    case 6:
      fadeInOneByOneRandom();
      break;
    default:
      Serial.println("Unknown Mode!");
      delay(1000);
      break;
  }

  // Handle Lights, checks the structure every loop and updates the network.
  for (int i = 0; i < numOutputs; i++)
  {
    ShiftPWM.SetOne(i, lights[i].brightness);
  }
}

// Function to turn all lights off
void setZero(void)
{
  for (int i = 0; i < numOutputs; i++)
  {
    lights[i].brightness = 0;
  }
  ShiftPWM.SetAll(0);
  ledCount = 1;
}

// Function to turn all lights on and to the max brightness
void setMax(void)
{
  for (int i = 0; i < numOutputs; i++)
  {
    lights[i].brightness = maxBrightness;
  }
  ShiftPWM.SetAll(maxBrightness);
}

int percent = 40;
int prevBri = 0;

void fadeInOneByOne(void)
{ // Fade in all outputs
  
  for (int k = 0; k < (maxBrightness*((double)percent/100)*(numOutputs))+1000; k++)
  {
    delay(5);
    unsigned long brightness = k;
    if (brightness % (int)(maxBrightness * percent * 0.01) >= (int)(maxBrightness * percent * 0.01) - 1 && prevBri != (brightness % (int)(maxBrightness * percent * 0.01)) && ledCount < numOutputs)
    {
      ledCount++;
    }
    prevBri = brightness % (int)(maxBrightness * percent * 0.01);
    for (int i = 0; i < ledCount; i++)
    {
      int instBright = (brightness)-((int)((maxBrightness*percent*0.01)-1)*i);
      if (instBright <= maxBrightness)
      {
        lights[i].brightness = instBright;
        ShiftPWM.SetOne(i, lights[i].brightness);
        //Serial.print(instBright);
      }
      else
      {
        lights[i].brightness = maxBrightness;
        ShiftPWM.SetOne(i, lights[i].brightness);
        //Serial.print("255");
      }
      //Serial.print(" ");
    }
    //Serial.print(k);Serial.println(ledCount);
  }
  ledCount = 1;
  prevBri = 0;
  fadingMode = 1;
}

/*
  void fadeInOneByOne(void)
  { // Fade in all outputs one at a time
  //delay(50);
  unsigned char brightness;
  unsigned long fadeTime = 10000;                     // Time taken to fade a single output
  unsigned long loopTime = numOutputs * fadeTime ;  // Total time taken for the loop to complete
  unsigned long time = millis() - startTime;        // Calculates time from command issue to current time.
  unsigned long timer = time % loopTime;

  for(int i=0;i<numOutputs;i++)
  {
    unsigned long bright = (timer * 255 / ((loopTime/numOutputs)*(i+1)));
    if(bright<=255)
    {
      lights[i].brightness = bright;
      //Serial.print(bright);Serial.print(" ");
    }
    else
    {
      lights[i].brightness = 255;
      //Serial.print("255 ");
    }
  }
  //Serial.println();
  //printStatus();
  /*
  if (lights[activeLED].brightness < maxBrightness - 1) // Used to prevent making a fully bright led go to 0 and then increase again to max when the loop starts again
    lights[activeLED].brightness = brightness;

  }
*/
void fadeInOneByOne(int list[])
{ // Fade in all outputs one at a time. Same as previous but takes in the custom list and follows that order.
  unsigned char brightness;
  unsigned long fadeTime = 500;                     // Time taken to fade a single output
  unsigned long loopTime = numOutputs * fadeTime ;  // Total time taken for the loop to complete
  unsigned long time = millis() - startTime;        // Calculates time from command issue to current time.
  unsigned long timer = time % loopTime;            // Calculates the number of steps remaining till loop completion
  unsigned long currentStep = timer % (fadeTime);   // Calculates the current step of the current fade sequence

  int activeLED = timer / (fadeTime);               // From timer which tracks progress till loop completion, and the number of times fadeTime has passed, active led can be found.

  if (currentStep <= fadeTime)                      // Make sure currentStep is in range
  {
    brightness = currentStep * maxBrightness / fadeTime; ///fading in
  }
  else
  {
    brightness = maxBrightness;// - (currentStep - fadeTime) * maxBrightness / fadeTime; ///fading out;
  }
  if (lights[list[activeLED] - 1].brightness < maxBrightness - 1) // Used to prevent making a fully bright led go to 0 and then increase again to max when the loop starts again
    lights[list[activeLED] - 1].brightness = brightness;
}

void fadeInOneByOneRandom(void)
{ // Fade in all outputs one at a time. Same as previous but uses the randomly generated sequence as the order.
  unsigned char brightness;
  unsigned long fadeTime = 500;                     // Time taken to fade a single output
  unsigned long loopTime = numOutputs * fadeTime ;  // Total time taken for the loop to complete
  unsigned long time = millis() - startTime;        // Calculates time from command issue to current time.
  unsigned long timer = time % loopTime;            // Calculates the number of steps remaining till loop completion
  unsigned long currentStep = timer % (fadeTime);   // Calculates the current step of the current fade sequence

  int activeLED = timer / (fadeTime);               // From timer which tracks progress till loop completion, and the number of times fadeTime has passed, active led can be found.

  if (currentStep <= fadeTime)                      // Make sure currentStep is in range
  {
    brightness = currentStep * maxBrightness / fadeTime; ///fading in
  }
  else
  {
    brightness = maxBrightness;// - (currentStep - fadeTime) * maxBrightness / fadeTime; ///fading out;
  }

  if (activeLED != previous)                        // When the activeLED changes
  {
    previous = activeLED;
    if (activeLED == 0)                             // If the sqeuence is completed and is now starting from zero,
    {
      scrambleArray(currentLED, 0, numOutputs);     // Generate a new random sequence and follow that order
    }
  }

  if (lights[currentLED[activeLED]].brightness < maxBrightness - 1) // Used to prevent making a fully bright led go to 0 and then increase again to max when the loop starts again
    lights[currentLED[activeLED]].brightness = brightness;
}

void oneByOne(void)
{ // Fade in and fade out all outputs one at a time
  unsigned char brightness;
  unsigned long fadeTime = 500;                       // Time taken to fade a single output
  unsigned long loopTime = numOutputs * fadeTime * 2; // Total time taken for the loop to complete. Multiplied by 2 to consider fade out time too.
  unsigned long time = millis() - startTime;          // Calculates time from command issue to current time.
  unsigned long timer = time % loopTime;              // Calculates the number of steps remaining till loop completion
  unsigned long currentStep = timer % (fadeTime * 2); // Calculates the current step of the current fade sequence. Multiplied by 2 to consider fade out time too.

  int activeLED = timer / (fadeTime * 2);             // From timer which tracks progress till loop completion, and the number of times fadeTime has passed, active led can be found. Multiplied by 2 to consider fade out time too.

  if (currentStep <= fadeTime)                        // Make sure currentStep is in fade in range
  {
    brightness = currentStep * maxBrightness / fadeTime; //fading in
  }
  else
  {
    brightness = maxBrightness - (currentStep - fadeTime) * maxBrightness / fadeTime; //fading out;
  }
  setZero();                                          // Set all lights to zero to create fade in out effect
  lights[activeLED].brightness = brightness;
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
  }
}

void scrambleArray(int * array, int lower, int upper) //https://forum.arduino.cc/index.php?topic=225567.msg1634951#msg1634951
{
  int last = lower;
  int temp = array[last];
  for (int i = 0; i < (upper - lower); ++i)
  {
    int index = random(lower, upper);
    array[last] = array[index];
    last = index;
  }
  array[last] = temp;
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
  Serial.println("  4. Fade in one by one");
  Serial.println("  5. Fade in one by one (customList)");
  Serial.println("  6. Fade in one by one (Random)");
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
    Serial.println(lights[i].brightness);
  }
  Serial.println();
}
