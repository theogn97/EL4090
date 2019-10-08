#include "WiFi.h"
#include "IOXhop_FirebaseESP32.h"
#include "Arduino.h"
#include <Wire.h>
#include <BH1750.h>
#include <FastLED.h>

//Definisi Global Variables
#define FIREBASE_HOST "tugas-akhir-2b4c9.firebaseio.com"
#define FIREBASE_AUTH "1uB7l0i9kTWWIWXKIfJooU46by9TRwJKJmEkHC5X"
#define WIFI_SSID "TG97Hotspot" //Isi dengan WiFi SSID
#define WIFI_PASSWORD "TA181902" //Isi dengan WiFi Password

#define pinOutpH 34
#define pinOutEC 35
#define VREF 3.3
#define SCOUNT  30

#define LED_PIN     2
#define NUM_LEDS    4
CRGB leds[NUM_LEDS];

#define waterPumpA 16
#define waterPumpB 17
#define waterPumpAqua 5
#define waterPumpH 18
#define waterPumpK 19
#define waterPumpI 32
#define waterPumpO 33

#define fillTime 25000
#define emptyTime 50000

#define motorPumpDelay 1000
#define motorPumpOffDelay 60000

String ID = "ITB-001";
String pathID = "/Devices/" + ID + "/plant";
String plant;
String pathPlant;
String pathStatus;

float minec, maxec, minph, maxph, minflow, maxflow, minint, maxint;

int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0,tdsValue = 0,ecValue = 0,temperature = 27;
float lux;
float phValue;

volatile int flow_frequency;
int l_min;
unsigned char flowsensor = 15;

int sensorValue = 0; 
unsigned long int avgValue; 
float b;
int buf[10],temp;

BH1750 lightMeter(0x23);

float tempEC[10], tempPH[10], tempLux[10];
int tempFlow[10];
float finalEC, finalPH, finalLux, pH;
int finalFlow;

int brightLED = 10;

void flow ()
{
   flow_frequency++;
}

void setup ()
{
  initialization();
  
  connectingToWiFi();

  //readingDataFromDatabase();

  startingLED();
  startingLigthSensor();
  startingWaterFlowSensor();
  startingWaterPumps();
}

void initialization(){
  int BAUDRATE = 115200;
  Serial.begin(BAUDRATE); //Baudrate 115200
  Wire.begin();
  pinMode(pinOutpH, INPUT);
  pinMode(pinOutEC, INPUT);
  
  pinMode(waterPumpA, OUTPUT);
  pinMode(waterPumpB, OUTPUT);
  pinMode(waterPumpAqua, OUTPUT);
  pinMode(waterPumpH, OUTPUT);
  pinMode(waterPumpK, OUTPUT);
  pinMode(waterPumpI, OUTPUT);
  pinMode(waterPumpO, OUTPUT);

  Serial.println("Hydrotech");
  Serial.println("---------");
  Serial.println("Device starting...");
}

void connectingToWiFi(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
}

void readingDataFromDatabase(){
  Serial.println("Retrieving data from Cloud Database");
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  changeStatus("Reading optimal parameters from database");
 
  plant = Firebase.getString(pathID);
  Serial.print("Device ID: ");
  Serial.println(ID);
  Serial.print("Plant assigned to " + ID + ": ");
  Serial.println(plant);
  pathPlant = "/Optimal Parameters/" + plant;
  
  String pathMinec = pathPlant + "/minec";
  minec = Firebase.getFloat(pathMinec);
  String pathMaxec = pathPlant + "/maxec";
  maxec = Firebase.getFloat(pathMaxec);
  String pathMinph = pathPlant + "/minph";
  minph = Firebase.getFloat(pathMinph);
  String pathMaxph = pathPlant + "/maxph";
  maxph = Firebase.getFloat(pathMaxph);
  String pathMinflow = pathPlant + "/minflow";
  minflow = Firebase.getFloat(pathMinflow);
  String pathMaxflow = pathPlant + "/maxflow";
  maxflow = Firebase.getFloat(pathMaxflow);
  String pathMinint = pathPlant + "/minint";
  minint = Firebase.getFloat(pathMinint);
  String pathMaxint = pathPlant + "/maxint";
  maxint = Firebase.getFloat(pathMaxint);
  
  Serial.println("Optimal Parameters for "+plant+":");
  Serial.print("Min. EC: ");
  Serial.println(minec);
  Serial.print("Max. EC: ");
  Serial.println(maxec);
  Serial.print("Min. pH: ");
  Serial.println(minph);
  Serial.print("Max. pH: ");
  Serial.println(maxph);
  Serial.print("Min. Flow: ");
  Serial.println(minflow);
  Serial.print("Max. Flow: ");
  Serial.println(maxflow);
  Serial.print("Min. Intensity: ");
  Serial.println(minint);
  Serial.print("Max. Intensity: ");
  Serial.println(maxint);
  Serial.println("");
  changeStatus("Finished reading optimal parameters");
}

