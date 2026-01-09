#include <Arduino_RouterBridge.h>
#include <Wire.h>
#include <Servo.h>
#include <Adafruit_INA219.h>
#include <zephyr/kernel.h>

#include "src/motors.h"
#include "src/remote.h"
#include "src/BatteryMonitor.h"
#include "src/encoders.h"
#include "src/MPU6050_Driver.h"
#include "src/sonar.h"

//              CONFIGURACIÓN

const int PIN_ECHO       = A4;
const int PIN_TRIG       = A5;
const int PIN_SERVO      = 3;
const int PIN_ACS        = A0;
const int MIN_DISTANCE   = 20;
const int OK_DISTANCE    = 35;

// Tiempos
const int SCAN_TIME      = 700;
const int TURN_90_TIME   = 1000;
const int BACKUP_TIME    = 1000;

// Constantes
const float Kp_STRAIGHT    = 10.0;
const float Kp_VISION      = 5.0;
const int   BASE_SPEED     = 150;
const int   MAX_CORRECTION = 50;
const int   FRAME_CENTER_X = 160;
const float WHEEL_DIA      = 65.0;
const int   TICKS          = 2;

//      VARIABLES COMPARTIDAS Y MUTEX

K_MUTEX_DEFINE(data_mutex);
K_MUTEX_DEFINE(print_mutex); // Candado para el Monitor Serial



// Datos protegidos
volatile float shared_angleZ = 0.0;
volatile float shared_targetHeading = 0.0;
volatile int   shared_objX = 0;
volatile int   shared_objY = 0;
volatile int   shared_confidence = 0;
volatile unsigned long shared_lastVisionTime = 0;
String         shared_label = "";

// Estados
enum RobotState {
  STATE_STOPPED,
  STATE_FORWARD,
  STATE_BACKWARD,
  STATE_TURNING,
  STATE_FOLLOWING_OBJECT,
  STATE_AVOIDING_SEQUENCE
};
volatile RobotState currentState = STATE_STOPPED;

// Instancias Hardware
BatteryMonitor  battery(PIN_ACS, 0.185, 3.81, 2837.29);
SimpleEncoder   encLeft; SimpleEncoder encRight;
MPU6050_Driver  imu;
Servo           headServo;


//    STACKS

K_THREAD_STACK_DEFINE(imu_stack_area, 4096);
struct k_thread imu_thread_data;

K_THREAD_STACK_DEFINE(logic_stack_area, 8192);
struct k_thread logic_thread_data;

K_THREAD_STACK_DEFINE(telem_stack_area, 4096);
struct k_thread telem_thread_data;


//        HILO 1: SENSORES (ALTA PRIORIDAD)

void imu_thread_entry(void *, void *, void *) {
    float ax, ay, az, gx, gy, gz;
    float gyroZOffset = 0.0;
    float angleAccumulator = 0.0;

    // Calibración local
    long sumGz = 0;
    for(int i=0; i<500; i++) {
        imu.getProcessedData(ax, ay, az, gx, gy, gz);
        sumGz += gz;
        k_msleep(2);
    }
    gyroZOffset = sumGz / 500.0;

    while (1) {
        imu.getProcessedData(ax, ay, az, gx, gy, gz);
        float velocityZ = gz - gyroZOffset;
        if (abs(velocityZ) < 1.0) velocityZ = 0;

        angleAccumulator += velocityZ * 0.01;

        k_mutex_lock(&data_mutex, K_FOREVER);
        shared_angleZ = angleAccumulator;
        k_mutex_unlock(&data_mutex);

        k_msleep(10);
    }
}


//        HILO 2: LÓGICA (MEDIA PRIORIDAD)

//  HELPERS
float get_angle() {
    k_mutex_lock(&data_mutex, K_FOREVER);
    float a = shared_angleZ;
    k_mutex_unlock(&data_mutex);
    return a;
}

// Esta función se ejecuta cuando Python manda algo, conectada por Bridge.
void update_bbox(String label, int confidence, int x, int y) {
    k_mutex_lock(&data_mutex, K_FOREVER);
    shared_label = label;
    shared_confidence = confidence;
    shared_objX = x;
    shared_objY = y;
    shared_lastVisionTime = millis();
    k_mutex_unlock(&data_mutex);
}

// Esta función accede de manera segura a los datos compartidos de visión
void get_vision_data(int &x, int &conf, unsigned long &time) {
    k_mutex_lock(&data_mutex, K_FOREVER);
    x = shared_objX;
    conf = shared_confidence;
    time = shared_lastVisionTime;
    safe_print_val("Recibido x = ", x);
    k_mutex_unlock(&data_mutex);
}

// Función auxiliar para mantener recto con los datos compartidos del IMU
void keep_straight() {
    float current = get_angle();
    float error = shared_targetHeading - current;
    int correction = (int)(error * Kp_STRAIGHT);
    correction = constrain(correction, -MAX_CORRECTION, MAX_CORRECTION);
    motors_drive(BASE_SPEED - correction, BASE_SPEED + correction);
}

// Funciones auxiliares para imprimir seguro por el Monitor Serial
void safe_print(String msg) {
    k_mutex_lock(&print_mutex, K_FOREVER);
    Monitor.println(msg);
    k_mutex_unlock(&print_mutex);
}

void safe_print_val(String label, int val) {
    k_mutex_lock(&print_mutex, K_FOREVER);
    Monitor.print(label); Monitor.println(val);
    k_mutex_unlock(&print_mutex);
}

// Función auxiliar para ejecutar la evasión automática
void run_avoidance_sequence() {
    // Paramos motores
    motors_stop();
    safe_print("RTOS: Evasión iniciada...");

    // Pausa inicial para estabilizar lecturas
    k_msleep(200);


    // FASE 1: ESCANEO


    // MIRAR IZQUIERDA
    headServo.write(160);
    k_msleep(500);

    int l_dist = sonar_get_distance();
    safe_print_val("Servo Izq: ", l_dist);

    // MIRAR DERECHA
    headServo.write(20);
    k_msleep(1000);

    int r_dist = sonar_get_distance();
    safe_print_val("Servo Der: ", r_dist);

    // RECENTRAR
    headServo.write(90);
    k_msleep(500);


    // FASE 2: ACCIÓN FÍSICA


    if (l_dist > r_dist && l_dist > OK_DISTANCE) {
         safe_print("Decision: Giro Izquierda");
         motors_drive(-220, 220); // Giro físico Izquierda
         k_msleep(TURN_90_TIME);
    }
    else if (r_dist > l_dist && r_dist > OK_DISTANCE) {
         safe_print("Decision: Giro Derecha");
         motors_drive(220, -220); // Giro físico Derecha
         k_msleep(TURN_90_TIME);
    }
    else {
         safe_print("Decision: Callejón -> Atrás");
         // Retroceder
         motors_drive(-180, -180);
         k_msleep(BACKUP_TIME);

         // Giro de 180 grados para salir
         motors_drive(220, -220);
         k_msleep(TURN_90_TIME * 2);
    }

    // Restablecer estado
    shared_targetHeading = get_angle(); // Actualizar rumbo al nuevo frente
    currentState = STATE_FORWARD;
    motors_stop();
}

void logic_thread_entry(void *, void *, void *) {
    safe_print("RTOS: Hilo Logica Iniciado");

    // Variable para recordar hacia dónde gira manualmente
    static int manualTurnDir = 0; // -1 Izq, 1 Der

    while (1) {
        // LEER MANDO
        uint32_t ir_code = remote_read();

        if (ir_code != 0) {
            // Si se acciona cualquier botón, se toma el control manual
            switch (ir_code) {
                case IR_FORWARD:
                    safe_print("STATE: FORWARD");
                    currentState = STATE_FORWARD;
                    shared_targetHeading = get_angle(); // Fijar rumbo actual
                    break;

                case IR_BACK:
                    safe_print("STATE: BACK");
                    currentState = STATE_BACKWARD;
                    shared_targetHeading = get_angle();
                    break;

                case IR_LEFT:
                    safe_print("STATE: LEFT");
                    currentState = STATE_TURNING;
                    manualTurnDir = -1;
                    break;

                case IR_RIGHT:
                    safe_print("STATE: RIGHT");
                    currentState = STATE_TURNING;
                    manualTurnDir = 1;
                    break;

                case IR_STOP:
                    safe_print("STATE: STOP");
                    currentState = STATE_STOPPED;
                    motors_stop();
                    break;

                case IR_MODE:
                    safe_print("STATE: FOLLOWING");
                    currentState = STATE_FOLLOWING_OBJECT;
                    break;
            }
        }

        //  MAQUINA DE ESTADOS
        switch (currentState) {

            //  AVANCE RECTO (CON EVASIÓN)
            case STATE_FORWARD:
                keep_straight();
                {
                  // Chequeo de obstáculos
                  int d = sonar_get_distance();
                  if (d > 0 && d < MIN_DISTANCE) {
                      currentState = STATE_AVOIDING_SEQUENCE;
                  }
                }
                k_msleep(50);
                break;

            case STATE_BACKWARD:
                {
                   float current = get_angle();
                   float error = shared_targetHeading - current;
                   int correction = (int)(error * Kp_STRAIGHT);
                   motors_drive(-BASE_SPEED - correction, -BASE_SPEED + correction);
                }
                k_msleep(50);
                break;

            //  GIRO MANUAL (MANDO)
            case STATE_TURNING:
                // Giro sobre el eje (Tank Turn)
                if (manualTurnDir == -1) {
                    motors_drive(220, -220); // Izquierda
                } else {
                    motors_drive(-220, 220); // Derecha
                }
                k_msleep(50);
                break;

            //  EVASIÓN AUTOMÁTICA
            case STATE_AVOIDING_SEQUENCE:
                run_avoidance_sequence();
                break;

            //  SEGUIMIENTO DE OBJETOS
            case STATE_FOLLOWING_OBJECT:
                {

                    // Consulta el sonar antes de moverse
                    int dist = sonar_get_distance();

                    // Si hay obstáculo cerca (y la lectura es válida > 0)
                    if (dist > 0 && dist < MIN_DISTANCE) {
                        safe_print("¡EMERGENCIA! Obstáculo detectado");
                        motors_stop();

                        break;
                    }

                    int x, conf;
                    unsigned long time;
                    get_vision_data(x, conf, time);
                    safe_print_val("x: ",x);

                    if (millis() - time > 1500) {
                        motors_stop();
                    }
                    else if (conf > 40) { // Umbral de confianza
                        int errorX = x - FRAME_CENTER_X;
                        // Zona Muerta: Si está centrado, error es 0
                        if (abs(errorX) < 20) errorX = 0;
                        // Control Proporcional
                        int turnCorrection = (int)(errorX * Kp_VISION);

                        // CASO A: Objeto muy desviado -> Giro Pivotante Rápido
                        if (abs(errorX) > 100) {
                            safe_print("Modo tanque");
                            if (errorX > 0) motors_drive(220, -220);
                            else            motors_drive(-220, 220);
                        } else {
                            safe_print("Modo Suave");
                            int leftSpeed = 200 - turnCorrection;
                            int rightSpeed = 200 + turnCorrection;

                            // Fix Velocidad Mínima
                            auto fixSpeed = [](int spd) -> int {
                                if (abs(spd) < 40) return 0;
                                if (spd > 0 && spd < 130) return 130;
                                if (spd < 0 && spd > -130) return -130;
                                return spd;
                            };
                            leftSpeed = constrain(fixSpeed(leftSpeed), -255, 255);
                            rightSpeed = constrain(fixSpeed(rightSpeed), -255, 255);
                            motors_drive(-leftSpeed, -rightSpeed);
                        }
                    } else {
                        safe_print("Motor parado");
                        motors_stop();
                    }
                  }
                break;
            //  PARADO
            case STATE_STOPPED:
                motors_stop();
                k_msleep(100);
                break;

            default:
                k_msleep(100);
                break;
        }
    }
}


