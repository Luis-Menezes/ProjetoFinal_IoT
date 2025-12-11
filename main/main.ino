/********************** 
ESP 32
***********************/
#include "Wire.h"
#include <WiFi.h>
#include <PubSubClient.h>


/*Wifi & MQTT connections*/
const char *WIFI_SSID = "tutu";
const char *MQTT_SERVER = "mqtt.flespi.io";
const char *WIFI_PASSWORD = "amor1234";
const uint16_t MQTT_PORT = 1883;
const char* mqtt_user = "965MM2B0TCIfmV02asHjKAeT3heGN8ZiafXc0BwINyEDGR9PuUpf0GM49IelHMFd";
;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);


/* PORTS */
#define SLAVE_ADDR 8
#define INTERVAL_TIME 500 /* ms */

/*TCS3200*/
#define S0_PIN 61632
#define S1_PIN 61632
#define S2_PIN 61632
#define S3_PIN 61632
#define OUT_PIN 61632

int rgb[2];

/* VARIABLES */
#define N_SENSORS 3
float sensor_values[N_SENSORS] = {0.0, 0.0, 0.0}; // Sensor values stores arrays
unsigned long previousMillis = 0;

/* Variáveis de momento */
bool is_tank_empty = 0; // 
bool has_food_in_bowl = 0; // 0 - sem comida, 1 - com comida
bool has_water_in_bowl = 0; // 0 - sem agua, 1 - com agua

/*COLOR RECOGNITION*/
void getColors(){
  /*S2 E S3 LOW -> RED*/
  digitalWrite(S2_PIN, LOW);
  digitalWrite(S3_PIN, LOW);
}



/* CONFIGURACAO: COMUNICACAO I2C*/

/*Envia o comando para o dispositivo secundario, solicitando o dado */
void enviarI2C(int comando) {
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(comando);
  Wire.endTransmission();
}

/* Recebe o dado do dispositivo secundario, como float*/
float receberFloat() {
  float valor = 0.0;

  Wire.requestFrom(SLAVE_ADDR, sizeof(float));
  byte *p = (byte*)&valor;
  int i = 0;

  while (Wire.available() && i < sizeof(float)) {
    p[i++] = Wire.read();
  }
  return valor;
}

void print_sensors(){
  Serial.print("LDR: ");
  Serial.print(sensor_values[0]);
  Serial.print(" | DHT Humidity: ");
  Serial.print(sensor_values[1]);
  Serial.print(" | Ultrassound (cm): ");
  Serial.println(sensor_values[2]);
}

void recebeDados(){
  for(int i=0; i<N_SENSORS; i++){
    enviarI2C(i); // Envia comando 0, 1 ou 2    
    sensor_values[i] = receberFloat(); // Recebe o valor correspondente 
  }
  print_sensors();
}

/* CONFIGURACAO: WIFI */
void ConnectToWiFi(){
  Serial.print("Connecting to WiFi ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nConnected to ");
  Serial.println(WIFI_SSID);
}

/* CONFIGURACAO MQTT */
void ConnectToMqtt(){
  Serial.println("Connecting to MQTT Broker...");
  while (!mqttClient.connected()){
    char clientId[100] = "\0";
    sprintf(clientId, "ESP32Client-%04X", random(0xffff));
    Serial.print("ClientID: ");
    Serial.println(clientId);

    // *** AQUI É A PARTE IMPORTANTE: TOKEN COMO USERNAME ***
    if (mqttClient.connect(clientId, mqtt_user, mqtt_pass)){
      Serial.println("Connected to MQTT broker."); 
      mqttClient.subscribe("test");   // tópico de teste
    } else {
      Serial.print("Falha na conexão, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" tentando novamente em 2 segundos...");
      delay(2000);
    }
  }
}

void CallbackMqtt(char* topic, byte* payload, unsigned int length){
  Serial.println("ReceivedMessage!"); 
  String t = String(topic);
  String msg = "";

  for (int i = 0; i < length; i++) msg += (char)payload[i];
  char buf[20];

  // ===== PEDIDOS AO NOSSO GRUPO (MAULA) =====
  if (t == MENINOS.tpc_dist_pedido) {
    if (msg == "pedir") {
      dtostrf(vetor[2], 6, 3, buf);  // distância local
      mqttClient.publish(MENINOS.tpc_dist_resposta, buf);
    }

  } else if (t == MENINOS.tpc_temp_pedido) {
    if (msg == "pedir") {
      dtostrf(vetor[1], 6, 3, buf);  // temperatura local
      mqttClient.publish(MENINOS.tpc_temp_resposta, buf);
    }

  } else if (t == MENINOS.tpc_led_pedido) {
    if (msg == "toggle") {
      Pisca();
      mqttClient.publish(MENINOS.tpc_led_resposta, luz ? "1" : "0");
    }

  // ===== RESPOSTAS DO GRUPO REMOTO (MENINOS) =====
  } else if (t == MAULA.tpc_dist_resposta) {
    Serial.print("DISTÂNCIA Grupo 2: ");
    Serial.print(msg);
    Serial.println(" cm");

  } else if (t == MAULA.tpc_temp_resposta) {
    Serial.print("TEMPERATURA Grupo 2: ");
    Serial.print(msg);
    Serial.println(" °C");

  } else if (t == MAULA.tpc_led_resposta) {
    Serial.println("LED do Grupo 2 FOI ALTERADO!");
  }
}


void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(CallbackMqtt);

  ConnectToWiFi();

  /*TCS3200 CONFIG*/
  pinMode(S0_PIN, OUTPUT);
  pinMode(S1_PIN, OUTPUT);
  pinMode(S2_PIN, OUTPUT);
  pinMode(S3_PIN, OUTPUT);

  pinMode(OUT_PIN, INPUT);

  digitalWrite(S0_PIN, HIGH);
  digitalWrite(S1_PIN, HIGH);
}

int x = 0;
void loop() {
  /* Will read from Arduino in an interval */
  if(millis() - previousMillis >= INTERVAL_TIME){
    previousMillis = millis();
    recebeDados();
  }
  /* Checks food tank level */
  is_tank_empty = (sensor_values[2] > 15.0); /* See if values are correct */
  if(is_tank_empty){ /* Envia alerta MQTT */
    Serial.println("ALERT: Food tank is empty!");
  }

  has_water_in_bowl = (sensor_values[0] < 500.0); /* Example threshold */

  if (!mqttClient.connected()) ConnectToMqtt();
  mqttClient.loop();

  if(x == 0){
      
    mqttClient.publish("test", "oiii");
    x++;
  }

}
