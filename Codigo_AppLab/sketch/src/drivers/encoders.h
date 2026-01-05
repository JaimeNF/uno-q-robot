#ifndef ENCODERS_H
#define ENCODERS_H

#include <Arduino.h>

class SimpleEncoder {
  public:
    // Variables miembro
    volatile long count;
    float mPerPulse;
    unsigned long lastTime;
    long lastCount;

    // Array estático para gestionar las interrupciones (ISRs)
    // Aquí solo decimos que existe, no le damos valor.
    static SimpleEncoder* instances[2];

    // Constructor
    SimpleEncoder();

    // Métodos de configuración y uso
    void begin(int pin, float wheelDia, int pulses);
    float getSpeed();

    // Rutinas de servicio de interrupción (Static para poder usarlas con attachInterrupt)
    static void isr0();
    static void isr1();
};

#endif