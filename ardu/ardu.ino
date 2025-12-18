/********************** 
 * ARDUINO 
***********************/
#include "DHT.h"
#include "Wire.h"
#include "Servo.h"

#define SERVO_PIN 12 //mudar
#define LDR_PIN A3
#define DHT_PIN 8 
#define ULT_TRIG_PIN 10
#define ULT_ECHO_PIN 11
#define DHTTYPE DHT11
#define SOUND_SPEED 0.034
#define TIME_INTERVAL 500 /* ms */
#define SERVO_TIME 3000 //timer do servo
#define N_AMOSTRAS 10  // Número de leituras para a média móvel (Suavização)

/*PINOS DO SENSOR DE COR*/
#define S0_PIN 4  //escala de freq
#define S1_PIN 5  //escala de freq
#define S2_PIN 6  //filtro de cor
#define S3_PIN 7  //filtro de cor
#define OUT_PIN 3

#define RED 30
#define GREEN 30
#define BLUE 40

#define FOOD_TIME 10000

Servo servinho;
int servoState = 0;
int servoEN = 0;
int servoForce = 0;
unsigned long servoTimer, food_timer;

int first;

int red, blue, green;

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





/*FUNÇAO DE LEITURA DE COR*/
void getColors(){
  digitalWrite(S2_PIN, LOW);
  digitalWrite(S3_PIN, LOW);
  red = pulseIn(OUT_PIN, digitalRead(OUT_PIN) == HIGH ? LOW : HIGH);
  delay(20);

  digitalWrite(S3_PIN, HIGH);
  blue = pulseIn(OUT_PIN, digitalRead(OUT_PIN) == HIGH ? LOW : HIGH);
  delay(20);

  digitalWrite(S2_PIN, HIGH);
  green = pulseIn(OUT_PIN, digitalRead(OUT_PIN) == HIGH ? LOW : HIGH);
  delay(20);
}

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
  if (((oc == 0) && (servoEN)) || (oc == 2)){   //servo abrindo
    servinho.write(90);
    servoTimer = millis();
    servoState = 1;

    return 1;
  } else if (oc == 1){    //servo fechando
    servinho.write(0);
    servoState = 0;
    servoForce = 0;

    return 0;
  } else {
    return -1;
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
    case 3: valor = 3; servoForce = 1; break;
    default: valor = -1.0; break;
  }
  if(cmd<3) /* Envia só caso seja um dado */
    Wire.write((byte*)&valor, sizeof(float));
}


bool temComida(){
  getColors();

  bool rval = ((red >= RED - 5) && (red <= RED + 5));
  bool gval = ((green >= GREEN - 5) && (green <= GREEN + 5));
  bool bval = ((blue >= BLUE - 5) && (blue <= BLUE + 5));

  return (rval && gval && bval);
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

  //pinos do sensor de cor
  pinMode(S0_PIN, OUTPUT);
  pinMode(S1_PIN, OUTPUT);
  pinMode(S2_PIN, OUTPUT);
  pinMode(S3_PIN, OUTPUT);
  pinMode(OUT_PIN, INPUT);

  //escala de freq 100% do sensor de cor
  digitalWrite(S0_PIN, HIGH);
  digitalWrite(S1_PIN, HIGH);

  first = 1;
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
  
  getColors();

if (!temComida()){
  servoEN = 1;
  Serial.println("===== NAO TEM COMIDA =====");
} else {
  servoEN = 0;
}


  // Serial.println("CORES");
  // Serial.print("RED: ");
  // Serial.print(red);
  // Serial.print("BLUE: ");
  // Serial.print(blue);
  // Serial.print("GREEN: ");
  // Serial.print(green);

  //checa timer do servo
  if (servoState && ((millis() - servoTimer) > SERVO_TIME)){
    ctrlServo(1);
  }

  if (((millis() - food_timer) >= FOOD_TIME) && servoEN){
    ctrlServo(0);
    servoEN = 0;
    food_timer = millis();
  }

  if (servoForce) {
    ctrlServo(2);
    servoForce = 0;
  }

  if (first){
    first = 0;
    food_timer = millis();
  }

  //red 30 green 30 blue 40
  
  delay(100); // Delay aumentado para facilitar leitura
}