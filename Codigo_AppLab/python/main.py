import time
import sys
import threading
import json
from threading import Lock
from arduino.app_utils import *
from iot_client import IoTClient
from arduino.app_bricks.video_objectdetection import VideoObjectDetection

#  CONFIGURACIÓN
MQTT_BROKER = "10.149.28.20"
MQTT_TOPIC = "robot/telemetria"

#  SEMÁFORO (MUTEX)
bridge_lock = Lock()

#  HILO DE TELEMETRÍA (BAJA PRIORIDAD / NO BLOQUEANTE)
def telemetry_poller(iot_client):
    time.sleep(3)
    print("[Python] Iniciando hilo de telemetria (Modo no bloqueante)...")

    while True:
        try:
            response = None

            # blocking=False: "Intenta coger el candado. Si está ocupado, devuelve False."
            is_acquired = bridge_lock.acquire(blocking=False)

            if is_acquired:
                try:
                    # Canal libre
                    response = Bridge.call("get_telemetry")
                finally:
                    # Liberar candado
                    bridge_lock.release()

                # Procesamos la respuesta
                if response:
                    try:
                        data = json.loads(response)
                        iot_client.update_data(data)
                    except ValueError:
                        pass
            else:
                # VISION está usando el Bridge.
                # print("[Python] Telemetria saltada por prioridad de Vision")
                pass

        except Exception as e:
            print(f"[Python] Error Telemetria: {e}")

        # Frecuencia de Telemetría
        time.sleep(0.2)

#  CALLBACK DE VISIÓN (ALTA PRIORIDAD)
def notify_microcontroller(detections: dict):
    if not detections:
        return

    # Procesar todas las detecciones
    for label, data in detections.items():
        confianza_pct = int(data['confidence'] * 100)

        # Filtro previo
        if confianza_pct < 40:
            continue

        bbox = data['bounding_box_xyxy']
        center_x = int((bbox[0] + bbox[2]) / 2)
        center_y = int((bbox[1] + bbox[3]) / 2)

        # print(f"Vision: {label} ({confianza_pct}%) X:{center_x}")

        #  LLAMADA RPC (MODO BLOQUEANTE / PRIORITARIO)
        try:
            with bridge_lock:
                Bridge.call("update_bbox", str(label), int(confianza_pct), int(center_x), int(center_y))

        except Exception as e:
            print(f"Error Vision RPC: {e}")

#  MAIN
print(" INICIANDO ROBOT ")

# Iniciar hilo de MQTT
mqtt_worker = IoTClient(MQTT_BROKER, MQTT_TOPIC)
mqtt_worker.start()

# Iniciar hilo de telemetría
t_poller = threading.Thread(target=telemetry_poller, args=(mqtt_worker,))
t_poller.daemon = True
t_poller.start()

# Configuración de detección
detection_stream = VideoObjectDetection(confidence=0.4, debounce_sec=0.0)
detection_stream.on_detect_all(notify_microcontroller)

print("[Python] Sistema listo.")
App.run()