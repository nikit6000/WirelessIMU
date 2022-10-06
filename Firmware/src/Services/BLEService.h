#ifndef _BLE_SERVICE_H_
#define _BLE_SERVICE_H_

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "FreeRTOS/semphr.h"

#define BLE_SERVICE_DEVICE_NAME "TTIN IMU"

class BLEDeviceServiceDelegate;

class BLEDeviceService: public BLEServerCallbacks, public BLECharacteristicCallbacks
{
public:
    enum State {
        idle,
        connected,
        uninitialized
    };
    
    enum RequestType: uint8_t {
        beginSampling = 1,
        endSampling = 2,
        beginCalibration = 3,
        setSamplingFreq = 4,
        requetFileTransfer = 5,
        stateRequest = 6,
        requestError = 255
    };

    enum AnswerType: uint8_t {
        ok = 0,
        widiSSID = 1,
        wifiPassword = 2,
        stateResponse = 3,
        error = 255
    };

private:
    struct __attribute__ ((packed)) RequestHeader {
        uint8_t size;
        RequestType type;
        uint16_t sequeceNum;
    };

    struct __attribute__ ((packed)) AnswerHeader {
        uint8_t size;
        AnswerType type;
        RequestType requestType;
        uint16_t sequeceNum;
    };

public:
    static BLEDeviceService& shared() {
        static BLEDeviceService service;
        return service;
    }

    void setDelegate(BLEDeviceServiceDelegate* delgate);

    inline State getState() { return state; }

    bool configure();
    void writeResponseWithRandomNum(AnswerType type, RequestType requestType);
    void writeResponse(AnswerType type, RequestType requestType, uint8_t* data, size_t dataLen);
private: 
    void setState(State newState);
    void onRead(BLECharacteristic* pCharacteristic) override;
	void onWrite(BLECharacteristic* pCharacteristic) override;
    void onNotify(BLECharacteristic* pCharacteristic) override;
    void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t* param) override;
	void onDisconnect(BLEServer* pServer) override;

private:
    BLEDeviceService();
    SemaphoreHandle_t mutex;
    State state;
    BLEServer *pServer;
    BLECharacteristic *pTxCharacteristic;
    BLEDeviceServiceDelegate* delegate;
    uint16_t connecteDeviceId;
    uint16_t sequenceNum;
};

class BLEDeviceServiceDelegate {
public:
    virtual void bluetoothDeviceServiceSetSamplingRate(BLEDeviceService* service, int frequency) = 0;
    virtual void bluetoothDeviceServiceStartSampling(BLEDeviceService* service) = 0;
    virtual void bluetoothDeviceServiceStopSampling(BLEDeviceService* service) = 0;
    virtual void bluetoothDeviceServiceBeginCalibration(BLEDeviceService* service) = 0;
    virtual void bluetoothDeviceServiceRequestDataTransfer(BLEDeviceService* service) = 0;
};

#endif
