#include "sonar.h"

// Variables privadas
int _trigPin;
int _echoPin;
int _servoPin = -1;

// =========================================================
// PULSE-IN MANUAL
// =========================================================
unsigned long my_pulseIn(int pin, uint8_t state, unsigned long timeout) {
    unsigned long startMicros = micros();

    // Esperar a que el pin llegue al estado deseado
    while (digitalRead(pin) != state) {
        if (micros() - startMicros > timeout) return 0;
    }

    unsigned long pulseStart = micros();

    // Esperar a que el pin salga del estado
    while (digitalRead(pin) == state) {
        if (micros() - startMicros > timeout) return 0;
    }
    return micros() - pulseStart;
}

// =========================================================
// ULTRASONIDOS
// =========================================================
void sonar_init(int trigPin, int echoPin) {
    // Inicialización
    _trigPin = trigPin;
    _echoPin = echoPin;
    pinMode(_trigPin, OUTPUT);
    pinMode(_echoPin, INPUT);
}

int sonar_get_distance() {
    // Generar Trigger
    digitalWrite(_trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(_trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trigPin, LOW);

    // Leer Echo
    long duration = my_pulseIn(_echoPin, HIGH, 30000); // 30ms timeout
    int distance = (int)(duration * 0.034 / 2); // Velocidad sonido = 340m/s

    if (distance > 400 || distance == 0) return 0;
    return distance;
}