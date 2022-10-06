#ifndef _SYNC_SERVICE_H_
#define _SYNC_SERVICE_H_

#include "BLEService.h"
#include "FileTransferService.h"

#define SYNC_SERVICE_NOTIFY_INTERVAL  (1500)

class SyncService: public BLEDeviceServiceDelegate, public FileTransferServiceDelegate
{
public:
    enum State {
        idle,
        sampling,
        transfering,
        uninitialized
    };
public:
    static SyncService& shared() {
        static SyncService service;
        return service;
    }

    bool configure();
    void loop();
private:
    void notifyAboutState();

    void bluetoothDeviceServiceSetSamplingRate(BLEDeviceService* service, int frequency) override;
    void bluetoothDeviceServiceStartSampling(BLEDeviceService* service) override;
    void bluetoothDeviceServiceStopSampling(BLEDeviceService* service) override;
    void bluetoothDeviceServiceBeginCalibration(BLEDeviceService* service) override;
    void bluetoothDeviceServiceRequestDataTransfer(BLEDeviceService* service) override;

    void fileTransferingDidStartServer(FileTransferService* service) override;
    void fileTransferingDidStopServer(FileTransferService* service) override;

    SyncService();

private:
    unsigned long long lastNotificationMillis;
    State state;
};

#endif
