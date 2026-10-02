#include "DHT.h"

#define DHTPIN 21
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);

  dht.begin();

  Serial.println("Sistema iniciado!");
}

void loop() {
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();

  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("Erro ao ler o DHT22!");
    delay(2000);
    return;
  }

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" °C");

  Serial.print("Umidade: ");
  Serial.print(umidade);
  Serial.println(" %");

  Serial.println("--------------------");

  delay(2000);
}