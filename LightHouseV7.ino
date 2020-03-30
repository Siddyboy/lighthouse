/*
  Light House
  25/12/12 Martin Kingsley
  
  Lighthouse at Bedtime.
  
  This program will scan the light house beam for a fixed period of time after "lights out" at bed time
  (thought there is no intelligence in the time aspect, future work). (Not done... If the lights go back on then the light will stop scanning.)
  Could make more complex e.g. not restart after a light on event but enough for now.
  
  Hardware description,
    There are three significant pieces of hardware
    a) Light sensors, simple light dependent resistors, three off to make tolerant of different lighting conditions and positions.
    b) Light House LED, direct drive from the mega328
    c) Motor drive, simple single transistor with fly wheel/snubber diode, to drive rotating mirror to scan light house beam.
    
  Software description
    The software consists of two, no three, sections.
    The first section reads light sensor values and exits to next section
    if light level drops quickly and sufficiently enough.
    3/1/13 If the sensors read zero, or close to zero it will trigger anyway.
    The second section turns LED and motor on for a number of minutes.
    The optic LED is reduced in intensity based on the amount of time left to run
    by using the map() function.
    
    3/1/2013 Third software section added after optic has run to remain turned off until the light level goes up again.
    After testing it is apparent that can't rely on the while loop termination in the first section to not continually retrigger
    as the LDR reading are zero and the maths doesn't work. There is also a lot of dodgy variable casting going on there too.

    11/1/2013 Changed light duration to five minutes from ten minutes at Thomas' request. Saved as V7.

*/
 

// Include libraries
#include <Time.h> //Using time library to avoid roll over problems with millis function.

// Define Interface Pins.
const int opticRotationDrive = 3;
const int opticIntensityDrive = 6;
const int onBoardLed = 13;
const int lightSensor1 = 1;
const int lightSensor2 = 2;
const int lightSensor3 = 3;


// Define Constants
const boolean ON = HIGH;
const boolean OFF = LOW;
const boolean debugOn = false;

void setup()
{
  if(debugOn)  Serial.begin(115200);           //Initialize serial port to send and recieve at 115200 baud

  analogWrite(opticRotationDrive, 0);         //Set motor control HW to safe state
  analogWrite(opticIntensityDrive, 0);         //Set LED control HW to safe state
  pinMode(onBoardLed, OUTPUT);
  
  if (debugOn) Serial.println("Reset"); 

//Flash LED to show powered up ok.
for ( int numberOfTimes = 3; numberOfTimes > 0; numberOfTimes--)
    {

        analogWrite(opticIntensityDrive, 100);
        digitalWrite(onBoardLed, ON);
        delay(50);
        analogWrite(opticIntensityDrive, 0);
        digitalWrite(onBoardLed, OFF);
        delay(100);

    } 

}

void loop()
{
  //Configuration constants
  const int lightSensorAverageDecay = 10;
  const int lightSensorReadDelay = 1000; //in ms.
  const float lightSensorTriggerLevel = .2;
  const int opticIntensity = 255; //Optic intensity start value, 0-255 PWM value
  const int opticRotationRate = 10; //0-127, x 2 headroom for startup, may not keep running if much less than 10
  const int opticRotationRunUp = 1000; //Motor run up time, ms
  const time_t opticRunDuration = 60*5; //in seconds
  
  //Variable Declarations
  int lightSensorMaxLevel = 0;
  float lightSensorMaxAverage = 0;
  boolean heartBeat = ON;
  time_t opticStartTime= 0;
  time_t opticStopTime = 0;

  

/* First Section
This section reads light sensor values and exits to next section
if light level drops quickly and sufficiently enough.
*/

lightSensorMaxAverage = getLightSensorMaxLevel(); //seed average light level value

do
  {
    heartBeat = !heartBeat; //Toggle heart beat
    digitalWrite(onBoardLed, heartBeat);

    delay(lightSensorReadDelay); //Change this to sleep to save power.

    lightSensorMaxLevel = getLightSensorMaxLevel();  

    // Compute average value.
    lightSensorMaxAverage = ((lightSensorMaxAverage *(lightSensorAverageDecay-1))+lightSensorMaxLevel)/lightSensorAverageDecay;

    if(debugOn)
      {
        Serial.print('\n');
        Serial.print(lightSensorMaxLevel);
        Serial.print(",");
        Serial.print(lightSensorMaxAverage);
      }
    }
  while (lightSensorMaxLevel > (lightSensorMaxAverage * lightSensorTriggerLevel));

if(debugOn)
  {
    Serial.print('\n');
    Serial.print("triggered");
  }

/* Second Section
This section turn LED and motor on for a number of minutes.
The optic LED is reduced in intensity based on the amount of time left to run
by using the map() function.
*/

    analogWrite(opticRotationDrive, opticRotationRate * 2); //Run up motor
    delay(opticRotationRunUp);                              //Then back off to running level
    analogWrite(opticRotationDrive, opticRotationRate);     
    
    opticStartTime = now();
    opticStopTime = opticStartTime + opticRunDuration;

    if(debugOn)
    {
      Serial.print('\n');
      Serial.print(opticStartTime);
      Serial.print(",");
      Serial.print(opticStopTime);
    }
          
    do
    {
      analogWrite(opticIntensityDrive, map(now(), opticStopTime, opticStartTime, 0, opticIntensity)); // Optic Intensity mapped to run time     
    }
    while(now() < opticStopTime);
    
    analogWrite(opticIntensityDrive, 0); //Turn LED and motor off
    analogWrite(opticRotationDrive, 0);
    
/* Third Section
This section waits in the dark doing nothing until it gets light again
at which point it contiues, the main loop finishes and the process starts again at the begining of the main loop
*/

lightSensorMaxAverage = getLightSensorMaxLevel(); //seed average light level value

do
  {
    heartBeat = OFF; //Toggle heart beat
    digitalWrite(onBoardLed, heartBeat);

    delay(lightSensorReadDelay); //Change this to sleep to save power.
    
    heartBeat = ON; //Toggle heart beat
    digitalWrite(onBoardLed, heartBeat);

    lightSensorMaxLevel = getLightSensorMaxLevel();  

    // Compute average value.
    lightSensorMaxAverage = ((lightSensorMaxAverage *(lightSensorAverageDecay-1))+lightSensorMaxLevel)/lightSensorAverageDecay;

    if(debugOn)
      {
        Serial.print('\n');
        Serial.print(lightSensorMaxLevel);
        Serial.print(",");
        Serial.print(lightSensorMaxAverage);
      }
    }
  while (lightSensorMaxAverage < 100);

if(debugOn)
  {
    Serial.print('\n');
    Serial.print("sun arise, early in di mornin'");
  }

// Rinse and repeat, loop() repeats automatically.
}



//Function Definitions ******************************************************************************************************

int getLightSensorMaxLevel()
{
  const int lightSensorReadDelay = 0;
  int lightLevel1 = 0;
  int lightLevel2 = 0;
  int lightLevel3 = 0;  
  int lightSensorMaxLevel = 0;
  
  //Read light sensor values
  delay(lightSensorReadDelay);
  lightLevel1 = analogRead(lightSensor1);
  delay(lightSensorReadDelay);
  lightLevel2 = analogRead(lightSensor2);
  delay(lightSensorReadDelay);
  lightLevel3 = analogRead(lightSensor3);

  //Select maximum value
  lightSensorMaxLevel = max(lightLevel1, lightLevel2);
  lightSensorMaxLevel = max(lightSensorMaxLevel, lightLevel3);

  return lightSensorMaxLevel;
}
