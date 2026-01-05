#ifndef MOTORS_H
#define MOTORS_H

#include <Arduino.h>

// --- PINES ASIGNADOS PARA ARDUINO UNO Q ---
#define ENA 5   // PWM A
#define ENB 6   // PWM B
#define IN1 7
#define IN2 8
#define IN3 9
#define IN4 11

// --- CONFIGURACIÓN ---
#define MIN_POWER 80

void motors_init();
void motors_drive(int speedLeft, int speedRight);
void motors_stop();
void motors_forward(int speed);
void motors_back(int speed);
void motors_left(int speed);
void motors_right(int speed);

#endif