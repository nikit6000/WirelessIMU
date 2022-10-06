#include "StorageService.h"

static const char* TAG = "StorageService";

#define STORAGE_SERVICE_MEASURMENT_FILE_PATH        "/measurment.dat"

StorageService::StorageService() {
    mutex = xSemaphoreCreateMutex();
    storageState = State::uninitialized;
    cardCapacity = 0;
}

bool StorageService::initializeStorage() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (storageState != State::uninitialized)
        return true;

    if(!SD_MMC.begin()){
        ESP_LOGE(TAG, "Can`t initialize physycal storage");
        return false;
    }

    uint8_t cardType = SD_MMC.cardType();

    if(cardType == CARD_NONE){
        ESP_LOGE(TAG, "Card not attached");
        return false;
    }

    cardCapacity = SD_MMC.cardSize() / (1024 * 1024);
    ESP_LOGI(TAG, "Detected card type %d, (%lluMB)", cardType, cardCapacity);

    storageState = State::idle;
    xSemaphoreGive(mutex);
    return true;
}

bool StorageService::configureForReading() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    
    if (storageState != State::idle)
        return false;
    if (storageFile)
        storageFile.close();
    
    storageFile = SD_MMC.open(STORAGE_SERVICE_MEASURMENT_FILE_PATH, FILE_READ);

    xSemaphoreGive(mutex);
    return true;
}

bool StorageService::configureForWriting() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    
    if (storageState != State::idle) {
        xSemaphoreGive(mutex);
        return false;
    }

    if (storageFile)
        storageFile.close();
    
    storageFile = SD_MMC.open(STORAGE_SERVICE_MEASURMENT_FILE_PATH, FILE_WRITE);
    
    if (!storageFile) {
        xSemaphoreGive(mutex);
        return false;
    } 

    storageState = State::writing;

    xSemaphoreGive(mutex);
    return true;
}

void StorageService::closeFile() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (!(storageState != idle)) {
        xSemaphoreGive(mutex);
        return;
    }
    if (!storageFile) {
        xSemaphoreGive(mutex);
        return;
    }
    storageFile.close();
    storageState = State::idle;
    xSemaphoreGive(mutex);
}

size_t StorageService::writeMeasurment(const StorageService::Measurment& measurment) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (storageState != State::writing) {
        xSemaphoreGive(mutex);
        ESP_LOGI(TAG, "Warning! writeMeasurment called at wrong state!");
        return 0;
    }
    size_t writen = storageFile.write((uint8_t*)&measurment, sizeof(Measurment));
    xSemaphoreGive(mutex);
    ESP_LOGI(TAG, "Writed: %d bytes", writen);
    return writen;
}

size_t StorageService::readMeasurment(StorageService::Measurment& measurment) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (storageState != State::reading) {
        xSemaphoreGive(mutex);
        return 0;
    }
    Measurment meas;
    size_t readed = storageFile.read((uint8_t*)&meas, sizeof(Measurment));
    if (readed != sizeof(Measurment)) {
        xSemaphoreGive(mutex);
        return 0;
    }
    measurment = meas;
    xSemaphoreGive(mutex);
    return readed;
}

File StorageService::getMeasurmentFile() {
    return storageFile;
}