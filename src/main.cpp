#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
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

// =====================================
// TEMPO DO SERVO
// =====================================

#define TEMPO_SERVO 6000

// =====================================
// WI-FI
// =====================================

// Wokwi
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// =====================================
// MQTT
// =====================================

const char* MQTT_SERVER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

WiFiClient espClient;
PubSubClient mqtt(espClient);

// =====================================
// TÓPICOS MQTT
// =====================================

String baseTopic;

String topicStatus;
String topicBotao1;
String topicBotao2;
String topicBotao3;
String topicBotao4;
String topicBotao5;
String topicPIR;
String topicServo;
String topicComando;

// =====================================
// COMANDOS MQTT
// =====================================

int comandoServo = 0;

// 0 = nenhum
// 1 = 0 -> 180
// 2 = 180 -> 0

bool comandoBuzzer = false;


// =====================================
// CONFIGURAÇÃO DOS TÓPICOS
// =====================================

void configurarTopicos() {

  // Usa o ID do ESP32 para deixar o tópico único

  String idESP = String((uint32_t)ESP.getEfuseMac(), HEX);

  baseTopic = "projeto_iot/esp32/" + idESP;

  topicStatus = baseTopic + "/status";

  topicBotao1 = baseTopic + "/botao1";
  topicBotao2 = baseTopic + "/botao2";
  topicBotao3 = baseTopic + "/botao3";
  topicBotao4 = baseTopic + "/botao4";
  topicBotao5 = baseTopic + "/botao5";

  topicPIR = baseTopic + "/pir";

  topicServo = baseTopic + "/servo";

  topicComando = baseTopic + "/comando";

  Serial.println();
  Serial.println("TOPICOS MQTT:");
  Serial.println(baseTopic);
}


// =====================================
// CONECTAR WI-FI
// =====================================

