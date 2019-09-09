#include <FastLED.h>

#define LED_PIN     2
#define NUM_LEDS    4
#define buzzPin     32
CRGB leds[NUM_LEDS];

RTC_DATA_ATTR int bootCount = 0;

void setup(){
  Serial.begin(115200);
  pinMode(buzzPin, OUTPUT);

  digitalWrite(buzzPin,HIGH);

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
  
  delay(1000); 

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  //Configure GPIO33 as ext0 wake up source for HIGH logic level
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33,1);

  delay(3000);
  
  //Go to sleep now
  Serial.println("ESP32 sleeping now...");
  //digitalWrite(buzzPin,LOW);
  esp_deep_sleep_start();
}

void loop(){}

//Function that prints the reason by which ESP32 has been awaken from sleep
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("Wakeup caused by timer"); break;
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
}
