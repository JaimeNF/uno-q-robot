#ifndef REMOTE_H
#define REMOTE_H

#include <Arduino.h>

//  CONFIGURACIÓN
#define RECV_PIN  12

//  CÓDIGOS DEL MANDO
#define IR_FORWARD 0xFF629D
#define IR_BACK    0xFFA857
#define IR_LEFT    0xFF22DD
#define IR_RIGHT   0xFFC23D
#define IR_STOP    0xFF02FD
#define IR_MODE    0xFF6897
#define IR_REPEAT  0xFFFFFFFF

// --- FUNCIONES ---
void remote_init();
uint32_t remote_read();

#endif