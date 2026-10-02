
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// =====================================
// PINOS
// =====================================

// RELES
#define RELE1 18
#define RELE2 19
#define RELE3 25
#define RELE4 26

// BOTOES
#define BOTAO1 13
#define BOTAO2 21
#define BOTAO3 23
#define BOTAO4 16
#define BOTAO5 27

// SAIDAS
#define BUZZER 18
#define LED1 22
#define LED2 25

// SENSOR PIR
#define PIR 33

// SERVO
#define SERVO_PIN 32

#define TEMPO_SERVO 6000

// =====================================
// WIFI
// =====================================

const char *WIFI_SSID = "WIFI-IOT";
const char *WIFI_PASSWORD = "Ac5ce1ss0@IOT";

// =====================================
// MQTT
// =====================================

const char *MQTT_SERVER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

WiFiClient espClient;
PubSubClient mqtt(espClient);

String baseTopic = "projeto_iot/esp32";

String topicStatus;
String topicBotao1;
String topicBotao2;
String topicBotao3;
String topicBotao4;
String topicBotao5;
String topicPIR;
String topicServo;
String topicLED1;
String topicLED2;
String topicBuzzer;
String topicRele1;
String topicRele2;
String topicRele3;
String topicRele4;

// =====================================
// SERVO
// =====================================

Servo meuServo;

int posicaoAtual = 0;
int posicaoAlvo = 0;

bool servoMovendo = false;
bool servoAguardando = false;

unsigned long ultimoMovimentoServo = 0;
unsigned long inicioEsperaServo = 0;

const unsigned long intervaloServo = 20;

// =====================================
// ESTADOS
// =====================================

bool led1Ligado = false;
bool led2Ligado = false;
bool buzzerLigado = false;

bool releEstado[4] = {false, false, false, false};

bool alarmeAtivo = false;

// =====================================
// ESTADOS ANTERIORES
// =====================================

int estadoAnterior1 = LOW;
int estadoAnterior2 = LOW;
int estadoAnterior3 = LOW;
int estadoAnterior4 = LOW;
int estadoAnterior5 = LOW;

int movimentoAnterior = LOW;

// =====================================
// CONFIGURAR TOPICOS
// =====================================

void configurarTopicos()
{
    topicStatus = baseTopic + "/status";

    topicBotao1 = baseTopic + "/botao1";
    topicBotao2 = baseTopic + "/botao2";
    topicBotao3 = baseTopic + "/botao3";
    topicBotao4 = baseTopic + "/botao4";
    topicBotao5 = baseTopic + "/botao5";

    topicPIR = baseTopic + "/pir";
    topicServo = baseTopic + "/servo";

    topicLED1 = baseTopic + "/led1";
    topicLED2 = baseTopic + "/led2";
    topicBuzzer = baseTopic + "/buzzer";

    topicRele1 = baseTopic + "/rele1";
    topicRele2 = baseTopic + "/rele2";
    topicRele3 = baseTopic + "/rele3";
    topicRele4 = baseTopic + "/rele4";

    Serial.println("TOPICOS CONFIGURADOS");
    Serial.println(baseTopic);
}

// =====================================
// PUBLICAR MQTT
// =====================================

void publicar(String topico, const char *mensagem)
{
    if (mqtt.connected())
    {
        bool enviado = mqtt.publish(
            topico.c_str(),
            mensagem,
            true
        );

        Serial.print("MQTT: ");
        Serial.print(topico);
        Serial.print(" -> ");
        Serial.print(mensagem);
        Serial.print(" | ");
        Serial.println(enviado ? "ENVIADO" : "FALHA");
    }
    else
    {
        Serial.println("MQTT DESCONECTADO");
    }
}

// =====================================
// ESTADO DOS BOTOES
// =====================================

void enviarEstado(String topico, bool ativado)
{
    publicar(
        topico,
        ativado ? "ATIVADO" : "DESATIVADO"
    );
}

// =====================================
// LEDS
// =====================================

void controlarLED1(bool ligado)
{
    if (led1Ligado == ligado)
        return;

    led1Ligado = ligado;

    digitalWrite(LED1, ligado ? HIGH : LOW);

    Serial.println(ligado ? "LED 1 LIGADO" : "LED 1 DESLIGADO");

    publicar(
        topicLED1,
        ligado ? "LIGADO" : "DESLIGADO"
    );
}

void controlarLED2(bool ligado)
{
    if (led2Ligado == ligado)
        return;

    led2Ligado = ligado;

    digitalWrite(LED2, ligado ? HIGH : LOW);

    Serial.println(ligado ? "LED 2 LIGADO" : "LED 2 DESLIGADO");

    publicar(
        topicLED2,
        ligado ? "LIGADO" : "DESLIGADO"
    );
}

// =====================================
// BUZZER
// =====================================

