# 🤖 Robót movil (Arduino UNO Q)

![Status](https://img.shields.io/badge/Status-Completed-success)
![Platform](https://img.shields.io/badge/Platform-Arduino_UNO_Q-00979D)
![OS](https://img.shields.io/badge/OS-Linux_%2B_Zephyr_RTOS-darkblue)
![Language](https://img.shields.io/badge/Languages-Python_%7C_C%2B%2B-blue)
![License](https://img.shields.io/badge/License-MIT-green)

> **Implementación de referencia para robótica híbrida asimétrica:** Visión Artificial en Linux (MPU) + Control Determinista en RTOS (MCU) integrados en un solo PCB.

---

## 📖 Descripción del Proyecto

Este proyecto valida una arquitectura de software jerárquica para robots móviles, resolviendo el problema de latencia y determinismo en sistemas distribuidos. Utilizando la plataforma **Arduino UNO Q**, el sistema combina la potencia de cálculo de un procesador **Qualcomm (Linux)** para tareas cognitivas (IA/Visión) con la fiabilidad de un microcontrolador **STM32 (Zephyr RTOS)** para la seguridad física y el control de motores.

El hardware se basa en una estrategia de *upcycling* del kit educativo **Elegoo Smart Car V3.0**, modernizándolo mediante la integración de la placa UNO Q y sensores avanzados (IMU, Encoders, Sonar) para lograr navegación autónoma inteligente.

### 🚀 Características Clave
* **Arquitectura Híbrida:** Desacoplamiento total entre la lógica de Visión (Alta Latencia) y el Control de Motores (Tiempo Real).
* **Seguridad Activa:** Frenada de emergencia y evasión de obstáculos gestionada por hardware (RTOS) con prioridad absoluta sobre la IA.
* **Visión Artificial:** Detección y seguimiento de rostros (*Face Tracking*) utilizando modelos TensorFlow Lite acelerados.
* **Sincronización RPC:** Comunicación interna MPU-MCU vía memoria compartida (`Arduino Bridge`) con latencia <10ms.
* **Telemetría IoT:** Monitorización remota de variables críticas (voltaje, velocidad, rumbo) mediante protocolo MQTT (QoS 0).

---

## 🏗️ Arquitectura del Sistema

El sistema opera mediante dos dominios de ejecución concurrentes:

1.  **Nivel Cognitivo (MPU - Linux Debian):**
    * Ejecuta scripts en **Python**.
    * Captura de video y procesamiento de Deep Learning.
    * Gestión de conectividad Wi-Fi y cliente MQTT.
    * Envía vectores de error $(X, Y)$ al MCU.

2.  **Nivel Reactivo (MCU - Zephyr OS):**
    * Ejecuta firmware en **C++**.
    * **Hilo Prioridad Alta:** Integración de datos del IMU (Giroscopio) a 100Hz.
    * **Hilo Prioridad Media:** Máquina de Estados (FSM) y lazos de control PID.
    * **Hilo Prioridad Baja:** Monitorización de batería y sensores auxiliares.

---

## 🛠️ Hardware y BOM (Bill of Materials)

| Componente | Descripción | Función |
| :--- | :--- | :--- |
| **Controlador** | Arduino UNO Q | SoC Qualcomm QRB2210 + STM32U585 |
| **Chasis** | Elegoo Smart Robot Car V3.0 | Estructura mecánica y motores TT (Upcycling) |
| **Driver Motores** | L298N Dual H-Bridge | Etapa de potencia (Integrado en Shield V3.0) |
| **Visión** | Cámara Web USB Genérica | Captura de video HD (720p) |
| **IMU** | MPU6050 | Giroscopio y Acelerómetro (I2C) |
| **Odometría** | Encoders de Efecto Hall | Medición de velocidad y distancia |
| **Distancia** | HC-SR04 | Sensor ultrasónico para seguridad |
| **Energía** | Li-Ion 2S (7.4V) + PowerBank | Alimentación dual (Motores + Lógica USB) |

---

## 📂 Estructura del Repositorio

```text
.
├── Codigo_AppLab/
│   ├── python/                 # Lógica de Alto Nivel (Linux)
│   │   ├── main.py             # Orquestador principal: Visión + RPC
│   │   ├── iot_client.py       # Cliente MQTT asíncrono
│   │   └── .env                # Credenciales (No incluido en repo)
│   └── sketch/                 # Firmware de Bajo Nivel (MCU)
│       ├── sketch.ino          # Entry point y configuración de Hilos Zephyr
│       └── src/                # Drivers de Hardware (C++)
│           ├── BatteryMonitor.cpp
│           ├── encoders.cpp
│           ├── motors.cpp
│           ├── MPU6050_Driver.cpp
│           ├── remote.cpp
│           └── sonar.cpp
│
├── complementary/
│   ├── Battery/                # Análisis Jupyter de curvas de descarga
│   ├── Datasheets/             # Hojas de datos de componentes
│   └── esp32_gateway/          # Código para monitor auxiliar (Opcional)
│
└── Docs/
    ├── Images/                 # Evidencia gráfica, diagramas y capturas
    └── Report/                 # Memoria técnica completa (LaTeX/PDF)
```
## ⚙️ Instalación y Despliegue (Arduino App Lab)

El despliegue de este proyecto se ha simplificado para realizarse íntegramente desde la interfaz web **Arduino App Lab**, sin necesidad de ejecutar comandos de terminal ni conexiones SSH complejas.

### 1. Firmware (MCU - Zephyr)
Despliegue del código de tiempo real para el control de motores y sensores:

1. Abre la interfaz **Arduino App Lab** de tu placa en el navegador.
2. Ve a la sección de edición de **Sketch** (C++).
3. Copia el contenido de la carpeta `Codigo_AppLab/sketch/` de este repositorio y pégalo en el editor del App Lab.
4. Pulsa en **Run/Compile**. El RTOS Zephyr iniciará automáticamente los 3 hilos de control.

### 2. Software (MPU - Python)
Despliegue de la lógica de visión artificial y telemetría:

1. En la misma interfaz de **Arduino App Lab**, navega a la pestaña o directorio de **Python** (Workspace).
2. Crea los archivos necesarios (`main.py`, `iot_client.py`) copiando y pegando el código fuente desde la carpeta `Codigo_AppLab/python/` de este repositorio.
3. Guarda los archivos.
4. El sistema ya está listo para ejecutarse directamente desde la interfaz.

### 3. Conexiones Físicas (Pinout)
* **Motores:** Definidos en `motors.h` (Compatibles con Shield Elegoo V3).
* **Encoders:** Pines **2** y **10** (Requieren interrupciones externas).
* **IMU:** Bus I2C estándar (**SDA**, **SCL**).
* **Sonar:** Pin **A4** (Echo) y Pin **A5** (Trig).
* **Servo:** Pin **3** (PWM).

---

## 🎮 Modos de Operación (FSM)

El comportamiento del robot se rige por una Máquina de Estados Finita:

* **`STATE_STOPPED`**: Estado de reposo seguro. Motores deshabilitados.
* **`STATE_FORWARD`**: Navegación libre asistida por giroscopio (corrección automática de deriva).
* **`STATE_FOLLOWING_OBJECT`**: Seguimiento autónomo de rostros (Control PID visual + Giro tanque si el error es alto).
* **`STATE_AVOIDING_SEQUENCE`**: Maniobra de seguridad bloqueante (*Stop-Look-Decide*) activada por el sonar.
* **`STATE_TURNING`**: Giros manuales o correcciones inerciales específicas.

---

## 📊 Resultados y Rendimiento

* **Eficiencia Computacional:** Uso de CPU equilibrado entre los 4 núcleos del procesador Qualcomm (`htop` load < 2.0 durante inferencia activa).
* **Coste del Prototipo:** ~136€ (vs 340€ de soluciones comerciales equivalentes como Duckiebot DB21).
* **Latencia:** Respuesta motriz <10ms tras comando de visión gracias a la arquitectura RPC local.
* **Autonomía:** ~45 minutos de operación continua con telemetría y visión activas.

---

## 👤 Autor y Créditos

**Jaime Nogueira Fuentes**

* Ingeniería de Software (Zephyr/Linux).
* Diseño de Algoritmos de Control y Visión.
* Integración Electrónica y Pruebas de Campo.

> *Este proyecto fue desarrollado como parte de un trabajo de una asigantura  sobre Programación de Nodos Sensores para IoT de la Universidad Complutense de Madrid.*
