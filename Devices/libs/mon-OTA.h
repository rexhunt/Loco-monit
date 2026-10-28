#ifndef mon_OTA
#define mon_OTA

#include <Arduino.h>
#include <WiFi.h>              //
#include <ArduinoOTA.h>        //OTA Updates

// Set up global WIFI details
#define WIFI_SSID "Loco"
#define WIFI_PASS "1234Update"
IPAddress local_IP(10, 0, 20, 2);
IPAddress gateway(10, 0, 20, 1);
IPAddress subnet(255, 0, 0, 0);

// Declare your function here (this is a prototype)
void WiFiSetup(void);       //Configure WiFi
void OTASetup(void);        //Configure OTA


//Define Functions here
void WiFiSetup(void){
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  Serial.println(WiFi.localIP());
  
  //Set up OTA
  OTASetup();
}


void OTASetup(void){
  // OTA Configiration and Enable OTA
  Serial.println("\nEnabling OTA Feature");
  ArduinoOTA.setPassword("lonelybinary");
  ArduinoOTA.onStart([]() {
    Serial.println("Start updating sketch...");
    esp_task_wdt_delete(NULL);  //Pause watchdog timer during update
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd update");
    esp_task_wdt_add(NULL);   //resume watchdog timer after update finished
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    esp_task_wdt_add(NULL);   //resume watchdog timer after update finished
  });
  ArduinoOTA.begin();
}

#endif