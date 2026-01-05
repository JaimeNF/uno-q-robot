#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// 1. CONFIGURACIÓN DE RED
const char* ssid = "MIWIFI_domo";
const char* password = "aDeZETA@22";

// 2. CONFIGURACIÓN MQTT
const char* mqtt_server = "192.168.1.100"; 
const int mqtt_port = 1883;
const char* mqtt_user = "mqtt_user";
const char* mqtt_pass = "root";
const char* topic_sub = "robot/telemetria";

// Objetos globales
WiFiClient espClient;
PubSubClient client(espClient);
AsyncWebServer server(80);
AsyncEventSource events("/events");

// Variables para guardar datos (Backup)
float currentSpd = 0.0;
int currentBat = 0;
float currentAng = 0.0;

// Variables de control de tiempo (para reconexión no bloqueante)
unsigned long lastMqttReconnectAttempt = 0;

// 3. CÓDIGO HTML + JS (Frontend con Brújula y Nuevas Variables)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Dashboard Robot UNO Q</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    /* ESTILOS GENERALES (Dark Mode) */
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; text-align: center; background-color: #121212; color: white; margin: 0; padding-bottom: 50px;}
    .header { background: linear-gradient(90deg, #03dac6 0%, #018786 100%); color: #000; padding: 15px; margin-bottom: 30px; box-shadow: 0 4px 15px rgba(3, 218, 198, 0.4);}
    h1 { margin: 0; font-size: 1.5rem; letter-spacing: 2px; text-transform: uppercase;}
    .container { display: flex; justify-content: center; flex-wrap: wrap; gap: 30px; max-width: 1000px; margin: 0 auto;}
    
    /* TARJETAS */
    .card { background-color: #1e1e1e; padding: 20px; border-radius: 20px; width: 280px; position: relative; box-shadow: 0 10px 20px rgba(0,0,0,0.5); border: 1px solid #333; display: flex; flex-direction: column; align-items: center; justify-content: space-between;}
    .card h3 { color: #bb86fc; font-size: 1rem; margin-top: 0; text-transform: uppercase; letter-spacing: 1px;}

    /* 1. VELOCÍMETRO */
    .gauge-wrapper { position: relative; width: 200px; height: 100px; overflow: hidden; margin-top: 20px;}
    .gauge-bg { 
      position: absolute; width: 200px; height: 200px; background: #333; border-radius: 50%; top: 0; 
      background: conic-gradient(from 270deg, #03dac6 0deg, #bb86fc 120deg, #cf6679 180deg);
      mask-image: radial-gradient(transparent 65%, black 66%); -webkit-mask-image: radial-gradient(transparent 65%, black 66%);
    }
    .gauge-needle {
      position: absolute; bottom: 0; left: 50%; width: 4px; height: 90px; background: white;
      transform-origin: bottom center; transform: rotate(-90deg);
      transition: transform 0.3s ease-out; border-radius: 5px; z-index: 10;
    }
    .gauge-center-dot { position: absolute; bottom: -10px; left: 50%; transform: translateX(-50%); width: 20px; height: 20px; background: white; border-radius: 50%; box-shadow: 0 0 10px rgba(255,255,255,0.5); z-index: 11;}
    .value-text { font-size: 2.5rem; font-weight: bold; z-index: 12; margin-top: -10px; text-shadow: 0 2px 4px black;}
    .unit { font-size: 0.9rem; color: #888; display: block;}

    /* 2. BATERÍA */
    .battery-shell {
      width: 140px; height: 60px; border: 4px solid #888; border-radius: 10px; position: relative; padding: 3px; margin: 20px 0;
      display: flex; align-items: center; justify-content: flex-start;
    }
    .battery-shell::after { content: ''; position: absolute; right: -12px; top: 18px; width: 6px; height: 24px; background: #888; border-radius: 0 4px 4px 0;}
    .battery-level {
      height: 100%; width: 0%; background: linear-gradient(90deg, #00b09b, #96c93d); border-radius: 5px;
      transition: width 0.5s ease, background 0.5s ease;
      box-shadow: 0 0 15px rgba(150, 201, 61, 0.4);
    }
    .bat-text { position: absolute; width: 100%; text-align: center; top: 50%; transform: translateY(-50%); font-weight: bold; text-shadow: 1px 1px 2px black; font-size: 1.4rem; z-index: 5;}

    /* 3. BRÚJULA (NUEVO) */
    .compass-container {
      position: relative; width: 140px; height: 140px; border: 4px solid #333; border-radius: 50%;
      background: radial-gradient(circle, #222 0%, #000 100%);
      box-shadow: inset 0 0 20px #000, 0 0 10px rgba(3, 218, 198, 0.2);
      margin: 10px 0; display: flex; align-items: center; justify-content: center;
    }
    /* Marcas cardinales */
    .cardinal { position: absolute; color: #555; font-weight: bold; font-size: 0.9rem; }
    .n { top: 5px; color: #cf6679; text-shadow: 0 0 5px #cf6679;} /* Norte Rojo */
    .s { bottom: 5px; }
    .e { right: 10px; }
    .w { left: 10px; }

    /* Aguja magnética */
    .compass-needle {
      position: absolute; width: 80%; height: 80%;
      transition: transform 0.5s cubic-bezier(0.4, 1.5, 0.6, 1); /* Rebote elástico */
    }
    .arrow-head {
      width: 0; height: 0; border-left: 10px solid transparent; border-right: 10px solid transparent;
      border-bottom: 40px solid #03dac6; position: absolute; top: 10px; left: 50%; transform: translateX(-50%);
      filter: drop-shadow(0 0 5px #03dac6);
    }
    .arrow-tail {
      width: 4px; height: 40px; background: #555; position: absolute; bottom: 30px; left: 50%; transform: translateX(-50%);
    }
    .angle-text { font-family: 'Courier New', Courier, monospace; color: #03dac6; font-size: 1.5rem; margin-top: 5px;}
  </style>
</head>
<body>
  <div class="header"><h1>Control UNO Q</h1></div>
  
  <div class="container">
    
    <div class="card">
      <h3>Velocidad</h3>
      <div class="gauge-wrapper">
        <div class="gauge-bg"></div>
        <div class="gauge-needle" id="needle"></div>
        <div class="gauge-center-dot"></div>
      </div>
      <div style="margin-top: -30px; position: relative; z-index: 20;">
        <span id="spd-val" class="value-text">0.00</span>
        <span class="unit">Km/h</span>
      </div>
    </div>

    <div class="card">
      <h3>Bateria</h3>
      <div class="battery-shell">
        <div class="battery-level" id="bat-bar"></div>
        <div class="bat-text"><span id="bat-val">0</span>%</div>
      </div>
      <span class="unit">Nivel de Carga</span>
    </div>

    <div class="card">
      <h3>Rumbo IMU</h3>
      <div class="compass-container">
        <span class="cardinal n">N</span>
        <span class="cardinal s">S</span>
        <span class="cardinal e">E</span>
        <span class="cardinal w">W</span>
        <div class="compass-needle" id="compass-rotator">
           <div class="arrow-head"></div>
           <div class="arrow-tail"></div>
        </div>
      </div>
      <div>
        <span id="ang-val" class="angle-text">0.0</span><span style="color:#888">°</span>
      </div>
    </div>

  </div>

<script>
// CONFIGURACIÓN VISUAL
const MAX_SPEED_DISPLAY = 5.0; // Velocidad máxima esperada para el tope del gráfico (Km/h)

if (!!window.EventSource) {
 var source = new EventSource('/events');

 source.addEventListener('open', function(e) { console.log("Conectado al Robot"); }, false);

 source.addEventListener('telemetry', function(e) {
  // Parseamos el JSON: {"v":..., "bat":..., "spd":..., "ang":...}
  var obj = JSON.parse(e.data);
  
  // ==============================
  // 1. ACTUALIZAR VELOCIDAD (spd)
  // ==============================
  let spd = parseFloat(obj.spd);
  document.getElementById("spd-val").innerHTML = spd.toFixed(2);
  
  // Mover aguja (-90 a 90 grados)
  // Si spd es mayor al maximo configurado, se queda en el tope
  let spdClamped = (spd > MAX_SPEED_DISPLAY) ? MAX_SPEED_DISPLAY : spd;
  if(spdClamped < 0) spdClamped = 0;
  
  let gaugeAngle = (spdClamped / MAX_SPEED_DISPLAY) * 180 - 90;
  document.getElementById("needle").style.transform = `rotate(${gaugeAngle}deg)`;

  // ==============================
  // 2. ACTUALIZAR BATERIA (bat)
  // ==============================
  let bat = parseInt(obj.bat);
  document.getElementById("bat-val").innerHTML = bat;
  
  let bar = document.getElementById("bat-bar");
  if(bat > 100) bat = 100; if(bat < 0) bat = 0;
  bar.style.width = bat + "%";

  // Color Semáforo
  if(bat > 50) {
    bar.style.background = "linear-gradient(90deg, #00b09b, #96c93d)";
    bar.style.boxShadow = "0 0 15px rgba(150, 201, 61, 0.4)";
  } else if (bat > 20) {
    bar.style.background = "linear-gradient(90deg, #f12711, #f5af19)"; 
    bar.style.boxShadow = "none";
  } else {
    bar.style.background = "linear-gradient(90deg, #eb3349, #f45c43)";
    bar.style.boxShadow = "0 0 15px rgba(235, 51, 73, 0.6)";
  }

  // ==============================
  // 3. ACTUALIZAR BRÚJULA (ang)
  // ==============================
  // Asumimos que 0 = Norte, 90 = Este, etc.
  // Invertimos o ajustamos según tu IMU. Normalmente rotate() va en sentido horario.
  let ang = parseFloat(obj.ang);
  document.getElementById("ang-val").innerHTML = ang.toFixed(1);
  
  // Rotamos la aguja. 
  // Nota: Si tu IMU aumenta en sentido antihorario, pon -ang.
  document.getElementById("compass-rotator").style.transform = `rotate(${ang}deg)`;

 }, false);
}
</script>
</body>
</html>
)rawliteral";

// 4. LÓGICA DEL BACKEND (C++)

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  
  // JSON esperado: {"v":12.5, "bat":90, "spd":1.2, "ang":180.5}
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, msg);

  if (!error) {
    // Extraemos variables (spd, bat, ang). 'v' la ignoramos en el dashboard.
    currentSpd = doc["spd"];
    currentBat = doc["bat"];
    currentAng = doc["ang"];

    // Enviar a la Web
    events.send(msg.c_str(), "telemetry", millis());
    
    // Debug mínimo para no saturar serial
    Serial.printf("Web Update -> Spd:%.1f Bat:%d Ang:%.1f\n", currentSpd, currentBat, currentAng);
  } else {
    Serial.print("Error JSON: ");
    Serial.println(error.c_str());
  }
}

void setup_wifi() {
  delay(10);
  Serial.print("Conectando a WiFi: "); Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado.");
  Serial.print("Dashboard IP: http://");
  Serial.println(WiFi.localIP());
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  events.onConnect([](AsyncEventSourceClient *client){
    client->send("Conectado", NULL, millis(), 1000);
  });
  server.addHandler(&events);

  server.begin();
}

void loop() {
  // Lógica de reconexión MQTT NO bloqueante
  if (!client.connected()) {
    long now = millis();
    if (now - lastMqttReconnectAttempt > 5000) {
      lastMqttReconnectAttempt = now;
      Serial.print("Intentando MQTT... ");
      if (client.connect("ESP32_Dashboard_Client", mqtt_user, mqtt_pass)) {
        Serial.println("Conectado!");
        client.subscribe(topic_sub);
      } else {
        Serial.print("Fallo rc="); Serial.println(client.state());
      }
    }
  } else {
    client.loop();
  }
}