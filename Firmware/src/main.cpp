#include <Arduino.h>
#include "FS.h"
#include "SD_MMC.h"
#include <math.h>
#include <SparkFunMPU9250-DMP.h>
#include "Services/SyncService.h"

void setupServices(void);

static const char* MAIN_TAG = "MAIN";

void setup() 
{
  esp_log_level_set("*", ESP_LOG_INFO); 
  Serial.begin(115200);
  setupServices();
}

void setupServices(void) {
  if(SyncService::shared().configure()) {
    ESP_LOGI(MAIN_TAG, "Initialization was success");
  } else {
    ESP_LOGI(MAIN_TAG, "Initialization was unsuccess");
  }
}

void loop() 
{
  SyncService::shared().loop();
}
