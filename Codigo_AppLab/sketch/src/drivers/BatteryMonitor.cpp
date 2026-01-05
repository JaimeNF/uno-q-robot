#include "batteryMonitor.h"

// Tabla de voltaje
const float BatteryMonitor::VOLTAGE_TABLE[BatteryMonitor::TABLE_SIZE] = {
    7.81, 7.79, 7.75, 7.72, 7.69, 7.65, 7.61, 7.56, 7.51,
    7.46, 7.41, 7.36, 7.31, 7.26, 7.21, 7.18, 7.13, 7.09,
    7.06, 7.03, 7.00
};

// Constructor
BatteryMonitor::BatteryMonitor(int pinACS, float sensibilidad, float offset, float capacidadTotalmAh) {
    _pinACS = pinACS;
    _sensibilidad = sensibilidad;
    _offset = offset;
    _capacidadTotal = capacidadTotalmAh;

    _voltsSmoothed = 0;
    _currentAmps = 0;
    _mAhAcumulados = 0;
    _firstRun = true;
    _lastTime = 0;
}

void BatteryMonitor::begin() {
    // Iniciar INA219
    _ina219.begin();
    // CORRECCIÓN: LECTURA ESTABLE AL INICIO
    float sumVoltaje = 0;
    int validSamples = 0;

    for(int i=0; i<20; i++) {
        float v = _ina219.getBusVoltage_V();
        // Solo se suman si tiene sentido (mayor que 1V) para evitar ceros falsos
        if (v > 1.0) {
            sumVoltaje += v;
            validSamples++;
        }
        delay(10);
    }

    float startVoltage = 0;
    if (validSamples > 0) {
        startVoltage = sumVoltaje / validSamples;
    } else {
        // Si todo falla (sensor desconectado), se leen una vez sin filtro
        startVoltage = _ina219.getBusVoltage_V();
    }
    // Se estiman el porcentaje inicial
    float startPercent = estimateInitialPercentage(startVoltage);

    // Se calculan los mAh ya gastados
    if (startVoltage < 1.0) {
        _mAhAcumulados = 0; // Asumimos llena si falla el sensor
    } else {
        _mAhAcumulados = _capacidadTotal * (1.0 - startPercent);
    }

    // Se inicializan variables
    _voltsSmoothed = startVoltage;
    _lastTime = millis();
}

void BatteryMonitor::update() {
    // VOLTAJE
    float voltajeRaw = _ina219.getBusVoltage_V();

    // CORRIENTE
    float lecturaVoltajeSensor = (analogRead(_pinACS) / 1023.0) * -5.0;
    float rawCurrent = (lecturaVoltajeSensor + _offset) / _sensibilidad;

    // Banda muerta: Si es muy bajo, es ruido -> 0
    if (abs(rawCurrent) < 0.08) {
        _currentAmps = 0;
    } else {
        _currentAmps = rawCurrent;
    }

    // COULOMB COUNTING
    unsigned long now = millis();
    if (!_firstRun && now > _lastTime) {
        float dt_horas = (now - _lastTime) / 3600000.0;

        // Se suman consumo (mAh)
        float consumoInstantaneo = abs(_currentAmps) * 1000.0 * dt_horas;
        _mAhAcumulados += consumoInstantaneo;

        // Clamp: No dejar que supere la capacidad total (para evitar % negativos)
        if (_mAhAcumulados > _capacidadTotal) {
            _mAhAcumulados = _capacidadTotal;
        }
    }
    _lastTime = now;

    // FILTRO VOLTAJE
    if (_firstRun) {
        _voltsSmoothed = voltajeRaw;
        _firstRun = false;
    } else {
        // Suavizado suave para eliminar ruido
        _voltsSmoothed = (_voltsSmoothed * 0.9) + (voltajeRaw * 0.1);
    }
}

int BatteryMonitor::getPercentage() {
    // Se calcula el porcentaje restante
    float restante = _capacidadTotal - _mAhAcumulados;
    int pct = (int)((restante / _capacidadTotal) * 100.0);

    // Se limita el porcentaje entre 0 y 100
    if (pct < 0) return 0;
    if (pct > 100) return 100;
    return pct;
}

float BatteryMonitor::estimateInitialPercentage(float v) {
    // Se calcula el porcentaje inicial
    // Si el voltaje es mayor al máximo de la tabla -> 100%
    if (v >= VOLTAGE_TABLE[0]) return 1.0;

    // Si el voltaje es menor al mínimo -> 0%
    if (v <= VOLTAGE_TABLE[TABLE_SIZE - 1]) return 0.0;

    // Se hace la búsqueda en la tabla
    for (int i = 0; i < TABLE_SIZE - 1; i++) {
        float v_high = VOLTAGE_TABLE[i];
        float v_low  = VOLTAGE_TABLE[i + 1];

        if (v <= v_high && v > v_low) {
            float pct_high = (100 - (i * PERCENTAGE_STEP)) / 100.0;
            float pct_low  = (pct_high - (PERCENTAGE_STEP / 100.0));

            // Interpolación
            float fraction = (v - v_low) / (v_high - v_low);
            return pct_low + (fraction * (PERCENTAGE_STEP / 100.0));
        }
    }
    return 0.0; // Fallback
}

float BatteryMonitor::getSmoothedVoltage() { return _voltsSmoothed; }
float BatteryMonitor::getCurrent() { return _currentAmps; }
float BatteryMonitor::getConsumedmAh() { return _mAhAcumulados; }
void BatteryMonitor::resetCapacity() { _mAhAcumulados = 0; _lastTime = millis(); }