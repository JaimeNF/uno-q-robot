#include "MPU6050_Driver.h"

MPU6050_Driver::MPU6050_Driver() {
  // Constructor
}

bool MPU6050_Driver::begin(uint8_t i2c_addr) {
  _i2c_address = i2c_addr;
  Wire.begin();

  // Despertar el MPU-6050 (sale del modo de suspensión)
  // Escribir 0x00 en el registro PWR_MGMT_1 (0x6B)
  writeByte(MPU6050_REG_PWR_MGMT_1, 0x00);

  // Configurar rangos por defecto
  setAccelRange(ACCEL_RANGE_2G);
  setGyroRange(GYRO_RANGE_500DPS);

  return true;
}

void MPU6050_Driver::setAccelRange(mpu6050_accel_range_t range) {
  writeByte(MPU6050_REG_ACCEL_CONFIG, range);

  // Actualizar el factor de escala según la hoja de datos
  switch (range) {
    case ACCEL_RANGE_2G: _accelScale = 16384.0; break;
    case ACCEL_RANGE_4G: _accelScale = 8192.0; break;
    case ACCEL_RANGE_8G: _accelScale = 4096.0; break;
    case ACCEL_RANGE_16G: _accelScale = 2048.0; break;
  }
}

void MPU6050_Driver::setGyroRange(mpu6050_gyro_range_t range) {
  writeByte(MPU6050_REG_GYRO_CONFIG, range);

  // Actualizar el factor de escala según la hoja de datos
  switch (range) {
    case GYRO_RANGE_250DPS: _gyroScale = 131.0; break;
    case GYRO_RANGE_500DPS: _gyroScale = 65.5; break;
    case GYRO_RANGE_1000DPS: _gyroScale = 32.8; break;
    case GYRO_RANGE_2000DPS: _gyroScale = 16.4; break;
  }
}

void MPU6050_Driver::readRawData(int16_t &ax, int16_t &ay, int16_t &az, int16_t &gx, int16_t &gy, int16_t &gz) {
  // 14 bytes en total (Acc-X/Y/Z, Temp, Gyro-X/Y/Z)
  uint8_t buffer[14];

  // Leer los 14 bytes empezando desde ACCEL_XOUT_H (0x3B)
  readBytes(MPU6050_REG_ACCEL_XOUT_H, 14, buffer);

  // Combinar bytes (MSB y LSB) para formar enteros de 16 bits
  ax = (int16_t)((buffer[0] << 8) | buffer[1]);
  ay = (int16_t)((buffer[2] << 8) | buffer[3]);
  az = (int16_t)((buffer[4] << 8) | buffer[5]);
  // Saltar 2 bytes de temperatura (buffer[6] y buffer[7])
  gx = (int16_t)((buffer[8] << 8) | buffer[9]);
  gy = (int16_t)((buffer[10] << 8) | buffer[11]);
  gz = (int16_t)((buffer[12] << 8) | buffer[13]);
}

void MPU6050_Driver::getProcessedData(float &ax_g, float &ay_g, float &az_g, float &gx_dps, float &gy_dps, float &gz_dps) {
  int16_t ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw;
  readRawData(ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw);

  // Convertir a unidades físicas (G y Grados/s)
  ax_g = (float)ax_raw / _accelScale;
  ay_g = (float)ay_raw / _accelScale;
  az_g = (float)az_raw / _accelScale;

  gx_dps = (float)gx_raw / _gyroScale;
  gy_dps = (float)gy_raw / _gyroScale;
  gz_dps = (float)gz_raw / _gyroScale;
}

// --- Funciones I2C Privadas ---

void MPU6050_Driver::writeByte(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(_i2c_address);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void MPU6050_Driver::readBytes(uint8_t reg, uint8_t count, uint8_t *buffer) {
  Wire.beginTransmission(_i2c_address);
  Wire.write(reg);
  Wire.endTransmission(false); // false = no liberar el bus (restart)

  Wire.requestFrom(_i2c_address, count);

  for (int i = 0; i < count; i++) {
    buffer[i] = Wire.read();
  }
}