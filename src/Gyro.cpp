#include "../Configuration.hpp"
#include "Utility.hpp"
#include "Gyro.hpp"

#if USE_GYRO_LEVEL == 1

PUSH_NO_WARNINGS

#define GYRO_USE_MPU9250 1

#if USE_GYRO_WITH_SOFTWAREI2C == 1
#include "SlowSoftWire.h" // I2C communication library
SlowSoftWire i2c(GYRO_SOFTWARE_SDA_PIN, GYRO_SOFTWARE_SCL_PIN);
#else
#include <Wire.h> // I2C communication library
TwoWire i2c;
#endif
#include <MPU9250.h> 

POP_NO_WARNINGS
#if GYRO_USE_MPU9250
#if USE_GYRO_WITH_SOFTWAREI2C == 1
MPU9250_<SlowSoftWire> gyro;
#else
MPU9250_<TwoWire> gyro;
#endif
#endif
/**
 * Tilt, roll, and temperature measurementusing the MPU-6050 MEMS gyro.
 * See: https://invensense.tdk.com/products/motion-tracking/6-axis/mpu-6050/
 * Datasheet: https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf
 * Register descriptions: https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
 * */

bool Gyro::isPresent(false);

void Gyro::startup()
/* Starts up the MPU-6050 device.
   Reads the WHO_AM_I register to verify if the device is present.
   Wakes device from power-down.
   Sets accelerometers to minimum bandwidth to reduce measurement noise.
*/
{
    // Initialize interface to the MPU6050
    LOGV1(DEBUG_INFO, F("GYRO:: Starting"));
    i2c.begin();
#if GYRO_USE_MPU9250
    MPU9250Setting setting = MPU9250Setting();
    gyro.setup(0x68, setting, i2c);
#else
    // Execute 1 byte read from MPU6050_REG_WHO_AM_I
    // This is a read-only register which should have the value 0x68
    i2c.beginTransmission(MPU6050_I2C_ADDR);
    i2c.write(MPU6050_REG_WHO_AM_I);
    i2c.endTransmission(true);
    i2c.requestFrom(MPU6050_I2C_ADDR, 1, 1);
    byte id   = (i2c.read() >> 1) & 0x3F;
    isPresent = (id == 0x34);
    if (!isPresent)
    {
        LOGV1(DEBUG_INFO, F("[GYRO]:: Not found!"));
        return;
    }

    // Execute 1 byte write to MPU6050_REG_PWR_MGMT_1
    i2c.beginTransmission(MPU6050_I2C_ADDR);
    i2c.write(MPU6050_REG_PWR_MGMT_1);
    i2c.write(0);  // Disable sleep, 8 MHz clock
    i2c.endTransmission(true);

    // Execute 1 byte write to MPU6050_REG_PWR_MGMT_1
    i2c.beginTransmission(MPU6050_I2C_ADDR);
    i2c.write(MPU6050_REG_CONFIG);
    i2c.write(6);  // 5Hz bandwidth (lowest) for smoothing
    i2c.endTransmission(true);
#endif

    LOGV1(DEBUG_INFO, F("[GYRO]:: Started"));
}

void Gyro::shutdown()
/* Shuts down the MPU-6050 device.
   Currently does nothing.
*/
{
    LOGV1(DEBUG_INFO, F("[GYRO]: Shutdown"));
    // Nothing to do
}

angle_t Gyro::getCurrentAngles()
/* Returns roll & tilt angles from MPU-6050 device in angle_t object in degrees.
   If MPU-6050 is not found then returns {0,0}.
*/
{
    struct angle_t result;
#if GYRO_USE_MPU9250
    gyro.update();
    result.pitchAngle = gyro.getPitch();
    result.rollAngle = gyro.getRoll();
#else
    // Read the accelerometer data
    const int windowSize = 16;
    result.pitchAngle = 0;
    result.rollAngle  = 0;
    if (!isPresent)
        return result;  // Gyro is not available

    for (int i = 0; i < windowSize; i++)
    {
        // Execute 6 byte read from MPU6050_REG_WHO_AM_I
        i2c.beginTransmission(MPU6050_I2C_ADDR);
        i2c.write(MPU6050_REG_ACCEL_XOUT_H);
        i2c.endTransmission(false);
        i2c.requestFrom(MPU6050_I2C_ADDR, 6, 1);      // Read 6 registers total, each axis value is stored in 2 registers
        int16_t AcX = i2c.read() << 8 | i2c.read();  // X-axis value
        int16_t AcY = i2c.read() << 8 | i2c.read();  // Y-axis value
        int16_t AcZ = i2c.read() << 8 | i2c.read();  // Z-axis value

        // Calculating the Pitch angle (rotation around Y-axis)
        result.pitchAngle += ((atanf(-1 * AcX / sqrtf(powf(AcY, 2) + powf(AcZ, 2))) * 180.0f / static_cast<float>(PI)) * 2.0f) / 2.0f;
        // Calculating the Roll angle (rotation around X-axis)
        result.rollAngle += ((atanf(-1 * AcY / sqrtf(powf(AcX, 2) + powf(AcZ, 2))) * 180.0f / static_cast<float>(PI)) * 2.0f) / 2.0f;

        delay(10);  // Decorrelate measurements
    }

    result.pitchAngle /= windowSize;
    result.rollAngle /= windowSize;
#endif
    #if GYRO_AXIS_SWAP == 1
    float temp        = result.pitchAngle;
    result.pitchAngle = result.rollAngle;
    result.rollAngle  = temp;
    #endif
    return result;
}

float Gyro::getCurrentTemperature()
/* Returns MPU-6050 device temperature in degree C.
   If MPU-6050 is not found then returns 99 (C).
*/
{
    float result;
    if (!isPresent)
        return 99.0f;  // Gyro is not available
#if GYRO_USE_MPU9250
    gyro.update();
    result = gyro.getTemperature();
#else
    // Execute 2 byte read from MPU6050_REG_TEMP_OUT_H
    i2c.beginTransmission(MPU6050_I2C_ADDR);
    i2c.write(MPU6050_REG_TEMP_OUT_H);
    i2c.endTransmission(false);
    i2c.requestFrom(MPU6050_I2C_ADDR, 2, 1);            // Read 2 registers total, the temperature value is stored in 2 registers
    int16_t tempValue = i2c.read() << 8 | i2c.read();  // Raw Temperature value

    // Calculating the actual temperature value
    result = static_cast<float>(tempValue) / 340.0f + 36.53f;
#endif
    return result;
}
#endif
