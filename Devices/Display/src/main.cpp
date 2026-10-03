#include <Arduino.h>
#include <Adafruit_NeoPixel.h> //Control multicolour LED on board
#include <driver/twai.h>       //CAN Stuff
#include <esp_task_wdt.h>      //Watchdog

#define PIN_NEOPIXEL 8  // Change this to your board's NeoPixel pin (e.g., 48 on some ESP32-S3 boards)
#define NUM_PIXELS 1     // Number of LEDs

#define TX_GPIO 5   //GPIO pin connected to CAN Transciever TX Pin
#define RX_GPIO 4   //GPIO pin connected to CAN Transciever RX Pin

Adafruit_NeoPixel pixels(NUM_PIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);


uint8_t button = BOOT_PIN;

/*Create separate task for controlling Onboard LED
  This allows delay without affecting other parts of loop()
*/
//Create handles for tasks
TaskHandle_t LED_h;

//Create handles for Queues
QueueHandle_t LRed = NULL;
QueueHandle_t LGreen = NULL;
QueueHandle_t LBlue = NULL;

//Create array for recieved data
uint8_t states[4095][8];

void LED_Code(void * parameter) {
  //Declare variables
  uint8_t red, green, blue = 0;
  Serial.println("LED Loop initialized");

  //Make code loop
  for(;;){
    // Get updated colours if changed
    if (xQueuePeek(LRed, &red, 0)){ //Set timeout to 0 to return immediately if queue is empty. Otherwise will wait forever
      //Serial.print(red);
      //Serial.println(" LED Red Value");
    }
    if (xQueuePeek(LGreen, &green, 0)){
      //Serial.print(green);
      //Serial.println(" LED Green Value");
    }
    if (xQueuePeek(LBlue, &blue, 0)){
      //Serial.print(blue);
      //Serial.println(" LED Blue Value");
    }

    // Control the LED here
    pixels.setPixelColor(0, pixels.Color(red, green, blue)); // Set pixel to colour
    pixels.show();   // Send the updated color to the hardware
    vTaskDelay(20 / portTICK_PERIOD_MS);
    pixels.setPixelColor(0, pixels.Color(0, 0, 0)); // Turn pixel off
    pixels.show();   // Send the updated color
    vTaskDelay(20 / portTICK_PERIOD_MS); 
  }
}

//Set LED
void setLED(bool value) {
  uint8_t intensity = 0;
  if (value){ intensity = 100;}
  xQueueOverwrite(LRed, &intensity);
}

// put function declarations here:

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Initialize the digital pin as an output
  pixels.begin();
  
  //Create queues for inter task comms
  LRed = xQueueCreate(1, sizeof(uint8_t)); 
  LGreen = xQueueCreate(1, sizeof(uint8_t)); // Colour queues are only 1 long, more like global variable than FIFO queue, use peek to leave value intact
  LBlue = xQueueCreate(1, sizeof(uint8_t)); 
  if (LRed == NULL && LGreen == NULL && LBlue == NULL) {
    Serial.println("Failed to create queue!");
    while (1);
  }

  //Create task for controlling the LED
  xTaskCreatePinnedToCore(
      LED_Code, /* Function to implement the task */
      "LED", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &LED_h,  /* Task handle. */
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
    .timeout_ms = 5000,     // Timeout set to 5000 ms (5 seconds)
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1, // Monitor idle tasks on all cores
    .trigger_panic = true   // Panic/Reset the ESP32 if triggered
  };
  esp_task_wdt_init(&wdt_config); // Enable panic, ESP32 auto-restarts on WDT timeout
  esp_task_wdt_add(NULL); // Add the current thread 
  
  //Set initial LED Colour indicating setup is finished
  uint8_t colour = 10;
  xQueueOverwrite(LBlue, &colour);

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

  twai_message_t response;
  uint8_t length;
  uint16_t ident;
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
  esp_task_wdt_reset(); // Feed watchdog timer
}

// put function definitions here: