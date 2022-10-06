#include "BLEService.h"
#include <Arduino.h>
#include <esp_log.h>
#include <rom/crc.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" 

static const char* BLE_TAG = "BLEDev";

BLEDeviceService::BLEDeviceService() {
    mutex = xSemaphoreCreateMutex();
    pServer = nullptr;
    pTxCharacteristic = nullptr;
    delegate = nullptr;
    connecteDeviceId = 0;
    setState(State::uninitialized);
}

bool BLEDeviceService::configure() {
    BLEDevice::init(BLE_SERVICE_DEVICE_NAME);
    
    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(this);

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    pTxCharacteristic = pService->createCharacteristic(
                                            CHARACTERISTIC_UUID_TX,
                                            BLECharacteristic::PROPERTY_NOTIFY
                                        );
                      
    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                                            CHARACTERISTIC_UUID_RX,
                                            BLECharacteristic::PROPERTY_WRITE
                                        );

    pRxCharacteristic->setCallbacks(this);

    // Start the service
    pService->start();

    // Start advertising
    pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
    pServer->getAdvertising()->start();
    ESP_LOGI(BLE_TAG, "BLE Server configured");
    setState(State::idle);
    return true;
}

void BLEDeviceService::setState(State newState) {
    const char* stateString;
    switch (newState)
    {
    case State::idle:
        stateString = "idle";
        break;
    case State::connected:
        stateString = "connected";
        break;
    case State::uninitialized:
        stateString = "uninitialized";
        break;
    default:
        stateString = "unknown";
        break;
    }
    //ESP_LOGI(BLE_TAG, "BLE current state: %s", stateString);
    Serial.print("BLE current state: ");
    Serial.println(stateString);
    state = newState;
}

void BLEDeviceService::writeResponseWithRandomNum(AnswerType type, RequestType requestType) {
    uint16_t randomNumber = esp_random() & 0xFFFF;
    writeResponse(type, requestType, (uint8_t*)&randomNumber, sizeof(uint16_t));
}

void BLEDeviceService::writeResponse(AnswerType type, RequestType requestType, uint8_t* data, size_t dataLen) {
    size_t headerSize = sizeof(AnswerHeader);
    size_t packetSize = headerSize + dataLen + 4;
    uint8_t* packet = new uint8_t[packetSize];
    uint8_t* offsettedPacket = packet;

    if (!packet) {
        return;
    }

    AnswerHeader header;
    header.type = type;
    header.requestType = requestType;
    header.sequeceNum = sequenceNum;
    header.size = dataLen;

    memcpy(offsettedPacket, &header, headerSize);

    offsettedPacket += headerSize;

    if (data && dataLen > 0) {
        memcpy(offsettedPacket, data, dataLen);
    }

    offsettedPacket += dataLen;

    uint32_t crc = crc32_le(0, packet, packetSize - 4);

    memcpy(offsettedPacket, &crc, sizeof(uint32_t));

    sequenceNum += 1;
    
    pTxCharacteristic->setValue(packet, packetSize);
    pTxCharacteristic->notify();
    delete [] packet;
}

void BLEDeviceService::setDelegate(BLEDeviceServiceDelegate* observer) {
    this->delegate = observer;
}

// BLECharacteristicCallbacks
void BLEDeviceService::onRead(BLECharacteristic* pCharacteristic) {

}

void BLEDeviceService::onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    uint8_t* data = (uint8_t*)value.data();
    size_t dataLen = value.length();
    size_t headerLen = sizeof(RequestHeader);
    uint32_t crc;

    if (dataLen < headerLen + 4) {
        writeResponseWithRandomNum(AnswerType::error, RequestType::requestError);
        return;
    }

    RequestHeader* header = (RequestHeader*)data;
    uint8_t* payload = data + headerLen;

    dataLen -= headerLen;

    if (dataLen < header->size) {
        writeResponseWithRandomNum(AnswerType::error, header->type);
        return;
    }

    dataLen -= header->size;
    uint8_t* crcData = payload + header->size;

    if (dataLen < 4) {
        writeResponseWithRandomNum(AnswerType::error, header->type);
        return;
    }

    uint32_t* packetCRC = (uint32_t*)crcData;

    crc = crc32_le(0, data, value.length() - 4);

    if (*packetCRC != crc) {
        Serial.printf("Wrong CRC %X != %X\n", *packetCRC, crc);
        writeResponseWithRandomNum(AnswerType::error, header->type);
        return;
    }

    if (delegate == nullptr) {
        writeResponseWithRandomNum(AnswerType::error, header->type);
        return;
    }

    switch (header->type)
    {
    case RequestType::beginSampling:
        {
            delegate->bluetoothDeviceServiceStartSampling(this);
            break;
        }
    case RequestType::endSampling:
        {
            delegate->bluetoothDeviceServiceStopSampling(this);
            break;
        }
    case RequestType::beginCalibration:
        {
            delegate->bluetoothDeviceServiceBeginCalibration(this);
            break;
        }
    case RequestType::requetFileTransfer: 
        {
            delegate->bluetoothDeviceServiceRequestDataTransfer(this);
            break;
        }
    case RequestType::setSamplingFreq: 
        {
            uint16_t* freq = (uint16_t*)payload;
            delegate->bluetoothDeviceServiceSetSamplingRate(this, *freq);
            break;
        }
    default:
        {
            writeResponseWithRandomNum(AnswerType::error, header->type);
            break;
        }
    }
}

void BLEDeviceService::onNotify(BLECharacteristic* pCharacteristic) {

}

// BLEServerCallbacks
void BLEDeviceService::onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t* param)  {
    //ESP_LOGI(BLE_TAG, "BLE device connected");
    Serial.print("BLE device connected: ");
    Serial.println(param->connect.conn_id);

    if (state != State::idle) {
        pServer->disconnect(param->connect.conn_id);
        return;
    }
    sequenceNum = 0;
    connecteDeviceId = param->connect.conn_id;
    setState(State::connected);
}

void BLEDeviceService::onDisconnect(BLEServer* pServer) {
    Serial.print("BLE device disconnected ");
    if (pServer->getConnId() != connecteDeviceId) {
        Serial.println(pServer->getConnId());
        return;
    }
    Serial.println(connecteDeviceId);
    connecteDeviceId = 0;
    setState(State::idle);
    pServer->startAdvertising();
}