void buzzerOn()
{
    if (buzzerLigado)
        return;

    buzzerLigado = true;

    tone(BUZZER, 1000);

    Serial.println("BUZZER LIGADO");

    publicar(topicBuzzer, "LIGADO");
}

void buzzerOff()
{
    if (!buzzerLigado)
        return;

    buzzerLigado = false;

    noTone(BUZZER);

    Serial.println("BUZZER DESLIGADO");

    publicar(topicBuzzer, "DESLIGADO");
}

// =====================================
// RELES
// =====================================

void controlarRele(int numero, bool ligado)
{
    int pinos[] = {RELE1, RELE2, RELE3, RELE4};

    String topicos[] = {
        topicRele1,
        topicRele2,
        topicRele3,
        topicRele4
    };

    if (numero < 1 || numero > 4)
        return;

    int indice = numero - 1;

    if (releEstado[indice] == ligado)
        return;

    releEstado[indice] = ligado;

    digitalWrite(
        pinos[indice],
        ligado ? HIGH : LOW
    );

    publicar(
        topicos[indice],
        ligado ? "LIGADO" : "DESLIGADO"
    );
}

// =====================================
// SERVO
// =====================================

void iniciarServo(int destino)
{
    if (destino != 0 && destino != 180)
        return;

    if (servoMovendo || servoAguardando)
        return;

    posicaoAlvo = destino;
    servoMovendo = true;
    ultimoMovimentoServo = millis();

    publicar(
        topicServo,
        destino == 180 ? "MOVENDO_0_180" : "MOVENDO_180_0"
    );
}

void atualizarServo()
{
    unsigned long agora = millis();

    if (servoMovendo)
    {
        if (agora - ultimoMovimentoServo < intervaloServo)
            return;

        ultimoMovimentoServo = agora;

        if (posicaoAtual < posicaoAlvo)
        {
            posicaoAtual += 2;

            if (posicaoAtual > posicaoAlvo)
                posicaoAtual = posicaoAlvo;
        }
        else if (posicaoAtual > posicaoAlvo)
        {
            posicaoAtual -= 2;

            if (posicaoAtual < posicaoAlvo)
                posicaoAtual = posicaoAlvo;
        }

        meuServo.write(posicaoAtual);

        if (posicaoAtual == posicaoAlvo)
        {
            servoMovendo = false;
            servoAguardando = true;
            inicioEsperaServo = agora;

            publicar(
                topicServo,
                posicaoAlvo == 180 ? "180_GRAUS" : "0_GRAUS"
            );
        }
    }

    if (servoAguardando &&
        agora - inicioEsperaServo >= TEMPO_SERVO)
    {
        servoAguardando = false;

        publicar(
            topicServo,
            posicaoAlvo == 180 ? "PARADO_180" : "PARADO_0"
        );
    }
}

// =====================================
// ALARME - SOMENTE SENSOR PIR
// =====================================

void atualizarAlarme(bool ativar)
{
    alarmeAtivo = ativar;

    if (ativar)
    {
        buzzerOn();
    }
    else
    {
        buzzerOff();
    }
}

// =====================================
// WIFI
// =====================================

