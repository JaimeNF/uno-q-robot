#include "remote.h"

volatile uint32_t _ir_code = 0;
volatile bool _ir_ready = false;
volatile unsigned long _last_micros = 0;

// Esta función se ejecuta automáticamente cada vez que el receptor detecta un cambio
void _ir_isr() {
    unsigned long current_micros = micros();
    unsigned long duration = current_micros - _last_micros;
    _last_micros = current_micros;

    static uint32_t temp_code = 0;
    static int bit_count = 0;

    // Protocolo NEC:
    // Start bit: ~13500us (9000 mark + 4500 space) -> Detectamos > 10000
    // Bit 0: ~1125us
    // Bit 1: ~2250us
    // Repeat: ~11000us

    if (duration > 10000) {
        bit_count = 0;
        temp_code = 0;
    }
    else if (duration > 7000 && duration < 10000) {
    }
    else {
        temp_code <<= 1;
        if (duration > 1600) {
            temp_code |= 1;
        }

        bit_count++;
        if (bit_count == 32) {
            _ir_code = temp_code;
            _ir_ready = true;
            bit_count = 0; // Reiniciar
        }
    }
}

void remote_init() {
    pinMode(RECV_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(RECV_PIN), _ir_isr, FALLING);
}

uint32_t remote_read() {
    if (_ir_ready) {
        noInterrupts();
        uint32_t code = _ir_code;
        _ir_ready = false;
        interrupts();
        return code;
    }
    return 0;
}