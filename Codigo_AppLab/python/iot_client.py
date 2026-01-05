import paho.mqtt.client as mqtt
import json
import time
import threading
import os
from dotenv import load_dotenv

# Cargamos las variables de entorno para seguridad
load_dotenv()

class IoTClient:
    def __init__(self, broker_ip, topic):
        self.broker = broker_ip
        self.topic = topic
        self.port = 1883
        self.client = mqtt.Client()

        # Configuración de credenciales
        user = os.getenv('MQTT_USER')
        password = os.getenv('MQTT_PASS')
        if user and password:
            self.client.username_pw_set(user, password)

        self.running = False
        self.latest_data = {}
        self.lock = threading.Lock() # Para evitar conflictos entre hilos

    def start(self):
        """Inicia la conexión y el hilo de publicación"""
        try:
            print(f"[IoT] Conectando a {self.broker}...")
            self.client.connect(self.broker, self.port, 60)
            self.client.loop_start() # Hilo interno de Paho para red

            self.running = True
            # Iniciamos nuestro propio hilo para publicar a ritmo constante
            self.pub_thread = threading.Thread(target=self._publish_loop)
            self.pub_thread.daemon = True
            self.pub_thread.start()
            print("[IoT] Servicio de Telemetría Iniciado.")

        except Exception as e:
            print(f"[IoT Error] No se pudo conectar al Broker: {e}")

    def stop(self):
        self.running = False
        self.client.loop_stop()
        self.client.disconnect()

    def update_data(self, data_dict):
        """Método seguro para actualizar los datos desde fuera"""
        with self.lock:
            self.latest_data = data_dict

    def _publish_loop(self):
        """Bucle infinito que envía datos cada 1 segundo"""
        while self.running:
            with self.lock:
                if self.latest_data:
                    payload = json.dumps(self.latest_data)
                    self.client.publish(self.topic, payload)
                    print(f"[IoT] Enviado: {payload} en {self.topic}")

            time.sleep(1.0) # Frecuencia de envío: 1Hz