//External Libraries
#include <Arduino.h>
#include <esp_task_wdt.h>      //Watchdog

#define TX_GPIO 5   //GPIO pin connected to CAN Transciever TX Pin
#define RX_GPIO 4   //GPIO pin connected to CAN Transciever RX Pin

// Project libs
#include "mon-OTA.h"          //OTA Functions
#include "mon-CAN.h"          //CAN Functions   -- MUST be after TX_GPIO and RX_GPIO are defined


uint8_t button = BOOT_PIN;

uint32_t loopTime, loopAverage;
uint16_t loopCount = 0;

// put function declarations here:

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  //Connect to WiFi and start OTA if available
  WiFiSetup();
  
  //Create task for Listening to CAN
  xTaskCreatePinnedToCore(
      canRecieve, /* Function to implement the task */
      "CAN", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &CAN_h,  /* Task handle. */
      0); /* Core where the task should run */
  


  Serial.println("Starting WDT on Main task");
  esp_task_wdt_add(NULL); // Add Watchdog to the current thread 

  Serial.println("setup() finished");
}

void loop() {
  // put your main code here, to run repeatedly:

  // Checking button for factory reset and reporting
  if (digitalRead(button) == LOW) {  // Push button pressed
    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(button) == LOW) {
      delay(50);
      if ((millis() - startTime) > 3000) {
        // If key pressed for more than 3secs, factory reset Zigbee and reboot
        Serial.println("Resetting factory and rebooting in 1s.");
        delay(1000);
        //Reset code here
      }
    }
  }

  // Check how long to run loop, less is better
  loopAverage = loopAverage + (millis() - loopTime);
  loopCount += 1;
  //Serial.println(loopAverage);
  if (loopCount == 500) {
    Serial.printf("Average time to run loop %lums at %lums\n", loopAverage / loopCount, millis());
    loopCount = 0;
    loopAverage = 0;
  }
  
  loopTime = millis();

  //Look for OTA
  ArduinoOTA.handle();

  delay(5);

  esp_task_wdt_reset(); // Feed watchdog timer
}

// put function definitions here:
