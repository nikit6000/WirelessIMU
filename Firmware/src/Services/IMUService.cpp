#include "IMUService.h"

IMUService::IMUService() {
    mutex = xSemaphoreCreateMutex();
    state = State::uninitialized;
    imu = MPU9250_DMP();
}

bool IMUService::configure() {
    if (imu.begin() != INV_SUCCESS)
    {
        return false;
    }
    imu.setSampleRate(100); // 4Hz to 1kHz
    imu.configureFifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    state = State::idle;
    return true;
}

bool IMUService::writeDataToStorageIfNeeded() {
    if (state != State::sampling) {
        return false;
    }

    if ( imu.fifoAvailable() >= 256)
    {
        while ( imu.fifoAvailable() > 0)
        {
            if ( imu.updateFifo() == INV_SUCCESS)
            {
                writeMeasurmentToStorage();
            }
        }
    }
    return true;
}

bool IMUService::writeMeasurmentToStorage() {
    StorageService::Measurment measurment {
        .ax = imu.calcAccel(imu.ax),
        .ay = imu.calcAccel(imu.ay),
        .az = imu.calcAccel(imu.az),
        .gx = imu.calcGyro(imu.gx),
        .gy = imu.calcGyro(imu.gy),
        .gz = imu.calcGyro(imu.gz)
    };
    return StorageService::shared().writeMeasurment(measurment) != 0;
}

bool IMUService::setSampleRate(int freq) {
    if (state != State::sampling) {
        return false;
    }
    if (freq < 4)
        freq = 4;
    else if (freq > 1000) {
        freq = 1000;
    }
    return imu.setSampleRate(freq) == INV_SUCCESS;
}

bool IMUService::beginSamplig() {
    if (state != State::idle) {
        Serial.println("Can`t start sampling: Already samling!");
        return false;
    }
    //imu.resetFifo();
    StorageService::shared().configureForWriting();
    state = State::sampling;
    Serial.println("Sampling started!");
    return true;
}

bool IMUService::endSampling() {
    if (state != State::sampling) {
        Serial.println("Can`t stop sampling: Already stopped!");
        return false;
    }
    StorageService::shared().closeFile();
    state = State::idle;
    Serial.println("Sampling stopped!");
    return true;
}