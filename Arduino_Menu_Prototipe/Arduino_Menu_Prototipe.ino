/*  Arduino TFT Tutorial
    Program made by Dejan Nedelkovski,
    www.HowToMechatronics.com
*/

/*  This program uses the UTFT and URTouch libraries
    made by Henning Karlsen.
    You can find and download them at:
    www.RinkyDinkElectronics.com
*/
String str;

#include <UTFT.h>
#include <URTouch.h>

//Temperature sensor Library, integers and objects
#include <Wire.h>
#include "Adafruit_MLX90614.h"
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
int c, f, t = 0, g = 0;
int data;

//Sats Sensor Library, integers and Objects
#include "spo2_algorithm.h"
#define MAX_BRIGHTNESS 255
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//Arduino Uno doesn't have enough SRAM to store 100 samples of IR led data and red led data in 32-bit format
//To solve this problem, 16-bit MSB of the sampled data will be truncated. Samples become 16-bit data.
uint16_t irBuffer[100]; //infrared LED sensor data
uint16_t redBuffer[100];  //red LED sensor data
#else
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
#endif
int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid
byte pulseLED = 11; //Must be on PWM pin
byte readLED = 13; //Blinks with each data read


//Heart rate Sensor Library, integers and objects
#include "MAX30105.h"
#include "heartRate.h"
MAX30105 particleSensor;
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;



//==== Creating Objects
UTFT    myGLCD(ILI9341_16, 38, 39, 40, 41); //Parameters should be adjusted to your Display/Schield model
URTouch  myTouch( 6, 5, 4, 3, 2);
//==== Defining Variables
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];
extern uint8_t franklingothic_normal[];
int patienteiD = 1;

int x, y;
char currentPage, selectedUnit;
//Ultrasonic Sensor
const int VCC = 13;
const int trigPin = 11;
const int echoPin = 12;
long duration;

// RGB LEDs
const int redLed = 10;
const int greenLed = 9;
const int blueLed = 8;
int xR = 38;
int xG = 38;
int xB = 38;


