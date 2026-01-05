#include "motors.h"

void motors_init() {
    // Configuramos todos los pines como salida
    pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

    motors_stop();
}

// Se limita la velocidad a un rango seguro
int clip_speed(int speed) {
    if (speed == 0) return 0;
    if (abs(speed) < MIN_POWER) return (speed > 0) ? MIN_POWER : -MIN_POWER;
    if (speed > 255) return 255;
    if (speed < -255) return -255;
    return speed;
}

// Se envían las velocidades a los motores
void motors_drive(int speedLeft, int speedRight) {
    speedLeft = clip_speed(speedLeft);
    speedRight = clip_speed(speedRight);

    // Motor A
    if (speedLeft > 0) {
        digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
        analogWrite(ENA, speedLeft);
    } else if (speedLeft < 0) {
        digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
        analogWrite(ENA, -speedLeft);
    } else {
        digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
        analogWrite(ENA, 0);
    }

    // Motor B
    if (speedRight > 0) {
        digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
        analogWrite(ENB, speedRight);
    } else if (speedRight < 0) {
        digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
        analogWrite(ENB, -speedRight);
    } else {
        digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
        analogWrite(ENB, 0);
    }
}

//  Funciones de control
void motors_stop() { motors_drive(0, 0); }
void motors_forward(int s) { motors_drive(s, s); }
void motors_back(int s) { motors_drive(-s, -s); }
void motors_left(int s) { motors_drive(-s, s); }
void motors_right(int s) { motors_drive(s, -s); }