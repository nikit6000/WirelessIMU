#include "SyncService.h"
#include "StorageService.h"
#include "IMUService.h"
#include "FileTransferService.h"
#include <Arduino.h>

SyncService::SyncService() {
    lastNotificationMillis = 0;
    state = State::uninitialized;
}

bool SyncService::configure() {

    if (!BLEDeviceService::shared().configure())
        return false;

    if (!StorageService::shared().initializeStorage())
        return false;
    
    if (!IMUService::shared().configure())
        return false;

    if (!FileTransferService::shared().configure()){
        return false;
    }
    
    BLEDeviceService::shared().setDelegate(this);
    FileTransferService::shared().setDelegate(this);

    state = State::idle;

    return true;
}

void SyncService::loop() {
    IMUService::shared().writeDataToStorageIfNeeded();
    FileTransferService::shared().loop();
    notifyAboutState();
}

void SyncService::notifyAboutState() {
    if (IMUService::shared().getState() != IMUService::State::sampling) {
        return;
    }

    if (millis() - lastNotificationMillis > SYNC_SERVICE_NOTIFY_INTERVAL) {
        uint32_t currentState = 0;
        
        if (IMUService::shared().getState() == IMUService::State::sampling) {
            currentState |= 1;
        }

        BLEDeviceService::shared().writeResponse(BLEDeviceService::AnswerType::stateResponse,  BLEDeviceService::RequestType::stateRequest, (uint8_t*)&currentState, sizeof(uint32_t));
        lastNotificationMillis = millis();
    }
}

// BLEDeviceServiceDelegate
void SyncService::bluetoothDeviceServiceSetSamplingRate(BLEDeviceService* service, int frequency) {
    if (state != State::sampling && IMUService::shared().setSampleRate(frequency)) {
        uint16_t sampleRate = IMUService::shared().getSampleRate();
        uint16_t data[] = { sampleRate, (uint16_t)(esp_random() & 0xFFFF) };
        service->writeResponse(BLEDeviceService::AnswerType::ok, BLEDeviceService::RequestType::setSamplingFreq, (uint8_t*)data, sizeof(uint16_t) * 2);
    } else {
        service->writeResponseWithRandomNum(BLEDeviceService::AnswerType::error, BLEDeviceService::RequestType::beginSampling);
    }
}

void SyncService::bluetoothDeviceServiceStartSampling(BLEDeviceService* service) {
    if (state == State::idle && IMUService::shared().beginSamplig()) {
        service->writeResponseWithRandomNum(BLEDeviceService::AnswerType::ok, BLEDeviceService::RequestType::beginSampling);
        state = State::sampling;
    } else {
        service->writeResponseWithRandomNum(BLEDeviceService::AnswerType::error, BLEDeviceService::RequestType::beginSampling);
    }
}

void SyncService::bluetoothDeviceServiceStopSampling(BLEDeviceService* service) {
    if (state == State::sampling && IMUService::shared().endSampling()) {
        service->writeResponseWithRandomNum(BLEDeviceService::AnswerType::ok, BLEDeviceService::RequestType::endSampling);
        state = State::idle;
    } else {
        service->writeResponseWithRandomNum(BLEDeviceService::AnswerType::error, BLEDeviceService::RequestType::endSampling);
    }
}

void SyncService::bluetoothDeviceServiceBeginCalibration(BLEDeviceService* service) {
    
}

void SyncService::bluetoothDeviceServiceRequestDataTransfer(BLEDeviceService* service) {
    if (state != State::idle) {
        service->writeResponseWithRandomNum(BLEDeviceService::AnswerType::error, BLEDeviceService::RequestType::requetFileTransfer);
        return;
    }
    FileTransferService::shared().prepareForFileTransfering();
    state = State::transfering;
}

// FileTransferServiceDelegate
void SyncService::fileTransferingDidStartServer(FileTransferService* service) {
    FileTransferService::WiFiData data = service->getWifiData();
    BLEDeviceService::shared().writeResponse(BLEDeviceService::AnswerType::widiSSID, BLEDeviceService::RequestType::requetFileTransfer, (uint8_t*)data.ssid.data(), data.ssid.length());
    vTaskDelay(500 / portTICK_PERIOD_MS);
    BLEDeviceService::shared().writeResponse(BLEDeviceService::AnswerType::wifiPassword, BLEDeviceService::RequestType::requetFileTransfer, (uint8_t*)data.password.data(), data.password.length());
}

void SyncService::fileTransferingDidStopServer(FileTransferService* service) {
    state = State::idle;
}