//      HILO 3: TELEMETRÍA (BAJA PRIORIDAD)
String get_telemetry() {
    // 1. Lecturas de sensores
    float volts = battery.getSmoothedVoltage();
    int pct = battery.getPercentage();
    float ang = get_angle();
    float spd = (encLeft.getSpeed() + encRight.getSpeed()) / 2.0;

    // 3. Lectura protegida del Label (String)
    k_mutex_lock(&data_mutex, K_FOREVER);
    String lbl = shared_label;
    k_mutex_unlock(&data_mutex);

    // 4. Construcción del JSON
    String json = "{";
    json += "\"v\":" + String(volts, 2) + ",";       // Voltaje
    json += "\"bat\":" + String(pct) + ",";          // Porcentaje Batería
    json += "\"spd\":" + String(spd, 2) + ",";       // Velocidad km/h
    json += "\"ang\":" + String(ang, 1);       // Ángulo IMU
    json += "}";

    return json;
}

void telemetry_thread_entry(void *, void *, void *) {
    while (1) {
        battery.update();
        k_msleep(500);
    }
}


//                  SETUP
void setup() {
    // INICIALIZAR HARDWARE PRIMERO
    Wire.begin();
    Bridge.begin();
    Monitor.begin();
    delay(6000);
    Monitor.println("--- ARRANQUE DEL SISTEMA ---");

    //  RPCs
    Bridge.provide("get_telemetry", get_telemetry);
    Bridge.provide("update_bbox", update_bbox);

    //  HARDWARE
    motors_init(); // motores
    remote_init(); // mando
    sonar_init(PIN_TRIG, PIN_ECHO); // ultrasonido
    battery.begin(); // batería
    encLeft.begin(2, WHEEL_DIA, TICKS); // encoder izq
    encRight.begin(10, WHEEL_DIA, TICKS); // encoder der

    imu.begin(); // IMU
    headServo.attach(PIN_SERVO); // servo
    headServo.write(90); // Mirar al frente al arrancar
    delay(1000);

    Monitor.println("HARDWARE LISTO. INICIANDO HILOS RTOS");

    // Hilo IMU (Prioridad 2 - ALTA)
    k_thread_create(&imu_thread_data, imu_stack_area,
                    K_THREAD_STACK_SIZEOF(imu_stack_area),
                    imu_thread_entry,
                    NULL, NULL, NULL,
                    2, 0, K_NO_WAIT);

    // Hilo Lógica (Prioridad 5 - MEDIA)
    k_thread_create(&logic_thread_data, logic_stack_area,
                    K_THREAD_STACK_SIZEOF(logic_stack_area),
                    logic_thread_entry,
                    NULL, NULL, NULL,
                    5, 0, K_NO_WAIT);

    // Hilo Telemetría (Prioridad 8 - BAJA)
    k_thread_create(&telem_thread_data, telem_stack_area,
                    K_THREAD_STACK_SIZEOF(telem_stack_area),
                    telemetry_thread_entry,
                    NULL, NULL, NULL,
                    8, 0, K_NO_WAIT);

    Monitor.println("HILOS ARRANCADOS.");
}

void loop() {
    k_msleep(1000);
}
