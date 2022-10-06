#include "FileTransferService.h"
#include "StorageService.h"

static const char* WifiSSID = "TTIN-IMU";
static const char* WifiPassword = "n97rJwS9";

FileTransferService::FileTransferService() {
    server = nullptr;
    delegate = nullptr;
    state = State::uninitialized;
}

bool FileTransferService::configure() {
    server = new WebServer(80);

    if (server == nullptr) {
        return false;
    }

    server->on("/measurment", HTTP_GET, [this](){
        this->handleFileRequest();
    });
    
    state = State::idle;

    return true;
}

bool FileTransferService::prepareForFileTransfering() {
    if (state != State::idle) {
        return false;
    }

    if (!WiFi.softAP(WifiSSID, WifiPassword, 1, 0, 1)) {
        return false;
    }

    server->begin();
    state = State::transfering;
    if (delegate != nullptr) {
        delegate->fileTransferingDidStartServer(this);
    }
    return true;
}

bool FileTransferService::handleFileRequest() {
    if (server == nullptr) {
        return false;
    }
    if (!StorageService::shared().configureForReading()) {
        server->send(404, "application/json", "{\"error\": \"System is busy\"}");
        return false;
    }

    File file = StorageService::shared().getMeasurmentFile();

    if (!file) {
        server->send(404, "application/json", "{\"error\": \"File not found\"}");
        return false;
    }

    server->streamFile(file, "application/octet-stream");

    StorageService::shared().closeFile();
    stopTransfering();
    return true;
}

void FileTransferService::stopTransfering() {
    if (state == State::idle) {
        return;
    }
    server->stop();
    state = State::idle;
    if (delegate != nullptr) {
        delegate->fileTransferingDidStopServer(this);
    }
}

void FileTransferService::loop() {
    if (server == nullptr) {
        return;
    }
    server->handleClient();
}

void FileTransferService::setDelegate(FileTransferServiceDelegate* observer) {
    this->delegate = observer;
}

FileTransferService::WiFiData FileTransferService::getWifiData() {
    WiFiData data = {
        .ssid = std::string(WifiSSID),
        .password = std::string(WifiPassword)
    };
    return data;
}