/************************************************************************************************************************************
 * ShiftPWM blocking RGB fades example, (c) Elco Jacobs, updated August 2012.
 *
 * ShiftPWM blocking RGB fades example. This example uses simple delay loops to create fades.
 * If you want to change the fading mode based on inputs (sensors, buttons, serial), use the non-blocking example as a starting point.
 * Please go to www.elcojacobs.com/shiftpwm for documentation, fuction reference and schematics.
 * If you want to use ShiftPWM with LED strips or high power LED's, visit the shop for boards.
 ************************************************************************************************************************************/
 
// ShiftPWM uses timer1 by default. To use a different timer, before '#include <ShiftPWM.h>', add
 #define SHIFTPWM_USE_TIMER2  // for Arduino Uno and earlier (Atmega328)
// #define SHIFTPWM_USE_TIMER3  // for Arduino Micro/Leonardo (Atmega32u4)

// Clock and data pins are pins from the hardware SPI, you cannot choose them yourself if you use the hardware SPI.
// Data pin is MOSI (Uno and earlier: 11, Leonardo: ICSP 4, Mega: 51, Teensy 2.0: 2, Teensy 2.0++: 22) 
// Clock pin is SCK (Uno and earlier: 13, Leonardo: ICSP 3, Mega: 52, Teensy 2.0: 1, Teensy 2.0++: 21)

// You can choose the latch pin yourself.
const int ShiftPWM_latchPin=8;

// ** uncomment this part to NOT use the SPI port and change the pin numbers. This is 2.5x slower **
// #define SHIFTPWM_NOSPI
// const int ShiftPWM_dataPin = 11;
// const int ShiftPWM_clockPin = 13;


// If your LED's turn on if the pin is low, set this to true, otherwise set it to false.
const bool ShiftPWM_invertOutputs = false; 

// You can enable the option below to shift the PWM phase of each shift register by 8 compared to the previous.
// This will slightly increase the interrupt load, but will prevent all PWM signals from becoming high at the same time.
// This will be a bit easier on your power supply, because the current peaks are distributed.
const bool ShiftPWM_balanceLoad = false;

#include <ShiftPWM.h>   // include ShiftPWM.h after setting the pins!

// Here you set the number of brightness levels, the update frequency and the number of shift registers.
// These values affect the load of ShiftPWM.
// Choose them wisely and use the PrintInterruptLoad() function to verify your load.
// There is a calculator on my website to estimate the load.

unsigned char maxBrightness = 255;
unsigned char pwmFrequency = 75;
unsigned const int numRegisters = 1;
unsigned const int numOutputs = numRegisters * 8;

int ledCount = 1;

struct lightInfo // The structure that saves brightness information of the lights.
{
  unsigned int brightness;
} lights[numOutputs];

int customList[] = {4, 2, 3, 5, 8, 6, 1, 7}; // This list is set to control the order in which the lights will light up. (Conditions: should be in the range of the outputs, ie if the number of shift registers are 3, the list should contain numbers between [1,24] inclding the edges)

// Variables to control random light selection
int randomList[numOutputs];
int previous = 0;
int count = 0;

void setup(){
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

  // Used to generate random values every time the sketch is run.
  randomSeed(analogRead(0));

  
  // Populates the random sequence array with the number of outputs present.
  for (int i = 0; i < numOutputs; i++)
    randomList[i] = i+1;

  // Scrambles the array to get a random sequence
  scrambleArray(randomList, 0, numOutputs);
  
  ShiftPWM.Start(pwmFrequency,maxBrightness);
}

void loop()
{    
  // Turn all LED's off.
  setZero();
  
  // Print information about the interrupt frequency, duration and load on your program
  ShiftPWM.PrintInterruptLoad();

  // Fade in and fade out all outputs one by one fast. Usefull for testing your hardware. Use OneByOneSlow when this is going to fast.
  //ShiftPWM.OneByOneFast();

  delay(1000);

  setMax();

  delay(1000);

  setZero();

  delay(1000);

  inOutAll();
  printStatus();

  delay(1000);

  fadeInOneByOne();
  printStatus();

  delay(1000);

  fadeOutOneByOne();
  printStatus();

  delay(1000);  

  fadeInOneByOne(customList);
  printStatus();

  delay(1000);

  fadeOutOneByOne(customList);
  printStatus();

  delay(1000);

  fadeInOneByOne(randomList);
  printStatus();

  delay(1000);

  fadeOutOneByOne(randomList);
  printStatus();

  delay(1000);  
  
}

