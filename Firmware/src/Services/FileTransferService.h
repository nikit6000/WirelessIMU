#ifndef _FILE_TRANSFER_SERVICE_H_
#define _FILE_TRANSFER_SERVICE_H_

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>

class FileTransferServiceDelegate;

class FileTransferService
{
public:
    struct WiFiData
    {
        std::string ssid;
        std::string password;
    };

    enum State {
        idle,
        transfering,
        uninitialized
    };

public:
    static FileTransferService& shared() {
        static FileTransferService service;
        return service;
    }

    bool configure();
    bool prepareForFileTransfering();
    void setDelegate(FileTransferServiceDelegate* observer);
    void stopTransfering();
    void loop();
    WiFiData getWifiData();
private:
    bool handleFileRequest();
private:
    FileTransferService();
    WebServer* server;
    State state;
    FileTransferServiceDelegate* delegate;
};

class FileTransferServiceDelegate {
public:
    virtual void fileTransferingDidStartServer(FileTransferService* service) = 0;
    virtual void fileTransferingDidStopServer(FileTransferService* service) = 0;
};

#endif
