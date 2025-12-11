/********************** 
ESP 32
***********************/
#include "Wire.h"

/* PORTS */


/*CONFIGURACAO: COMUNICACAO I2C*/

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

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  
}
