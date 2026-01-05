#ifndef SONAR_H
#define SONAR_H

#include <Arduino.h>

// --- ULTRASONIDOS ---
void sonar_init(int trigPin, int echoPin);
int sonar_get_distance();
#endif