void conectarWiFi()
{
    Serial.println("CONECTANDO AO WIFI...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WIFI CONECTADO");

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

// =====================================
// MQTT
// =====================================

void conectarMQTT()
{
    while (!mqtt.connected())
    {
        Serial.println("CONECTANDO AO HIVEMQ...");

        String clientID = "ESP32-";
        clientID += String(
            (uint32_t)ESP.getEfuseMac(),
            HEX
        );

        if (mqtt.connect(clientID.c_str()))
        {
            Serial.println("MQTT CONECTADO!");

            publicar(topicStatus, "ESP32_ONLINE");

            enviarEstado(topicBotao1, digitalRead(BOTAO1) == HIGH);
            enviarEstado(topicBotao2, digitalRead(BOTAO2) == HIGH);
            enviarEstado(topicBotao3, digitalRead(BOTAO3) == HIGH);
            enviarEstado(topicBotao4, digitalRead(BOTAO4) == HIGH);
            enviarEstado(topicBotao5, digitalRead(BOTAO5) == HIGH);

            publicar(
                topicPIR,
                digitalRead(PIR) == HIGH
                    ? "MOVIMENTO_DETECTADO"
                    : "SEM_MOVIMENTO"
            );

            publicar(topicLED1, led1Ligado ? "LIGADO" : "DESLIGADO");
            publicar(topicLED2, led2Ligado ? "LIGADO" : "DESLIGADO");
            publicar(topicBuzzer, buzzerLigado ? "LIGADO" : "DESLIGADO");

            publicar(topicRele1, releEstado[0] ? "LIGADO" : "DESLIGADO");
            publicar(topicRele2, releEstado[1] ? "LIGADO" : "DESLIGADO");
            publicar(topicRele3, releEstado[2] ? "LIGADO" : "DESLIGADO");
            publicar(topicRele4, releEstado[3] ? "LIGADO" : "DESLIGADO");
        }
        else
        {
            Serial.print("ERRO MQTT: ");
            Serial.println(mqtt.state());

            delay(3000);
        }
    }
}

// =====================================
// SETUP
// =====================================

void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("### ESP32 TRANSMISSOR MQTT ###");

    pinMode(BOTAO1, INPUT_PULLDOWN);
    pinMode(BOTAO2, INPUT_PULLDOWN);
    pinMode(BOTAO3, INPUT_PULLDOWN);
    pinMode(BOTAO4, INPUT_PULLDOWN);
    pinMode(BOTAO5, INPUT_PULLDOWN);

    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);

    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);

    pinMode(RELE1, OUTPUT);
    pinMode(RELE2, OUTPUT);
    pinMode(RELE3, OUTPUT);
    pinMode(RELE4, OUTPUT);

    digitalWrite(RELE1, LOW);
    digitalWrite(RELE2, LOW);
    digitalWrite(RELE3, LOW);
    digitalWrite(RELE4, LOW);

    pinMode(BUZZER, OUTPUT);
    noTone(BUZZER);

    pinMode(PIR, INPUT);

    meuServo.setPeriodHertz(50);
    meuServo.attach(SERVO_PIN, 500, 2400);
    meuServo.write(0);

    configurarTopicos();

    conectarWiFi();

    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt.setSocketTimeout(5);
    mqtt.setKeepAlive(30);

    conectarMQTT();

    estadoAnterior1 = digitalRead(BOTAO1);
    estadoAnterior2 = digitalRead(BOTAO2);
    estadoAnterior3 = digitalRead(BOTAO3);
    estadoAnterior4 = digitalRead(BOTAO4);
    estadoAnterior5 = digitalRead(BOTAO5);

    movimentoAnterior = digitalRead(PIR);

    Serial.println("SISTEMA INICIADO");
    Serial.println("BOTAO 2 -> LED 1");
    Serial.println("BOTAO 3 -> LED 2");
    Serial.println("PIR -> BUZZER");
}

// =====================================
// LOOP
// =====================================

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        conectarWiFi();
    }

    if (!mqtt.connected())
    {
        conectarMQTT();
    }

    mqtt.loop();

    // =====================================
    // LEITURA DOS BOTOES
    // =====================================

    int botao1 = digitalRead(BOTAO1);
    int botao2 = digitalRead(BOTAO2);
    int botao3 = digitalRead(BOTAO3);
    int botao4 = digitalRead(BOTAO4);
    int botao5 = digitalRead(BOTAO5);

    // =====================================
    // LEITURA DO PIR
    // =====================================

    int movimento = digitalRead(PIR);

    // =====================================
    // BOTAO 1
    // =====================================

    if (botao1 != estadoAnterior1)
    {
        enviarEstado(topicBotao1, botao1 == HIGH);
        estadoAnterior1 = botao1;
    }

    // =====================================
    // BOTAO 2 - LED 1
    // =====================================

    if (botao2 != estadoAnterior2)
    {
        enviarEstado(topicBotao2, botao2 == HIGH);
        estadoAnterior2 = botao2;
    }

    controlarLED1(botao2 == HIGH);

    // =====================================
    // BOTAO 3 - LED 2
    // =====================================

    if (botao3 != estadoAnterior3)
    {
        enviarEstado(topicBotao3, botao3 == HIGH);
        estadoAnterior3 = botao3;
    }

    controlarLED2(botao3 == HIGH);

    // =====================================
    // BOTAO 4
    // =====================================

    if (botao4 != estadoAnterior4)
    {
        enviarEstado(topicBotao4, botao4 == HIGH);
        estadoAnterior4 = botao4;
    }

    // =====================================
    // BOTAO 5
    // =====================================

    if (botao5 != estadoAnterior5)
    {
        enviarEstado(topicBotao5, botao5 == HIGH);
        estadoAnterior5 = botao5;
    }

    // =====================================
    // SENSOR PIR - MQTT
    // =====================================

    if (movimento != movimentoAnterior)
    {
        publicar(
            topicPIR,
            movimento == HIGH
                ? "MOVIMENTO_DETECTADO"
                : "SEM_MOVIMENTO"
        );

        movimentoAnterior = movimento;
    }

    // =====================================
    // BUZZER - SOMENTE O PIR
    // =====================================

    atualizarAlarme(movimento == HIGH);

    // =====================================
    // SERVO
    // =====================================

    static int ultimoAcionamento4 = LOW;
    static int ultimoAcionamento5 = LOW;

    if (botao4 == HIGH && ultimoAcionamento4 == LOW)
    {
        iniciarServo(180);
    }

    if (botao5 == HIGH && ultimoAcionamento5 == LOW)
    {
        iniciarServo(0);
    }

    ultimoAcionamento4 = botao4;
    ultimoAcionamento5 = botao5;

    atualizarServo();

    delay(10);
}