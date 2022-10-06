#ifndef _IMU_SERVICE_H_
#define _IMU_SERVICE_H_

#include "FreeRTOS.h"
#include <FreeRTOS/semphr.h>
#include <SparkFunMPU9250-DMP.h>
#include "Services/StorageService.h"

class IMUService
{
public:
    enum State {
        idle,
        sampling,
        uninitialized
    };

public:
    static IMUService& shared() {
        static IMUService service;
        return service;
    }

    inline uint16_t getSampleRate() { return imu.getSampleRate(); }
    inline State getState() { return state; }

    bool configure();
    bool writeDataToStorageIfNeeded();
    bool setSampleRate(int freq);
    bool beginSamplig();
    bool endSampling();
private:
    IMUService();

    bool writeMeasurmentToStorage();
private:
    State state;
    SemaphoreHandle_t mutex;
    MPU9250_DMP imu;
};

#endif