void conectarWiFi() {

  Serial.println();
  Serial.println("================================");
  Serial.println("CONECTANDO AO WI-FI");
  Serial.println("================================");

  Serial.print("Rede: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println("WI-FI CONECTADO!");

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}


// =====================================
// CALLBACK MQTT
// =====================================

void mqttCallback(
  char* topic,
  byte* payload,
  unsigned int length
) {

  String mensagem = "";

  for (unsigned int i = 0; i < length; i++) {

    mensagem += (char)payload[i];
  }

  Serial.println();
  Serial.println("================================");
  Serial.println("MENSAGEM MQTT RECEBIDA");
  Serial.println("================================");

  Serial.print("Topico: ");
  Serial.println(topic);

  Serial.print("Mensagem: ");
  Serial.println(mensagem);


  // ===================================
  // COMANDOS DO SERVO
  // ===================================

  if (mensagem == "SERVO_ABRIR") {

    comandoServo = 1;

    Serial.println("MQTT -> SERVO 0 -> 180");
  }


  if (mensagem == "SERVO_FECHAR") {

    comandoServo = 2;

    Serial.println("MQTT -> SERVO 180 -> 0");
  }


  // ===================================
  // LED 1
  // ===================================

  if (mensagem == "LED1_ON") {

    digitalWrite(LED1, HIGH);

    mqtt.publish(
      topicStatus.c_str(),
      "LED1_LIGADO"
    );
  }


  if (mensagem == "LED1_OFF") {

    digitalWrite(LED1, LOW);

    mqtt.publish(
      topicStatus.c_str(),
      "LED1_DESLIGADO"
    );
  }


  // ===================================
  // LED 2
  // ===================================

  if (mensagem == "LED2_ON") {

    digitalWrite(LED2, HIGH);

    mqtt.publish(
      topicStatus.c_str(),
      "LED2_LIGADO"
    );
  }


  if (mensagem == "LED2_OFF") {

    digitalWrite(LED2, LOW);

    mqtt.publish(
      topicStatus.c_str(),
      "LED2_DESLIGADO"
    );
  }


  // ===================================
  // BUZZER
  // ===================================

  if (mensagem == "BUZZER_ON") {

    comandoBuzzer = true;

    tone(BUZZER, 500);

    mqtt.publish(
      topicStatus.c_str(),
      "BUZZER_LIGADO"
    );
  }


  if (mensagem == "BUZZER_OFF") {

    comandoBuzzer = false;

    noTone(BUZZER);

    mqtt.publish(
      topicStatus.c_str(),
      "BUZZER_DESLIGADO"
    );
  }
}


// =====================================
// CONECTAR MQTT
// =====================================

void conectarMQTT() {

  while (!mqtt.connected()) {

    Serial.println();
    Serial.println("CONECTANDO AO MQTT...");

    // ID único do cliente

    String clientID = "ESP32-";

    clientID += String(
      (uint32_t)ESP.getEfuseMac(),
      HEX
    );

    if (mqtt.connect(clientID.c_str())) {

      Serial.println("MQTT CONECTADO!");

      // Receber comandos

      mqtt.subscribe(
        topicComando.c_str()
      );

      mqtt.publish(
        topicStatus.c_str(),
        "ESP32_ONLINE"
      );

    } else {

      Serial.print("ERRO MQTT. Codigo: ");

      Serial.println(
        mqtt.state()
      );

      Serial.println(
        "Tentando novamente em 3 segundos..."
      );

      delay(3000);
    }
  }
}


// =====================================
// ALARME
// =====================================

void alarme() {

  for (int i = 0; i < 4; i++) {

    tone(BUZZER, 500);

    mqtt.loop();

    delay(120);

    noTone(BUZZER);

    mqtt.loop();

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
// SERVO 0 -> 180
// =====================================

void servoHorario() {

  Serial.println();
  Serial.println("BOTAO 4");
  Serial.println("SERVO 0 -> 180 GRAUS");

  mqtt.publish(
    topicBotao4.c_str(),
    "PRESSIONADO"
  );

  mqtt.publish(
    topicServo.c_str(),
    "MOVENDO_0_180"
  );


  // Garante que começa em 0

  meuServo.write(0);

  delay(300);


  // Vai de 0 até 180

  for (int pos = 0; pos <= 180; pos += 2) {

    meuServo.write(pos);

    mqtt.loop();

    delay(20);
  }


  Serial.println("SERVO EM 180 GRAUS");

  mqtt.publish(
    topicServo.c_str(),
    "180_GRAUS"
  );


  // Fica 6 segundos em 180

  unsigned long inicio = millis();

  while (millis() - inicio < TEMPO_SERVO) {

    mqtt.loop();

    delay(20);
  }


  Serial.println("SERVO PARADO");

  mqtt.publish(
    topicServo.c_str(),
    "PARADO_180"
  );
}


// =====================================
// SERVO 180 -> 0
// =====================================

void servoAntiHorario() {

  Serial.println();
  Serial.println("BOTAO 5");
  Serial.println("SERVO 180 -> 0 GRAUS");

  mqtt.publish(
    topicBotao5.c_str(),
    "PRESSIONADO"
  );

  mqtt.publish(
    topicServo.c_str(),
    "MOVENDO_180_0"
  );


  // Garante que começa em 180

  meuServo.write(180);

  delay(300);


  // Vai de 180 até 0

  for (int pos = 180; pos >= 0; pos -= 2) {

    meuServo.write(pos);

    mqtt.loop();

    delay(20);
  }


  Serial.println("SERVO EM 0 GRAUS");

  mqtt.publish(
    topicServo.c_str(),
    "0_GRAUS"
  );


  // Fica 6 segundos em 0

  unsigned long inicio = millis();

  while (millis() - inicio < TEMPO_SERVO) {

    mqtt.loop();

    delay(20);
  }


  Serial.println("SERVO PARADO");

  mqtt.publish(
    topicServo.c_str(),
    "PARADO_0"
  );
}


// =====================================
// SETUP
// =====================================

void setup() {

  Serial.begin(115200);


  // ===================================
  // BOTÕES
  // ===================================

  pinMode(
    BOTAO1,
    INPUT_PULLDOWN
  );

  pinMode(
    BOTAO2,
    INPUT_PULLDOWN
  );

  pinMode(
    BOTAO3,
    INPUT_PULLDOWN
  );

  pinMode(
    BOTAO4,
    INPUT_PULLDOWN
  );

  pinMode(
    BOTAO5,
    INPUT_PULLDOWN
  );


  // ===================================
  // LEDS
  // ===================================

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);


  // ===================================
  // BUZZER
  // ===================================

  pinMode(BUZZER, OUTPUT);

  noTone(BUZZER);


  // ===================================
  // PIR
  // ===================================

  pinMode(PIR, INPUT);


  // ===================================
  // SERVO
  // ===================================

  meuServo.setPeriodHertz(50);

  meuServo.attach(
    SERVO_PIN,
    500,
    2400
  );

  // Começa em 0 graus

  meuServo.write(0);


  // ===================================
  // TÓPICOS
  // ===================================

  configurarTopicos();


  // ===================================
  // WI-FI
  // ===================================

  conectarWiFi();


  // ===================================
  // MQTT
  // ===================================

  mqtt.setServer(
    MQTT_SERVER,
    MQTT_PORT
  );

  mqtt.setCallback(
    mqttCallback
  );


  conectarMQTT();


  // ===================================
  // MENSAGENS
  // ===================================

  Serial.println();
  Serial.println("================================");
  Serial.println("SISTEMA INICIADO");
  Serial.println("================================");

  Serial.println("BOTAO 1 -> ALARME");
  Serial.println("BOTAO 2 -> LED 1");
  Serial.println("BOTAO 3 -> LED 2");
  Serial.println("BOTAO 4 -> SERVO 0 -> 180");
  Serial.println("BOTAO 5 -> SERVO 180 -> 0");
  Serial.println("PIR -> MOVIMENTO");

  Serial.println();
  Serial.println("MQTT PRONTO!");
}


// =====================================
// LOOP
// =====================================

void loop() {

  // ===================================
  // GARANTIR MQTT
  // ===================================

  if (!mqtt.connected()) {

    conectarMQTT();
  }

  mqtt.loop();


  // ===================================
  // LER BOTÕES
  // ===================================

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

    static bool ultimoMovimento = false;

    if (!ultimoMovimento) {

      Serial.println();
      Serial.println("MOVIMENTO DETECTADO!");

      mqtt.publish(
        topicPIR.c_str(),
        "MOVIMENTO_DETECTADO"
      );
    }

    ultimoMovimento = true;


    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);

    alarme();

  } else {

    static bool ultimoMovimento = false;

    if (ultimoMovimento) {

      mqtt.publish(
        topicPIR.c_str(),
        "SEM_MOVIMENTO"
      );

      ultimoMovimento = false;
    }

    noTone(BUZZER);


    // =================================
    // BOTAO 1
    // =================================

    if (botao1 == HIGH) {

      Serial.println(
        "BOTAO 1 - ALARME LIGADO"
      );

      mqtt.publish(
        topicBotao1.c_str(),
        "PRESSIONADO"
      );

      alarme();
    }


    // =================================
    // BOTAO 2
    // =================================

    if (botao2 == HIGH) {

      digitalWrite(LED1, HIGH);

      mqtt.publish(
        topicBotao2.c_str(),
        "LED1_LIGADO"
      );

    } else {

      digitalWrite(LED1, LOW);
    }


    // =================================
    // BOTAO 3
    // =================================

    if (botao3 == HIGH) {

      digitalWrite(LED2, HIGH);

      mqtt.publish(
        topicBotao3.c_str(),
        "LED2_LIGADO"
      );

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


  // ===================================
  // COMANDO MQTT DO SERVO
  // ===================================

  if (comandoServo == 1) {

    comandoServo = 0;

    servoHorario();
  }


  if (comandoServo == 2) {

    comandoServo = 0;

    servoAntiHorario();
  }


  delay(10);
}