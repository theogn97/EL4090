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
#define TdsSensorPin 32
#define VREF 3.4
#define SCOUNT  30
#define phPin 34
#define LED_PIN     2
#define NUM_LEDS    4
CRGB leds[NUM_LEDS];

int solenoidPin1 = 16; //A
int solenoidPin2 = 17; //B
int solenoidPin3 = 5;  //Aquades
int solenoidPin4 = 18; //HCl
int solenoidPin5 = 19; //KOH

String ID = "ITB-001";

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
float finalEC, finalPH, finalLux;
int finalFlow;

//Interrupt Function untuk Flow Sensor
void flow ()
{
   flow_frequency++;
}

void setup ()
{
  //Memulai program
  int BAUDRATE = 115200;
  Serial.begin(BAUDRATE); //Baudrate 115200
  Wire.begin();
  pinMode(TdsSensorPin,INPUT);
  pinMode(phPin,INPUT);
  pinMode(solenoidPin1, OUTPUT);
  pinMode(solenoidPin2, OUTPUT);
  pinMode(solenoidPin3, OUTPUT);
  pinMode(solenoidPin4, OUTPUT);
  pinMode(solenoidPin5, OUTPUT);

  Serial.println("Hydrotech");
  Serial.println("---------");
  Serial.println("Device starting...");
  
  //Koneksi ESP32 ke WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  //Sudah terkoneksi dengan WiFi
  Serial.println();
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);

  //Membaca data dari Firebase
  Serial.println("Retrieving data from Cloud Database");
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  String pathID = "/Devices/" + ID + "/plant";
  String plant = Firebase.getString(pathID);
  Serial.print("Device ID: ");
  Serial.println(ID);
  Serial.print("Plant assigned to " + ID + ": ");
  Serial.println(plant);
  String pathPlant = "/Optimal Parameters/" + plant;
  
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
  delay(1000);
  Serial.println("");

  //Memulai LED
  delay(1000);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(10);
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
  
  //Memulai Sensor Cahaya
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("Light sensor initialized!"));
  }
  else {
    Serial.println(F("Error initializing BH1750"));
  }
  Serial.println("");
  delay(1000);
  pinMode(flowsensor, INPUT);
  digitalWrite(flowsensor, HIGH); // Optional Internal Pull-Up
  attachInterrupt(15, flow, RISING); // Setup Interrupt
  sei(); // Enable interrupts
  Serial.println("Flow sensor initialized!");
  delay(1000);
  digitalWrite(solenoidPin1, HIGH);
  Serial.println("A valve started");
  delay(1000);
  digitalWrite(solenoidPin2, HIGH);
  Serial.println("B valve started");
  delay(1000);
  digitalWrite(solenoidPin3, HIGH);
  Serial.println("Aquades valve started");
  delay(1000);
  digitalWrite(solenoidPin4, HIGH);
  Serial.println("HCl valve started");
  delay(1000);
  digitalWrite(solenoidPin5, HIGH);
  Serial.println("KOH valve started");
  delay(1000);
  Serial.println("All valves initialized!");
  delay(1000);
}

void loop ()
{
  delay(5000);
  bacaParameter();
  controlValve();
}

