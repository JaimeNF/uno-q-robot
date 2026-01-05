#include "encoders.h"

// DEFINICIÓN DE VARIABLES ESTÁTICAS
SimpleEncoder* SimpleEncoder::instances[2] = {nullptr, nullptr};

// CONSTRUCTOR
SimpleEncoder::SimpleEncoder() {
    count = 0;
    mPerPulse = 0;
    lastTime = 0;
    lastCount = 0;
}

// MÉTODOS

void SimpleEncoder::begin(int pin, float wheelDia, int pulses) {
    // Se calcula la longitud de la circunferencia
    float circ = wheelDia * PI / 1000.0;
    mPerPulse = circ / pulses;
    // Se configura el pin como entrada con pull-up
    pinMode(pin, INPUT_PULLUP);
    // Se asigna la instancia actual (this) al array estático según el pin
    if (pin == 2) {
        instances[0] = this;
        attachInterrupt(digitalPinToInterrupt(2), isr0, RISING);
    } else if (pin == 10) {
        instances[1] = this;
        attachInterrupt(digitalPinToInterrupt(10), isr1, RISING);
    }
    // Se inicializa el último tiempo
    lastTime = millis();
}

float SimpleEncoder::getSpeed() {
    unsigned long now = millis();

    // Filtro simple para evitar división por cero o tiempos muy cortos
    if (now - lastTime < 2) return 0;

    long c = count; // Copia local por seguridad (volatile)
    long dCount = c - lastCount;
    float dt = (now - lastTime) / 1000.0;

    if (dt <= 0.0) return 0;

    float speed = (dCount * mPerPulse) / dt;

    lastCount = c;
    lastTime = now;
    return speed*3.6;
}

// RUTINAS DE INTERRUPCIÓN (ISRs)

void SimpleEncoder::isr0() {
    if (instances[0] != nullptr) {
        instances[0]->count++;
    }
}

void SimpleEncoder::isr1() {
    if (instances[1] != nullptr) {
        instances[1]->count++;
    }
}