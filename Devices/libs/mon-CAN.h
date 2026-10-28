#ifndef mon_CAN
#define mon_CAN

#include <driver/twai.h>       //CAN Stuff
#include <esp_task_wdt.h>      //Watchdog

//Create handles for tasks
TaskHandle_t CAN_h;

//Create array for recieved data
uint8_t states[4095][8];

// Declare your function here
void canRecieve(void * parameter);

//Define Functions here - To be moved to separate file
void canRecieve(void * parameter){
  twai_message_t response;
  uint8_t length;
  uint16_t ident;
  Serial.println("Starting WDT on CAN task");
  esp_task_wdt_add(NULL); // Add Watchdog to the current thread

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

  Serial.println("CAN Loop initialized");

  //Make code loop
  for(;;){
    esp_task_wdt_reset(); // Feed watchdog timer
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

#endif