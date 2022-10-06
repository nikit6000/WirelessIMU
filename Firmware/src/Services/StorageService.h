#ifndef _STORAGE_SERVICE_H_
#define _STORAGE_SERVICE_H_

#include "FreeRTOS.h"
#include "FS.h"
#include "SD_MMC.h"
#include "FreeRTOS/semphr.h"

class StorageService {
public:
    enum State {
        reading,
        writing,
        idle,
        uninitialized
    };

    struct __attribute__ ((packed)) Measurment
    {
        float ax;
        float ay;
        float az;
        float gx;
        float gy;
        float gz;
    };
    

public:
    static StorageService& shared() {
        static StorageService service;
        return service;
    }

    bool initializeStorage();

    bool configureForReading();
    bool configureForWriting();
    void closeFile();

    size_t writeMeasurment(const Measurment& measurment);
    size_t readMeasurment(Measurment& measurment);

    File getMeasurmentFile();
private:
    StorageService();
private:
    SemaphoreHandle_t mutex;
    State storageState;
    uint64_t cardCapacity;
    File storageFile;
};

#endif
