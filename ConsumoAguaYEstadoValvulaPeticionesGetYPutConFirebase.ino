#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

const char* ssid = "Megacable_pkPwtFp";
const char* password = "Eh8SfHtS5qc5pQ9cJw";

const int sensorPin = 2;
int relay = 4; // for ESP32 microcontroller
const int measureInterval = 2500;
volatile int pulseConter;

// YF-S201
const float factorK = 7.5;

float volume = 0;
long t0 = 0;

const char* firebase_host = "consumodeaguapi-default-rtdb.firebaseio.com";
const char* firebase_path = "/Consumodeagua/SensordeFlujo/1.json";

void ISRCountPulse()
{
   pulseConter++;
}

float GetFrequency()
{
   pulseConter = 0;

   interrupts();
   delay(measureInterval);
   noInterrupts();

   return (float)pulseConter * 1000 / measureInterval;
}

void SumVolume(float dV)
{
   volume += dV / 60 * (millis() - t0) / 1000.0;
   t0 = millis();
}

void setup() {
   attachInterrupt(digitalPinToInterrupt(sensorPin), ISRCountPulse, RISING);
   t0 = millis();
  pinMode(relay, OUTPUT);
  // Modificar el registro en la base de datos Firebase

  Serial.begin(115200);
  // put your setup code here, to run once:
WiFi.begin(ssid, password);

while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.println("Conectando a red WiFi...");
}

Serial.println("Conexión a red WiFi establecida");
}

void loop() {
  // put your main code here, to run repeatedly:
HTTPClient http;
http.begin("https://consumodeaguapi-default-rtdb.firebaseio.com/Consumodeagua/Valvula.json");

int httpCode = http.GET();

if (httpCode > 0) {
  String response = http.getString();
  
JSONVar objetos=JSON.parse(response);
bool Estado1=(bool) objetos["Estado"];
Serial.println("------------------------------Datos------------------------------");
  if (Estado1 == true) {
    Serial.println("El valor es verdadero (true)");
    digitalWrite(relay, HIGH);
  } else if (Estado1 == false) {
    Serial.println("El valor es falso (false)");
    digitalWrite(relay, LOW);
  } else {
    Serial.println("Respuesta desconocida: " + response);
  }
} else {
  Serial.println("Error en la petición HTTP");
}
  // Modificar el registro de flujo con un valor entero de 1234
  modifyFlowSensor(volume);

   // obtener frecuencia en Hz
   float frequency = GetFrequency();

   // calcular caudal L/min
   float flow_Lmin = frequency / factorK;
   SumVolume(flow_Lmin);
Serial.println("------------------------------Fin------------------------------");
delay(500);
http.end();
}

void modifyFlowSensor(float flowValue) {
  HTTPClient http;
  String endpoint = "https://consumodeaguapi-default-rtdb.firebaseio.com/Consumodeagua/SensordeFlujo/1.json";
  String payload = "{\"flujo\": " + String(flowValue) + "}";

  http.begin(endpoint);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.sendRequest("PUT", payload);

  if (httpResponseCode > 0) {
    Serial.print("Modificación exitosa del registro de flujo con valor ");
    Serial.println(flowValue);
  } else {
    Serial.print("Error en la modificación del registro de flujo. Código de respuesta HTTP: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}