void startingLED(){
  changeStatus("Turning on LEDs");
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(brightLED);
  for(int i = 0; i < NUM_LEDS; i++)
  {
    if(i%2==0)
    {
      leds[i] = CRGB(255, 0, 0);
      FastLED.show();
    }
    else
    {
      leds[i] = CRGB(0, 70, 255);
      FastLED.show();
    }
  }
  Serial.println("LEDs initialized!");
  changeStatus("LEDs are turned on");
}

void startingLigthSensor(){
  changeStatus("Starting light sensor");
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("Light sensor initialized!"));
  }
  else {
    Serial.println(F("Error initializing BH1750"));
  }
  changeStatus("Light sensor is ready");
}

void startingWaterFlowSensor(){
  changeStatus("Starting water flow sensor");
  pinMode(flowsensor, INPUT);
  digitalWrite(flowsensor, HIGH); // Optional Internal Pull-Up
  attachInterrupt(15, flow, RISING); // Setup Interrupt
  sei(); // Enable interrupts
  Serial.println("Flow sensor initialized!");
  changeStatus("Water flow sensor is ready");
}

void startingWaterPumps(){
  changeStatus("Starting water pumps");
  digitalWrite(waterPumpA, LOW);
  Serial.println("A valve started");
  digitalWrite(waterPumpB, LOW);
  Serial.println("B valve started");
  digitalWrite(waterPumpAqua, LOW);
  Serial.println("Aquades valve started");
  digitalWrite(waterPumpH, LOW);
  Serial.println("HCl valve started");
  digitalWrite(waterPumpK, LOW);
  Serial.println("KOH valve started");
  digitalWrite(waterPumpI, LOW);
  Serial.println("Input valve started");
  digitalWrite(waterPumpO, LOW);
  Serial.println("Output valve started");
  Serial.println("All valves initialized!");
  changeStatus("Water pumps are ready");
}

void loop ()
{
  fillJars();
  readActualParameters();
  emptyJars();
  //sendDataToFirebase();
//  controlMotorPumps();
}

void fillJars ()
{
  changeStatus("Filling jars");
  Serial.println("Filling jars");
//  digitalWrite(waterPumpI,HIGH);
//  delay(fillTime);
//  digitalWrite(waterPumpI,LOW);
  digitalWrite(waterPumpA,HIGH);
  delay(fillTime);
  digitalWrite(waterPumpA,LOW);
  digitalWrite(32,HIGH);
  digitalWrite(33,HIGH);
  delay(1000);
  changeStatus("Finished filling jars");
  Serial.println("Finished filling jars");
}

void emptyJars ()
{
  changeStatus("Emptying jars");
  Serial.println("Emptying jars");
  digitalWrite(32,LOW);
  digitalWrite(33,LOW);
  delay(1000);
//  digitalWrite(waterPumpO,HIGH);
//  delay(emptyTime);
//  digitalWrite(waterPumpO,LOW);
  digitalWrite(waterPumpB,HIGH);
  delay(emptyTime);
  digitalWrite(waterPumpB,LOW);
  changeStatus("Finished emptying jars");
  Serial.println("Finished emptying jars");
}