void inOutAll(void)
{ 
  // Fade in all outputs
  for(int j=0;j<maxBrightness;j++){
    ShiftPWM.SetAll(j);  
    delay(20);
  }

  delay(100);
  
  // Fade out all outputs
  for(int j=maxBrightness;j>=0;j--){
    ShiftPWM.SetAll(j);  
    delay(20);
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

  for (int k = 0; k < (maxBrightness * ((double)percent / 100) * (numOutputs)) + 1000; k++)
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
      int instBright = (brightness) - ((int)((maxBrightness * percent * 0.01) - 1) * i);
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
  setMax();
}

void fadeInOneByOne(int list[])
{ // Fade in all outputs

  for (int k = 0; k < (maxBrightness * ((double)percent / 100) * (numOutputs)) + 1000; k++)
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
      int instBright = (brightness) - ((int)((maxBrightness * percent * 0.01) - 1) * i);
      if (instBright <= maxBrightness)
      {
        lights[list[i] - 1].brightness = instBright;
        ShiftPWM.SetOne(list[i] - 1, lights[list[i] - 1].brightness);
        //Serial.print(instBright);
      }
      else
      {
        lights[list[i] - 1].brightness = maxBrightness;
        ShiftPWM.SetOne(list[i] - 1, lights[list[i] - 1].brightness);
        //Serial.print("255");
      }
      //Serial.print(" ");
    }
    //Serial.print(k);Serial.println(ledCount);
  }
  ledCount = 1;
  prevBri = 0;
  setMax();
}

void fadeOutOneByOne(void)
{ // Fade in all outputs

  for (int k = 0; k < (maxBrightness * ((double)percent / 100) * (numOutputs)) + 1000; k++)
  {
    delay(5);
    unsigned long brightness = k;
    if (brightness % (int)(maxBrightness * percent * 0.01) >= (int)(maxBrightness * percent * 0.01) - 1 && prevBri != (brightness % (int)(maxBrightness * percent * 0.01)) && ledCount < numOutputs)
    {
      ledCount++;
    }
    prevBri = brightness % (int)(maxBrightness * percent * 0.01);
    for (int i = ledCount-1; i >= 0; i--)
    {
      int instBright = (brightness) - ((int)((maxBrightness * percent * 0.01) - 1) * i);
      if(instBright < 0)
      {
        //lights[i].brightness = maxBrightness;
        ShiftPWM.SetOne(i, lights[i].brightness);
        Serial.print(lights[i].brightness);        
      }      
      else if (instBright <= maxBrightness)
      {
        lights[i].brightness = maxBrightness - instBright;
        ShiftPWM.SetOne(i, lights[i].brightness);
        Serial.print(maxBrightness - instBright);
      }
      else
      {
        lights[i].brightness = 0;
        ShiftPWM.SetOne(i, lights[i].brightness);
        Serial.print("0");
      }
      Serial.print(" ");
    }
    Serial.println(ledCount);
  }
  ledCount = 1;
  prevBri = 0;
  setZero();
}

void fadeOutOneByOne(int list[])
{ // Fade in all outputs

  for (int k = 0; k < (maxBrightness * ((double)percent / 100) * (numOutputs)) + 1000; k++)
  {
    delay(5);
    unsigned long brightness = k;
    if (brightness % (int)(maxBrightness * percent * 0.01) >= (int)(maxBrightness * percent * 0.01) - 1 && prevBri != (brightness % (int)(maxBrightness * percent * 0.01)) && ledCount < numOutputs)
    {
      ledCount++;
    }
    prevBri = brightness % (int)(maxBrightness * percent * 0.01);
    for (int i = ledCount-1; i >= 0; i--)
    {
      int instBright = (brightness) - ((int)((maxBrightness * percent * 0.01) - 1) * i);
      if(instBright < 0)
      {
        //lights[i].brightness = maxBrightness;
        ShiftPWM.SetOne(list[i] - 1, lights[list[i] - 1].brightness);
        Serial.print(lights[list[i] - 1].brightness);        
      }      
      else if (instBright <= maxBrightness)
      {
        lights[list[i] - 1].brightness = maxBrightness - instBright;
        ShiftPWM.SetOne(list[i] - 1, lights[list[i] - 1].brightness);
        Serial.print(maxBrightness - instBright);
      }
      else
      {
        lights[list[i]].brightness = 0;
        ShiftPWM.SetOne(list[i] - 1, lights[list[i] - 1].brightness);
        Serial.print("0");
      }
      Serial.print(" ");
    }
    Serial.println(ledCount);
  }
  ledCount = 1;
  prevBri = 0;
  setZero();
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
