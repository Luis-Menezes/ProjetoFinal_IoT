/********************** 
 * ARDUINO 
***********************/
#include "DHT.h"
#include "Wire.h"
#include "Servo.h"

#define SERVO_PIN A0 //mudar
#define LDR_PIN A3
#define DHT_PIN 8 
#define ULT_TRIG_PIN 10
#define ULT_ECHO_PIN 11
#define DHTTYPE DHT11
#define SOUND_SPEED 0.034
#define TIME_INTERVAL 500 /* ms */
#define SERVO_TIME 1000 //timer do servo
#define N_AMOSTRAS 10  // Número de leituras para a média móvel (Suavização)

Servo servinho;
int servoState = 0;
unsigned long servoTimer;

// Estrutura atualizada para suportar Média Móvel
struct sensor {
  int pin[2];      // Pinos do sensor
  float leituras[N_AMOSTRAS]; // Buffer histórico
  int indice;      // Índice atual no buffer
  float soma;      // Soma atual das leituras
  float media;     // O valor final suavizado
};

// Inicialização das structs (Buffers iniciam zerados automaticamente por serem globais)
struct sensor ldr = { {LDR_PIN, 0}, {0}, 0, 0.0, 0.0 }; 
struct sensor dht_humidity = { {DHT_PIN, 0}, {0}, 0, 0.0, 0.0 }; 
struct sensor ultrassound = { {ULT_TRIG_PIN, ULT_ECHO_PIN}, {0}, 0, 0.0, 0.0 };
int cmd = 0;

DHT dht(DHT_PIN, DHTTYPE);

void atualizarSensor(struct sensor &s, float novaLeitura) {
  // Se a leitura for inválida (ex: NaN do DHT), ignoramos para não quebrar a média
  if (isnan(novaLeitura)) return;

  // Subtrai a leitura mais antiga da soma
  s.soma = s.soma - s.leituras[s.indice];
  
  // Armazena a nova leitura no buffer
  s.leituras[s.indice] = novaLeitura;
  
  // Adiciona a nova leitura à soma
  s.soma = s.soma + novaLeitura;
  
  // Avança o índice (circular)
  s.indice = (s.indice + 1) % N_AMOSTRAS;
  
  // Calcula a nova média
  s.media = s.soma / N_AMOSTRAS;
}

float get_distance(){
  float distanceCm = -1;
  long duration;
  
  // Trigger (pin[0])
  digitalWrite(ultrassound.pin[0], LOW);
  delayMicroseconds(2);
  digitalWrite(ultrassound.pin[0], HIGH);
  delayMicroseconds(10); // 
  digitalWrite(ultrassound.pin[0], LOW);

  // Echo (pin[1])
  duration = pulseIn(ultrassound.pin[1], HIGH);

  distanceCm = duration * SOUND_SPEED / 2;
  if (distanceCm>2200.0){
    return 50.0;
  } 
  return distanceCm;
}

//=== CONTROLE DO SERVO ===//
int ctrlServo(int oc){
  if (oc == 0){   //servo abrindo
    servinho.write(180);
    servoTimer = millis();
    servoState = 1;

    return 1;
  } else if (oc == 1){    //servo fechando
    servinho.write(0);
    servoState = 0;

    return 0;
  }
}

/*=== CONFIGURACAO: COMUNICACAO I2C ===*/

/* Recebe o comando do dispositivo principal */
void receiveEvent(int n) {
  if (Wire.available()){
    cmd = Wire.read();
    Serial.print("cmd recebido: ");
    Serial.println(cmd);
    // cmd = cmd - 48; 
  }
}

/* Envia o dado pedido pelo comando recebido na funcao receiveEvent*/
void enviarDados() {
  float valor = 0.0;

  switch (cmd) {
    case 0: valor = ldr.media; break;   
    case 1: valor = dht_humidity.media; break;
    case 2: valor = ultrassound.media; break;
    case 3: valor = ctrlServo(0); break;
    default: valor = -1.0; break;
  }
  if(cmd<3) /* Envia só caso seja um dado */
    Wire.write((byte*)&valor, sizeof(float));
}

void setup() {
  // Configuração LDR
  pinMode(ldr.pin[0], INPUT);
  
  // Configuração Ultrassom (Trigger Output, Echo Input)
  pinMode(ultrassound.pin[0], OUTPUT);
  pinMode(ultrassound.pin[1], INPUT);

  Serial.begin(9600);

  /*setup da comunicacao I2C*/
  Wire.begin(8);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(enviarDados);

  //setup do sensor DHT
  dht.begin();

  //atribui pino ao servo
  servinho.attach(SERVO_PIN);
}

void loop() {
  // Leitura LDR
  float raw_ldr = analogRead(ldr.pin[0]);
  float raw_dht = dht.readHumidity();
  float raw_ult = get_distance();

  // 2. Atualiza as Médias Móveis
  atualizarSensor(ldr, raw_ldr);
  atualizarSensor(dht_humidity, raw_dht);
  atualizarSensor(ultrassound, raw_ult);

  // Debug: Mostrar as médias no Serial
  Serial.print("Umid (Avg): ");
  Serial.print(dht_humidity.media);
  Serial.print(" | Luz (Avg): ");
  Serial.print(ldr.media);
  Serial.print(" | Dist (Avg): ");
  Serial.println(ultrassound.media);
  
  //checa timer do servo
  if (servoState && ((millis() - servoTimer) > SERVO_TIME)){
    ctrlServo(1);
  }
  
  delay(100); // Delay aumentado para facilitar leitura
}