void readActualParameters ()
{
  changeStatus("Reading actual parameters");
  tempEC[0] = 0;
  tempLux[0] = 0;
  tempFlow[0] = 0;
  tempPH[0] = 0;
  tempEC[1] = 10;
  tempLux[1] = 10;
  tempFlow[1] = 10;
  tempPH[1] = 10;
  int i = 1;
  Serial.println("Parameter scan starting...");
  Serial.println("");
  int j = 0;
  while (((tempEC[i]-tempEC[i-1])<-0.05)||((tempEC[i]-tempEC[i-1])>0.05)||((tempPH[i]-tempPH[i-1])<-0.05)||((tempPH[i]-tempPH[i-1])>0.05)||((tempFlow[i]-tempFlow[i-1])<-1)||((tempFlow[i]-tempFlow[i-1])>1)||((tempLux[i]-tempLux[i-1])<-50)||((tempLux[i]-tempLux[i-1])>50))
  {
    j++;
    Serial.print("Pembacaan ke : ");
    Serial.println(j);
    readingEC();
    tempEC[i] = ecValue;
//    readingLux();
//    tempLux[i] = lux;
    readingFlow();
    tempFlow[i] = l_min;
    readingPH();
    tempPH[i] = pH;
    i++;
    if (i > 8)
    {
      i = 1;
      tempEC[0] = tempEC[9];
      tempLux[0] = tempLux[9];
      tempFlow[0] = tempFlow[9];
      tempPH[0] = tempPH[9];
    }
    delay(1000);
  }
  finalEC = tempEC[i];
  finalLux = tempLux[i];
  finalFlow = tempFlow[i];
  finalPH = tempPH[i];
  Serial.println("");
  Serial.println("Parameter scan complete");
  changeStatus("Finished reading actual parameters");
}

void controlMotorPumps ()
{
  if (finalPH < minph)
  {
    addingPH();
  }
  else if (finalPH > maxph)
  {
    reducingPH();
  }
  else
  {
  }

  if(finalPH >= minph && finalPH <= maxph)
  {
    if (finalEC < minec)
    {
      addingEC();
    }
    else if (finalEC > maxec)
    {
      reducingEC();
    }
    else
    {
      Serial.println("EC and pH parameters optimized!");
    }
  }
  
//  if (finalLux < minint)
//  {
//    addingLux();
//  }
//  else if (finalLux > maxint)
//  {
//    reducingLux();
//  }
//  else
//  {
//  }

  
}

void readingEC ()
{
  changeStatus("Reading EC");
  for(analogBufferIndex=0;analogBufferIndex<SCOUNT;analogBufferIndex++)
  {
   analogBuffer[analogBufferIndex] = analogRead(pinOutEC);
  }
  
  averageVoltage = getMedianNum(analogBuffer,SCOUNT) * (float)VREF / 4096.0;
  float compensationCoefficient=1.0+0.02*(temperature-25.0);
  float compensationVolatge=averageVoltage/compensationCoefficient;
  tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*3.3/5.0;
  ecValue=tdsValue/500;
  Serial.print("EC Vol: ");
  Serial.println(averageVoltage,3);
  Serial.print("TDS: ");
  Serial.print(tdsValue,0);
  Serial.println(" ppm");
  Serial.print("EC: ");
  Serial.print(ecValue,3);
  Serial.println(" mS");
  
  changeStatus("Finished reading EC");
}

void readingLux ()
{
  changeStatus("Reading light intensity");
  lux = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
  changeStatus("Finished reading light intensity");
}

void readingFlow ()
{
  changeStatus("Reading water flow");
  l_min = (flow_frequency / 7.5);
  flow_frequency = 0;
  Serial.print("Flow: ");
  Serial.print(l_min, DEC);
  Serial.println(" L/min");
  changeStatus("Finished reading water flow");
}

void readingPH ()
{
  changeStatus("Reading pH");
  
  for(analogBufferIndex=0;analogBufferIndex<SCOUNT;analogBufferIndex++)
  {
   analogBuffer[analogBufferIndex] = analogRead(pinOutpH);
  }
  
//  int analogReadpH = analogRead(pinOutpH);
  int analogReadpH = getMedianNum(analogBuffer,SCOUNT);
  pH = -0.0065 * analogReadpH + 27.895;
  Serial.print("AR : ");
  Serial.println(analogReadpH);
  Serial.print("pH : ");
  Serial.println(pH);
  
  changeStatus("Finished reading pH");
}

void sendDataToFirebase()
{
  changeStatus("Sending actual data to database");
  String pathEC = "/Devices/" + ID + "/ec";
  Firebase.setFloat(pathEC, finalEC);
  String pathLux = "/Devices/" + ID + "/intensity";
  Firebase.setFloat(pathLux, finalLux);
  String pathFlow = "/Devices/" + ID + "/flow";
  Firebase.setFloat(pathFlow, finalFlow);
  String pathPH = "/Devices/" + ID + "/ph";
  Firebase.setFloat(pathPH, finalPH);
  changeStatus("Actual data are sent");
}

void changeStatus(String inputStatus){
//  pathStatus = "/Devices/" + ID + "/status";
//  Firebase.setString(pathStatus, inputStatus);
}

void addingEC ()
{
  changeStatus("Adding EC");
  digitalWrite(waterPumpA, HIGH);
  Serial.println("A valve opened!");
  delay(motorPumpDelay);
  digitalWrite(waterPumpA, LOW);
  Serial.println("A valve closed!");
  delay(motorPumpOffDelay);
  digitalWrite(waterPumpB, HIGH);
  Serial.println("B valve opened!");
  delay(motorPumpDelay);
  digitalWrite(waterPumpB, LOW);
  Serial.println("B valve closed!");
  delay(motorPumpOffDelay); 
  changeStatus("Finished adding EC");
}

void reducingEC ()
{
  changeStatus("Reducing EC");
  digitalWrite(waterPumpAqua, HIGH);
  Serial.println("Aquades valve opened!");
  delay(motorPumpDelay);
  digitalWrite(waterPumpAqua, LOW);
  Serial.println("Aquades valve closed!");
  delay(motorPumpOffDelay);
  changeStatus("Finished reducing EC");
}

void addingPH ()
{
  changeStatus("Adding pH");
  digitalWrite(waterPumpK, HIGH);
  Serial.println("KOH valve opened!");
  delay(motorPumpDelay);
  digitalWrite(waterPumpK, LOW);
  Serial.println("KOH valve closed!");
  delay(motorPumpOffDelay);
  changeStatus("Finished adding pH");
}

void reducingPH ()
{
  changeStatus("Reducing pH");
  digitalWrite(waterPumpH, HIGH);
  Serial.println("HCl valve opened!");
  delay(motorPumpDelay);
  digitalWrite(waterPumpH, LOW);
  Serial.println("HCl valve closed!");
  delay(motorPumpOffDelay);
  changeStatus("Finished reducing pH");
}

void addingLux ()
{
  brightLED = brightLED + 5;
  FastLED.setBrightness(brightLED);
}

void reducingLux ()
{
  brightLED = brightLED - 5;
  FastLED.setBrightness(brightLED);
}

int getMedianNum(int bArray[], int iFilterLen)
{
      int bTab[iFilterLen];
      for (byte i = 0; i<iFilterLen; i++)
      bTab[i] = bArray[i];
      int i, j, bTemp;
      for (j = 0; j < iFilterLen - 1; j++)
      {
      for (i = 0; i < iFilterLen - j - 1; i++)
          {
        if (bTab[i] > bTab[i+ 1])
            {
        bTemp = bTab[i];
            bTab[i] = bTab[i+ 1];
        bTab[i+1] = bTemp;
         }
      }
      }
      if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
      else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
      return bTemp;
}
