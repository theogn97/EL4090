#include "WiFi.h"
#include "IOXhop_FirebaseESP32.h"

#define FIREBASE_HOST "tugas-akhir-2b4c9.firebaseio.com"
#define FIREBASE_AUTH "1uB7l0i9kTWWIWXKIfJooU46by9TRwJKJmEkHC5X"
#define WIFI_SSID "Edys Fam" //Isi dengan WiFi SSID
#define WIFI_PASSWORD "15172764" //Isi dengan WiFi Password

void setup() {
  Serial.begin(115200); //Baudrate 115200
  delay(1000);
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
  //Mulai Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

void loop() {
  //Terima data dari Firebase
  int ec = Firebase.getFloat("/devices/ITB-001/ec");
  int flow = Firebase.getFloat("/devices/ITB-001/flow");
  int intensity = Firebase.getFloat("/devices/ITB-001/intensity");
  int ph = Firebase.getFloat("/devices/ITB-001/ph");
  String plant = Firebase.getString("/devices/ITB-001/plant");
  //Print data pada Serial Monitor
  Serial.print("Electric Conductivity: ");
  Serial.println(ec);
  Serial.print("Water Flow: ");
  Serial.println(flow);
  Serial.print("Light Intensity: ");
  Serial.println(intensity);
  Serial.print("pH: ");
  Serial.println(ph);
  Serial.print("Plant: ");
  Serial.println(plant);
  delay(500);
  //Kirim data pada Firebase
  Firebase.setFloat("/devices/ITB-003/ec", 2.8);
  Firebase.setFloat("/devices/ITB-003/flow", 20.5);
  Firebase.setFloat("/devices/ITB-003/intensity", 157.5);
  Firebase.setFloat("/devices/ITB-003/ph", 5.5); 
  Firebase.setString("/devices/ITB-003/plant", "tomato");
  Serial.println("Database entry success!");
  delay(2000);
}