//Membaca semua parameter
void bacaParameter ()
{
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
  while (((tempEC[i]-tempEC[i-1])<-0.1)||((tempEC[i]-tempEC[i-1])>0.1)||((tempPH[i]-tempPH[i-1])<-0.1)||((tempPH[i]-tempPH[i-1])>0.1)||((tempFlow[i]-tempFlow[i-1])<-1)||((tempFlow[i]-tempFlow[i-1])>1)||((tempLux[i]-tempLux[i-1])<-20)||((tempLux[i]-tempLux[i-1])>20))
  {
    i++;
    bacaEC();
    tempEC[i] = ecValue;
    bacaLux();
    tempLux[i] = lux;
    bacaFlow();
    tempFlow[i] = l_min;
    bacaPH();
    tempPH[i] = phValue;
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
  kirimData();
  Serial.println("Data sent to Firebase");
  Serial.println("");
}

//Buka dan tutup valve
void controlValve ()
{
  if (finalEC < minec)
  {
    Serial.println("EC value too low!");
    Serial.println("Optimizing EC value...");
    tambahEC();
  }
  else
  {
    Serial.println("EC value not too low");
  }
  if (finalEC > maxec)
  {
    Serial.println("EC value too high!");
    Serial.println("Optimizing EC value...");
    kurangiEC();
  }
  else
  {
    Serial.println("EC value not too high");
  }
  if (finalPH < minph)
  {
    Serial.println("PH value too low!");
    Serial.println("Optimizing PH value...");
    tambahPH();
  }
  else
  {
    Serial.println("PH value not too low");
  }
  if (finalPH > maxph)
  {
    Serial.println("PH value too high!");
    Serial.println("Optimizing PH value...");
    kurangiPH();
  }
  else
  {
    Serial.println("EC value not too high");
  }
  Serial.println("All parameters optimized!");
}

//Membaca konduktivitas elektrik
void bacaEC ()
{
   for(analogBufferIndex=0;analogBufferIndex<SCOUNT;analogBufferIndex++)
   {
     analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
   }
   
   averageVoltage = getMedianNum(analogBuffer,SCOUNT) * (float)VREF / 4096.0;
   float compensationCoefficient=1.0+0.02*(temperature-25.0);
   float compensationVolatge=averageVoltage/compensationCoefficient;
   tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5;
   ecValue=tdsValue/500;
   Serial.print("EC: ");
   Serial.print(ecValue,3);
   Serial.println(" mS");
}

//Membaca intensitas cahaya
void bacaLux ()
{
  lux = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
}

//Membaca debit air
void bacaFlow ()
{
   l_min = (flow_frequency / 7.5);
   flow_frequency = 0;
   Serial.print("Flow: ");
   Serial.print(l_min, DEC);
   Serial.println(" L/min");
}

//Membaca derajat keasaman
void bacaPH ()
{
  for(int i=0;i<10;i++) 
  { 
    buf[i]=analogRead(phPin);
    delay(10);
  }
  for(int i=0;i<9;i++)
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }
  avgValue=0;
  for(int i=2;i<8;i++)
  avgValue+=buf[i];
  float pHVol=(float)avgValue*3.3/4095/6;
  phValue = -5.70 * pHVol + 21.34;
  //phValue = -4.2424 * pHVol + 19.6;
  Serial.print("pH: ");
  Serial.println(phValue,2);
}

//Mengirim data menuju Firebase
void kirimData ()
{
  String pathEC = "/Devices/" + ID + "/ec";
  Firebase.setFloat(pathEC, finalEC);
  String pathLux = "/Devices/" + ID + "/intensity";
  Firebase.setFloat(pathLux, finalLux);
  String pathFlow = "/Devices/" + ID + "/flow";
  Firebase.setFloat(pathFlow, finalFlow);
  String pathPH = "/Devices/" + ID + "/ph";
  Firebase.setFloat(pathPH, finalPH);
} 

//Menambah EC
void tambahEC ()
{
  while(finalEC < maxec)
  {
    digitalWrite(solenoidPin1, LOW);
    Serial.println("A valve opened!");
    delay(1000);
    digitalWrite(solenoidPin1, HIGH);
    Serial.println("A valve closed!");
    delay(1000);
    digitalWrite(solenoidPin2, LOW);
    Serial.println("B valve opened!");
    delay(1000);
    digitalWrite(solenoidPin2, HIGH);
    Serial.println("B valve closed!");
    delay(1000);
    finalEC = finalEC + 0.2;
    Serial.print("EC: ");
    Serial.println(finalEC);
  } 
}

//Mengurangi EC
void kurangiEC ()
{
  digitalWrite(solenoidPin3, LOW);
  Serial.println("Aquades valve opened!");
  while(finalEC >= maxec)
  {
    finalEC = finalEC - 0.1;
    Serial.print("EC: ");
    Serial.println(finalEC);
    delay(1000);
  }
  digitalWrite(solenoidPin3, HIGH);
  Serial.println("Aquades valve closed!");
}

//Menambah PH
void tambahPH ()
{
  digitalWrite(solenoidPin5, LOW);
  Serial.println("KOH valve opened!");
  while(finalPH < maxph)
  {
    finalPH = finalPH + 0.1;
    Serial.print("PH: ");
    Serial.println(finalPH);
    delay(1000);
  }
  digitalWrite(solenoidPin5, HIGH);
  Serial.println("KOH valve closed!");
}

//Mengurangi PH
void kurangiPH ()
{
  digitalWrite(solenoidPin4, LOW);
  Serial.println("HCl valve opened!");
  while(finalPH >= maxph)
  {
    finalPH = finalPH - 0.1;
    Serial.print("PH: ");
    Serial.println(finalPH);
    delay(1000);
  }
  digitalWrite(solenoidPin4, HIGH);
  Serial.println("HCl valve closed!");
}

//Mencari nilai median (untuk pembacaan konduktivitas elektrik)
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
