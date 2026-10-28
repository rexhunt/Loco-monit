#include <Arduino.h>
#include <driver/twai.h>       //CAN Stuff
#include <esp_task_wdt.h>      //Watchdog
#include "mon-OTA.h"  //OTA Functions

#define TX_GPIO 5   //GPIO pin connected to CAN Transciever TX Pin
#define RX_GPIO 4   //GPIO pin connected to CAN Transciever RX Pin

uint8_t button = BOOT_PIN;

uint32_t loopTime, loopAverage;
uint8_t loopCount = 0;

//Create handles for tasks
TaskHandle_t CAN_h;

//Create array for recieved data
uint8_t states[4095][8];

// put function declarations here:
void canRecieve(void * parameter);

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
  
  //Set up CAN Bus
  // Configure TWAI driver for 500 kbps (standard automotive)
  twai_general_config_t g_config =
    TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)TX_GPIO,
                                (gpio_num_t)RX_GPIO,
                                TWAI_MODE_NORMAL); //MODE_NO_ACK for testing, rather than MODE_NORMAL
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    Serial.println("TWAI driver installed");
  }
  if (twai_start() == ESP_OK) {
    Serial.println("TWAI driver started");
  }

  Serial.println("Configuring WDT...");
  esp_task_wdt_config_t wdt_config = {
    //.timeout_ms = 5000,     // Timeout set to 5000 ms (5 seconds)
    .timeout_ms = 50000,     // Timeout set to 5000 ms (5 seconds)
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1, // Monitor idle tasks on all cores
    .trigger_panic = true   // Panic/Reset the ESP32 if triggered
  };
  esp_task_wdt_init(&wdt_config); // Enable panic, ESP32 auto-restarts on WDT timeout
  esp_task_wdt_add(NULL); // Add the current thread 

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
  if (loopCount == 500) {
    Serial.printf("Average time to run loop %ums\n", loopAverage / loopCount);
    loopCount = 0;
    loopAverage = 0;
  }
  
  loopTime = millis();

  //Look for OTA
  ArduinoOTA.handle();

  delay(50);

  esp_task_wdt_reset(); // Feed watchdog timer
}

// put function definitions here:

void canRecieve(void * parameter){
  twai_message_t response;
  uint8_t length;
  uint16_t ident;
  Serial.println("CAN Loop initialized");

  //Make code loop
  for(;;){
    if (twai_receive(&response, pdMS_TO_TICKS(100)) == ESP_OK) {
        //Process packet from CAN bus
        ident = response.identifier;
        length = response.data_length_code;
        Serial.printf("Packet id: 0x%X, and Length: %u Data: ", ident, length);
        for (int i = 0; i < length; i++) {
          states[ident][i] = response.data[i];
          Serial.printf("0x%X, ", states[ident][i]);
        }
        Serial.println();
    }
  }
}

