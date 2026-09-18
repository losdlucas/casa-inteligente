#include <Arduino.h>
#include <ESP32Servo.h>

// =====================================
// BOTÕES
// =====================================

#define BOTAO1 19
#define BOTAO2 21
#define BOTAO3 23

#define BOTAO4 26
#define BOTAO5 27

// =====================================
// SAÍDAS
// =====================================

#define BUZZER 18

#define LED1 22
#define LED2 25

// =====================================
// PIR
// =====================================

#define PIR 33

// =====================================
// SERVO SG90
// =====================================

#define SERVO_PIN 32

Servo meuServo;

// Posição central
#define SERVO_CENTRO 90

// =====================================
// CONFIGURAÇÃO
// =====================================

#define TEMPO_SERVO 6000


// =====================================
// SETUP
// =====================================

void setup() {

  Serial.begin(115200);

  // -----------------------------------
  // BOTÕES
  // -----------------------------------

  pinMode(BOTAO1, INPUT_PULLDOWN);
  pinMode(BOTAO2, INPUT_PULLDOWN);
  pinMode(BOTAO3, INPUT_PULLDOWN);

  pinMode(BOTAO4, INPUT_PULLDOWN);
  pinMode(BOTAO5, INPUT_PULLDOWN);

  // -----------------------------------
  // LEDS
  // -----------------------------------

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);

  // -----------------------------------
  // BUZZER
  // -----------------------------------

  pinMode(BUZZER, OUTPUT);
  noTone(BUZZER);

  // -----------------------------------
  // PIR
  // -----------------------------------

  pinMode(PIR, INPUT);

  // -----------------------------------
  // SERVO
  // -----------------------------------

  meuServo.setPeriodHertz(50);

  meuServo.attach(
    SERVO_PIN,
    500,
    2400
  );

  // Começa no meio
  meuServo.write(SERVO_CENTRO);

  delay(500);

  // -----------------------------------
  // MENSAGENS
  // -----------------------------------

  Serial.println("================================");
  Serial.println("SISTEMA INICIADO");
  Serial.println("================================");

  Serial.println("BOTAO 1 -> ALARME");
  Serial.println("BOTAO 2 -> LED 1");
  Serial.println("BOTAO 3 -> LED 2");
  Serial.println("BOTAO 4 -> SERVO HORARIO");
  Serial.println("BOTAO 5 -> SERVO ANTI-HORARIO");
  Serial.println("PIR -> MOVIMENTO");
}


// =====================================
// ALARME
// =====================================

void alarme() {

  for (int i = 0; i < 4; i++) {

    tone(BUZZER, 500);

    delay(120);

    noTone(BUZZER);

    delay(70);

    if (
      digitalRead(BOTAO1) == LOW &&
      digitalRead(PIR) == LOW
    ) {

      noTone(BUZZER);

      return;
    }
  }

  noTone(BUZZER);

  delay(180);
}


// =====================================
// SERVO PARA DIREITA
// =====================================

void servoHorario() {

  Serial.println("BOTAO 4 - SERVO 0 -> 180 GRAUS");

  for (int pos = 0; pos <= 180; pos += 2) {
    meuServo.write(pos);
    delay(20);
  }

  Serial.println("SERVO EM 180 GRAUS");

  delay(6000);

  Serial.println("SERVO PARADO");
}


void servoAntiHorario() {

  Serial.println("BOTAO 5 - SERVO 180 -> 0 GRAUS");

  for (int pos = 180; pos >= 0; pos -= 2) {
    meuServo.write(pos);
    delay(20);
  }

  Serial.println("SERVO EM 0 GRAUS");

  delay(6000);

  Serial.println("SERVO PARADO");
}

// =====================================
// LOOP
// =====================================

void loop() {

  int botao1 = digitalRead(BOTAO1);
  int botao2 = digitalRead(BOTAO2);
  int botao3 = digitalRead(BOTAO3);

  int botao4 = digitalRead(BOTAO4);
  int botao5 = digitalRead(BOTAO5);

  int movimento = digitalRead(PIR);


  // ===================================
  // PIR
  // ===================================

  if (movimento == HIGH) {

    Serial.println("MOVIMENTO DETECTADO!");

    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);

    alarme();

  } else {

    noTone(BUZZER);


    // -------------------------------
    // BOTAO 1
    // -------------------------------

    if (botao1 == HIGH) {

      Serial.println("BOTAO 1 - ALARME LIGADO");

      alarme();
    }


    // -------------------------------
    // BOTAO 2
    // -------------------------------

    if (botao2 == HIGH) {

      digitalWrite(LED1, HIGH);

    } else {

      digitalWrite(LED1, LOW);
    }


    // -------------------------------
    // BOTAO 3
    // -------------------------------

    if (botao3 == HIGH) {

      digitalWrite(LED2, HIGH);

    } else {

      digitalWrite(LED2, LOW);
    }
  }


  // ===================================
  // BOTAO 4
  // ===================================

  static bool ultimoBotao4 = false;

  if (
    botao4 == HIGH &&
    ultimoBotao4 == false
  ) {

    servoHorario();
  }

  ultimoBotao4 = botao4;


  // ===================================
  // BOTAO 5
  // ===================================

  static bool ultimoBotao5 = false;

  if (
    botao5 == HIGH &&
    ultimoBotao5 == false
  ) {

    servoAntiHorario();
  }

  ultimoBotao5 = botao5;


  delay(10);
}