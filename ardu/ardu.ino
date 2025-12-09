/********************** ARDUINO 
***********************/
#include "DHT.h"
#include "Wire.h"

#define LDR_PIN A3
#define DHT_PIN 8 
#define ULT_TRIG_PIN 10
#define ULT_ECHO_PIN 11
#define DHTTYPE DHT11
#define SOUND_SPEED 0.034
#define TIME_INTERVAL 500 /* ms */

struct sensor{
  int pin[2]; // Capacidade para até 2 pinos (índice 0 e 1)
  int outputPin;
  float value;
};

// We use '0' if pin is not used
struct sensor ldr = { {LDR_PIN, 0}, 0.0 }; 
struct sensor dht_humidity = { {DHT_PIN, 0}, 0.0 }; 
struct sensor ultrassound = { {ULT_TRIG_PIN, ULT_ECHO_PIN}, 0.0 };

DHT dht(DHT_PIN, DHTTYPE);

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

void setup() {
  // Configuração LDR
  pinMode(ldr.pin[0], INPUT);
  
  // Configuração Ultrassom (Trigger Output, Echo Input)
  pinMode(ultrassound.pin[0], OUTPUT);
  pinMode(ultrassound.pin[1], INPUT);

  Serial.begin(9600);
  Wire.begin();
  dht.begin();
}

void loop() {
  // Leitura LDR
  ldr.value = analogRead(ldr.pin[0]);
  
  // Leitura DHT
  dht_humidity.value = dht.readHumidity();
  
  // Leitura Ultrassom
  ultrassound.value = get_distance();

  Serial.print("Umid: ");
  Serial.print(dht_humidity.value);
  Serial.print(" | Luz: ");
  Serial.print(ldr.value);
  Serial.print(" | Dist: ");
  Serial.println(ultrassound.value);
  
  delay(100); // Delay aumentado para facilitar leitura
}