void setup() {
  // Initial setup
  myGLCD.InitLCD();
  myGLCD.clrScr();
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_HI);
  // Defining Pin Modes
  pinMode(VCC, OUTPUT); // VCC
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  digitalWrite(VCC, HIGH); // +5V - Pin 13 as VCC


  drawHomeScreen();  // Draws the Home Screen
  currentPage = '0'; // Indicates that we are at Home Screen
  selectedUnit = '0'; // Indicates the selected unit for the first example, cms or inches

  //Setup for Temperature Sensor;
  mlx.begin();


  //Setup for HeartRate Sensor;
  Serial.begin(115200);
  particleSensor.begin();  // Initialize sensor
  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
}
void loop() {
  // Home Screen
  if (currentPage == '0') {
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x = myTouch.getX(); // X coordinate where the screen has been pressed
      y = myTouch.getY(); // Y coordinates where the screen has been pressed
      // If we press the Temperature Button
      if ((x >= 0) && (x <= 155) && (y >= 90) && (y <= 130)) {
        drawFrame(0, 90, 155, 130); // Custom Function -Highlighs the buttons when it's pressed
        currentPage = '1'; // Indicates that we are the first example
        myGLCD.clrScr(); // Clears the screen
        drawTemperatureSensor(); // It is called only once, because in the next iteration of the loop, this above if statement will be false so this funtion won't be called. This function will draw the graphics of the first example.
      }
      // If we press the Oxygen Saturation Button
      if ((x >= 165) && (x <= 319) && (y >= 90) && (y <= 130)) {
        drawFrame(165, 90, 319, 130);
        //Setup for Spo2
        pinMode(pulseLED, OUTPUT);
        pinMode(readLED, OUTPUT);
        byte ledBrightness = 60; //Options: 0=Off to 255=50mA
        byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
        byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
        byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
        int pulseWidth = 411; //Options: 69, 118, 215, 411
        int adcRange = 4096; //Options: 2048, 4096, 8192, 16384
        particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
        currentPage = '2';
        myGLCD.clrScr();
        drawSats();
      }
      // If we press the Heart Rate Button
      if ((x >= 35) && (x <= 285) && (y >= 140) && (y <= 180)) {
        drawFrame(35, 140, 285, 180);
        currentPage = '3';
        myGLCD.clrScr();
        myGLCD.setColor(114, 198, 206);
        myGLCD.fillRect(0, 0, 319, 239);
        drawHeartRate();
      }
      //If we press the Blood Pressure Button
      if ((x >= 35) && (x <= 285) && (y >= 190) && (y <= 230)) {
        drawFrame(35, 190, 285, 230);
        currentPage = '4';
        myGLCD.clrScr();
        myGLCD.setColor(114, 198, 206);
        myGLCD.fillRect(0, 0, 319, 239);
        drawBloodPressure();
      }
    }
  }
  // Temperature
  if (currentPage == '1') {
    getTemperature(); // Gets distance from the sensor and this function is repeatedly called while we are at the first example in order to print the lasest results from the distance sensor
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x = myTouch.getX();
      y = myTouch.getY();

      // If we press the Centimeters Button
      if ((x >= 10) && (x <= 135) && (y >= 90) && (y <= 163)) {
        drawFrame(10, 135, 90, 163);
        selectedUnit = '0';
      }
      // If we press the Inches Button
      if ((x >= 10) && (x <= 90) && (y >= 173) && (y <= 201)) {
        drawFrame(10, 173, 90, 201);
        selectedUnit = '1';
      }
      // If we press the Back Button
      if ((x >= 10) && (x <= 60) && (y >= 10) && (y <= 36)) {
        drawFrame(10, 10, 60, 36);
        currentPage = '0'; // Indicates we are at home screen
        myGLCD.clrScr();
        drawHomeScreen(); // Draws the home screen
      }
    }
  }
  // Sats
  if (currentPage == '2') {
    setSats();
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x = myTouch.getX();
      y = myTouch.getY();

      //Back button
      if ((x >= 10) && (x <= 60) && (y >= 10) && (y <= 36)) {
        drawFrame(10, 10, 60, 36);
        currentPage = '0';
        myGLCD.clrScr();
        drawHomeScreen();
      }
    }
  }
  // Heart Rate
  if (currentPage == '3') {
    setHeartRate();
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x = myTouch.getX();
      y = myTouch.getY();

      //Back button
      if ((x >= 10) && (x <= 60) && (y >= 10) && (y <= 36)) {
        drawFrame(10, 10, 60, 36);
        currentPage = '0';
        myGLCD.clrScr();
        drawHomeScreen();
      }
    }
  }
  // Blood Pressure
  if (currentPage == '4') {
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x = myTouch.getX();
      y = myTouch.getY();

      //Back button
      if ((x >= 10) && (x <= 60) && (y >= 10) && (y <= 36)) {
        drawFrame(10, 10, 60, 36);
        currentPage = '0';
        myGLCD.clrScr();
        drawHomeScreen();
      }
    }
  }
}
// ====== Custom Funtions ======
// drawHomeScreen - Custom Function
void drawHomeScreen() {
  // Title
  if (t >= 300 || g >= 300) {
    t = 0;
    g = 0;
  }
  myGLCD.fillScr(44, 47, 51);
  myGLCD.setBackColor(44, 47, 51); // Sets the background color of the area where the text will be printed to black
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(franklingothic_normal); // Sets font to big
  myGLCD.print("E-Health Monitoring", CENTER, 10); // Prints the string on the screen
  myGLCD.setColor(255, 0, 0); // Sets color to red
  myGLCD.drawLine(0, 32, 319, 32); // Draws the red line
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.setFont(SmallFont); // Sets the font to small
  myGLCD.print("by: Hugo Martins", CENTER, 55 ); // Prints the string

  // Button - Temperature
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (0, 90, 155, 130); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (0, 90, 155, 130); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(franklingothic_normal);// Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("   Temp", LEFT + 1, 102); // Prints the string

  // Button - Blood Oxygen
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (165, 90, 319, 130); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (165, 90, 319, 130); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(franklingothic_normal); // Sets the font to big
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("  Sats", LEFT + 180, 102); // Prints the string

  // Button - Heart Rate
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 140, 285, 180); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255); // Sets color to white
  myGLCD.drawRoundRect (35, 140, 285, 180); // Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(franklingothic_normal); // Sets the font to new font
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Heart Rate", CENTER, 152); // Prints the string

  // Button - Blood Pressure
  myGLCD.setColor(16, 167, 103); // Sets green color
  myGLCD.fillRoundRect (35, 190, 285, 230); // Draws filled rounded rectangle
  myGLCD.setColor(255, 255, 255);// Sets color to white
  myGLCD.drawRoundRect (35, 190, 285, 230);// Draws rounded rectangle without a fill, so the overall appearance of the button looks like it has a frame
  myGLCD.setFont(franklingothic_normal); // Sets the font to new font
  myGLCD.setBackColor(16, 167, 103); // Sets the background color of the area where the text will be printed to green, same as the button
  myGLCD.print("Blood Pressure", CENTER, 202); // Prints the string

}
// Highlights the button when pressed
void drawFrame(int x1, int y1, int x2, int y2) {
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
  while (myTouch.dataAvailable())
    myTouch.read();
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
}
//========================================================================================================
void drawTemperatureSensor() {
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(franklingothic_normal);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Back to Main Menu", 70, 18);
  myGLCD.setFont(franklingothic_normal);
  myGLCD.print("Temperature", CENTER, 50);
  myGLCD.print("", CENTER, 76); //!!!!!!!!!!!!!!!!!!!Change after for sensor color!!!!!!!!!!!!!!!!!!!
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawLine(0, 100, 319, 100);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Select Unit", 10, 114);
  myGLCD.setFont(franklingothic_normal);
  myGLCD.print("Temperature:", 120, 120);
  myGLCD.setColor(223, 77, 55);
  myGLCD.fillRoundRect (10, 135, 90, 163);
  myGLCD.setColor(225, 255, 255);
  myGLCD.drawRoundRect (10, 135, 90, 163);
  myGLCD.setBackColor(223, 77, 55);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("`C", 33, 140);
  myGLCD.setColor(223, 77, 55);
  myGLCD.fillRoundRect (10, 173, 90, 201);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 173, 90, 201);
  myGLCD.setBackColor(223, 77, 55);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("`F", 33, 180);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);

}
//====================================================
//===== getTemperature - Custom Function
void getTemperature() {
  c = mlx.readObjectTempC();
  f = mlx.readObjectTempF();
  // Prints the temperature in centigrade
  if (selectedUnit == '0' && c <= 400 && t < 300) {
    myGLCD.setFont(SevenSegNumFont);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.printNumI(c, 130, 145, 3, '0');
    myGLCD.setFont(franklingothic_normal);
    myGLCD.print("`C  ", 235, 178);
    t++;

    Serial.println(c);

  }
  // Prints the temperature in fahrenheit
  if (selectedUnit == '1' && f <= 160 && g < 300) {
    myGLCD.setFont(SevenSegNumFont);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.printNumI(f, 130, 145, 3, '0');
    myGLCD.setFont(franklingothic_normal);
    myGLCD.print("`F", 235, 178);
    g++;
    Serial.println(f);
  }
}
//========================================================================================================
void drawSats() {
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(franklingothic_normal);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Back to Main Menu", 70, 18);
  myGLCD.setFont(franklingothic_normal);
  myGLCD.print("Blood Saturation", CENTER, 60);
  myGLCD.print("", CENTER, 76); //!!!!!!!!!!!!!!!!!!!Change after for sensor color!!!!!!!!!!!!!!!!!!!
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawLine(0, 100, 319, 100);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(SmallFont);
}
//============= setLedColor()
void setSats() {
  bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps

  //read the first 100 samples, and determine the signal range
  for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample
  }

  //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
  while (1)
  {
    //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
    for (byte i = 25; i < 100; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    //take 25 sets of samples before calculating the heart rate.
    for (byte i = 75; i < 100; i++)
    {
      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check(); //Check the sensor for new data

      digitalWrite(readLED, !digitalRead(readLED)); //Blink onboard LED with every data read

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample

      //send samples and calculation result to terminal program through UART
      myGLCD.setFont(SevenSegNumFont);
      myGLCD.setColor(0, 255, 0);
      myGLCD.setBackColor(0, 0, 0);
      myGLCD.printNumI(spo2, 130, 145, 3, '0');
      myGLCD.setFont(franklingothic_normal);
      myGLCD.print("%", 235, 178);
      Serial.println(spo2);
    }
    //After gathering 25 new samples recalculate HR and SP02
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  }

}

