#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

class BatteryMonitor {
  public:
    BatteryMonitor(int pinACS, float sensibilidad, float offset, float capacidadTotalmAh);

    void begin(); // Inicializa el monitor
    void update(); // Actualiza las lecturas
    int getPercentage(); // Devuelve el porcentaje restante
    float getSmoothedVoltage(); // Devuelve el voltaje suavizado
    float getCurrent(); // Devuelve la corriente actual
    float getConsumedmAh(); // Devuelve la cantidad de mAh consumidos
    void resetCapacity(); // Restablece la capacidad total

  private:
    // MÉTODOS PRIVADOS
    // Función auxiliar para estimar la carga inicial por voltaje
    float estimateInitialPercentage(float voltage);

    // Hardware
    Adafruit_INA219 _ina219;
    int _pinACS;
    float _sensibilidad;
    float _offset;
    float _capacidadTotal;

    // Estado
    float _voltsSmoothed;
    float _currentAmps;
    float _mAhAcumulados;
    unsigned long _lastTime;
    bool _firstRun;

    // TABLA DE CARACTERIZACIÓN (Para el arranque)
    static const int TABLE_SIZE = 21;
    static const float VOLTAGE_TABLE[TABLE_SIZE];
    static const int PERCENTAGE_STEP = 5;
};

#endif