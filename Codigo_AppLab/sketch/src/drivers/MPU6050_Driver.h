#ifndef MPU6050_DRIVER_H
#define MPU6050_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

#define MPU6050_ADDRESS     (0x68) // Dirección I2C

// Registros clave
#define MPU6050_REG_PWR_MGMT_1 (0x6B)     // PWR_MGMT_1
#define MPU6050_REG_ACCEL_XOUT_H (0x3B)   // ACCEL_XOUT_H
#define MPU6050_REG_GYRO_XOUT_H  (0x43)   // GYRO_XOUT_H
#define MPU6050_REG_GYRO_CONFIG  (0x1B)   // GYRO_CONFIG
#define MPU6050_REG_ACCEL_CONFIG (0x1C)   // ACCEL_CONFIG

// Rangos (para configurar la sensibilidad)
enum mpu6050_accel_range_t {
  ACCEL_RANGE_2G = 0x00,  // 2G
  ACCEL_RANGE_4G = 0x08,  // 4G
  ACCEL_RANGE_8G = 0x10,  // 8G
  ACCEL_RANGE_16G = 0x18  // 16G
};

enum mpu6050_gyro_range_t {
  GYRO_RANGE_250DPS = 0x00,  // 250 grados por segundo
  GYRO_RANGE_500DPS = 0x08,  // 500 grados por segundo
  GYRO_RANGE_1000DPS = 0x10, // 1000 grados por segundo
  GYRO_RANGE_2000DPS = 0x18  // 2000 grados por segundo
};

class MPU6050_Driver {
public:
  MPU6050_Driver();
  bool begin(uint8_t i2c_addr = MPU6050_ADDRESS);

  void setAccelRange(mpu6050_accel_range_t range); // Configura el rango del acelerometro
  void setGyroRange(mpu6050_gyro_range_t range);   // Configura el rango del giroscopio

  // Lee los 6 ejes crudos
  void readRawData(int16_t &ax, int16_t &ay, int16_t &az, int16_t &gx, int16_t &gy, int16_t &gz); // Lee los 6 ejes crudos

  // Lee los datos procesados en unidades físicas (G y Grados/s)
  void getProcessedData(float &ax_g, float &ay_g, float &az_g, float &gx_dps, float &gy_dps, float &gz_dps); // Lee los datos procesados en unidades físicas (G y Grados/s)

private:
  uint8_t _i2c_address;
  float _accelScale; // LSB por G
  float _gyroScale;  // LSB por Grado/seg

  void writeByte(uint8_t reg, uint8_t value); // Escribe un byte en el registro
  void readBytes(uint8_t reg, uint8_t count, uint8_t *buffer); // Lee bytes desde el registro
};

#endif