//========================================================================================================
void drawHeartRate() {
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(franklingothic_normal);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Back to Main Menu", 70, 18);
  myGLCD.setFont(franklingothic_normal);
  myGLCD.print("Heart Rate", CENTER, 60);
  myGLCD.print("", CENTER, 76); //!!!!!!!!!!!!!!!!!!!Change after for sensor color!!!!!!!!!!!!!!!!!!!
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawLine(0, 100, 319, 100);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(SmallFont);
}
//============= setHeartRate()
void setHeartRate() {
  long irValue = particleSensor.getIR();
  if (checkForBeat(irValue) == true) {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();
    beatsPerMinute = 60 / (delta / 1000.0);
    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable
      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
      myGLCD.setFont(SevenSegNumFont);
      myGLCD.setColor(0, 255, 0);
      myGLCD.setBackColor(0, 0, 0);
      myGLCD.printNumI(beatAvg, 130, 145, 3, '0');
      myGLCD.setFont(franklingothic_normal);
      myGLCD.print("BPM  ", 235, 178);
      Serial.println(beatAvg);
    }
  }
}

//====================================================
void drawBloodPressure() {
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(franklingothic_normal);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Back to Main Menu", 70, 18);
  myGLCD.setFont(franklingothic_normal);
  myGLCD.print("Blood Pressure", CENTER, 50);
}
