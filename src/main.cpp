
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// PROTOTIPOS DAS FUNCOES
void controlarAlarme(bool ativar);
void iniciarServo(int angulo);
void atualizarAlarme(bool movimento);

// =====================================
// PINOS DOS COMPONENTES
// =====================================

// BOTOES
#define BOTAO1 13
#define BOTAO2 21
#define BOTAO3 23
#define BOTAO4 18
#define BOTAO5 27

// LEDS
#define LED_AMARELO 22
#define LED_VERMELHO 19

// BUZZER
#define BUZZER 4

// SENSOR PIR
#define PIR 33

// SERVO
#define SERVO_PIN 32

#define TEMPO_SERVO 6000

// =====================================
// WIFI
// =====================================

const char *WIFI_SSID = "WIFI-IOT";
const char *WIFI_PASSWORD = "ac5ce1ss0@IOT";

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
String topicAlarme;
String topicComando;

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
bool led1Manual = false;
bool led2Manual = false;
bool buzzerLigado = false;

bool buzzerManual = false;
bool alarmeAtivo = false;

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
    topicAlarme = baseTopic + "/alarme";
    topicComando = baseTopic + "/comando";

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
}

void enviarEstado(String topico, bool ativado)
{
    publicar(
        topico,
        ativado ? "ATIVADO" : "DESATIVADO"
    );
}

// =====================================
// LED AMARELO
// =====================================

void controlarLED1(bool ligado)
{
    if (led1Ligado == ligado)
        return;

    led1Ligado = ligado;

    digitalWrite(
        LED_AMARELO,
        ligado ? HIGH : LOW
    );

    Serial.println(
        ligado ? "LED AMARELO LIGADO" : "LED AMARELO DESLIGADO"
    );

    publicar(
        topicLED1,
        ligado ? "LIGADO" : "DESLIGADO"
    );
}

// =====================================
// LED VERMELHO
// =====================================

void controlarLED2(bool ligado)
{
    if (led2Ligado == ligado)
        return;

    led2Ligado = ligado;

    digitalWrite(
        LED_VERMELHO,
        ligado ? HIGH : LOW
    );

    Serial.println(
        ligado ? "LED VERMELHO LIGADO" : "LED VERMELHO DESLIGADO"
    );

    publicar(
        topicLED2,
        ligado ? "LIGADO" : "DESLIGADO"
    );
}

// =====================================
// RECEBER COMANDOS DO SITE
// =====================================

void receberComando(char *topic, byte *payload, unsigned int length)
{
    String comando = "";

    for (unsigned int i = 0; i < length; i++)
    {
        comando += (char)payload[i];
    }

    comando.trim();

    Serial.println();
    Serial.println("COMANDO RECEBIDO DO SITE:");
    Serial.println(comando);

    if (comando == "LED1_ON")
    {
        led1Manual = true;
    }
    else if (comando == "LED1_OFF")
    {
        led1Manual = false;
    }
    else if (comando == "LED2_ON")
    {
        led2Manual = true;
    }
    else if (comando == "LED2_OFF")
    {
        led2Manual = false;
    }
    else if (comando == "ALARME_ON")
    {
        controlarAlarme(true);
    }
    else if (comando == "ALARME_OFF")
    {
        controlarAlarme(false);
    }
    else if (comando == "BUZZER_ON")
    {
        buzzerManual = true;
    }
    else if (comando == "BUZZER_OFF")
    {
        buzzerManual = false;
    }
    else if (comando == "SERVO_ABRIR")
    {
        iniciarServo(180);
    }
    else if (comando == "SERVO_FECHAR")
    {
        iniciarServo(0);
    }
    else
    {
        Serial.println("COMANDO DESCONHECIDO");
    }

    atualizarAlarme(digitalRead(PIR) == HIGH);
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
// ALARME
// =====================================

void controlarAlarme(bool ativar)
{
    if (alarmeAtivo == ativar)
        return;

    alarmeAtivo = ativar;

    Serial.println(
        alarmeAtivo ? "ALARME ATIVADO" : "ALARME DESATIVADO"
    );

    publicar(
        topicAlarme,
        alarmeAtivo ? "ATIVADO" : "DESATIVADO"
    );

    atualizarAlarme(digitalRead(PIR) == HIGH);
}

void atualizarAlarme(bool movimento)
{
    // O alarme armado só dispara quando o PIR detecta movimento.
    bool disparo = alarmeAtivo && movimento;

    // Os LEDs ligam quando o alarme dispara,
    // mas preservam os controles individuais.
    controlarLED1(led1Manual || disparo);
    controlarLED2(led2Manual || disparo);

    // O buzzer funciona com alarme, movimento ou comando manual.
    if (disparo || buzzerManual)
    {
        buzzerOn();
    }
    else
    {
        buzzerOff();
    }
}

// =====================================
// SERVO
// =====================================


void iniciarServo(int destino)
{
    if (destino != 0 && destino != 180)
        return;

    // Permite receber um novo comando mesmo durante
    // o movimento ou o tempo de espera anterior.
    posicaoAlvo = destino;
    servoMovendo = (posicaoAtual != posicaoAlvo);
    servoAguardando = false;

    ultimoMovimentoServo = millis();

    if (!servoMovendo)
    {
        publicar(
            topicServo,
            destino == 180 ? "PARADO_180" : "PARADO_0"
        );
        return;
    }

    publicar(
        topicServo,
        destino == 180 ? "MOVENDO_0_180" : "MOVENDO_180_0"
    );

    Serial.println(
        destino == 180 ? "ABRINDO PORTA" : "FECHANDO PORTA"
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

            Serial.println("MOVIMENTO DO SERVO FINALIZADO");
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

        Serial.println("SERVO PRONTO");
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

            mqtt.subscribe(topicComando.c_str());

            Serial.println("AGUARDANDO COMANDOS DO SITE...");

            publicar(topicStatus, "ESP32_ONLINE");

            enviarEstado(topicBotao1, digitalRead(BOTAO1) == LOW);
            enviarEstado(topicBotao2, digitalRead(BOTAO2) == LOW);
            enviarEstado(topicBotao3, digitalRead(BOTAO3) == LOW);
            enviarEstado(topicBotao4, digitalRead(BOTAO4) == LOW);
            enviarEstado(topicBotao5, digitalRead(BOTAO5) == LOW);

            publicar(
                topicPIR,
                digitalRead(PIR) == HIGH
                    ? "MOVIMENTO_DETECTADO"
                    : "SEM_MOVIMENTO"
            );

            publicar(topicLED1, led1Ligado ? "LIGADO" : "DESLIGADO");
            publicar(topicLED2, led2Ligado ? "LIGADO" : "DESLIGADO");
            publicar(topicBuzzer, buzzerLigado ? "LIGADO" : "DESLIGADO");

            publicar(
                topicAlarme,
                alarmeAtivo ? "ATIVADO" : "DESATIVADO"
            );
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
    Serial.println("### CASA INTELIGENTE ESP32 ###");

    // BOTOES

    // BOTOES

    pinMode(BOTAO1, INPUT_PULLUP);
    pinMode(BOTAO2, INPUT_PULLUP);
    pinMode(BOTAO3, INPUT_PULLUP);
    pinMode(BOTAO4, INPUT_PULLUP);
    pinMode(BOTAO5, INPUT_PULLUP);

    // LEDS

    pinMode(LED_AMARELO, OUTPUT);
    pinMode(LED_VERMELHO, OUTPUT);

    digitalWrite(LED_AMARELO, LOW);
    digitalWrite(LED_VERMELHO, LOW);

    // BUZZER

    pinMode(BUZZER, OUTPUT);
    noTone(BUZZER);

    // SENSOR PIR

    pinMode(PIR, INPUT);

    // SERVO

    meuServo.setPeriodHertz(50);

    meuServo.attach(SERVO_PIN, 500, 2400);

    meuServo.write(0);

    // MQTT

    configurarTopicos();

    conectarWiFi();

    mqtt.setServer(MQTT_SERVER, MQTT_PORT);

    mqtt.setCallback(receberComando);

    mqtt.setSocketTimeout(5);
    mqtt.setKeepAlive(30);

    conectarMQTT();

    // ESTADOS INICIAIS

    estadoAnterior1 = digitalRead(BOTAO1);
    estadoAnterior2 = digitalRead(BOTAO2);
    estadoAnterior3 = digitalRead(BOTAO3);
    estadoAnterior4 = digitalRead(BOTAO4);
    estadoAnterior5 = digitalRead(BOTAO5);

    movimentoAnterior = digitalRead(PIR);

    Serial.println();
    Serial.println("SISTEMA INICIADO");

    Serial.println("BOTAO 1 -> ALARME");
    Serial.println("BOTAO 2 -> LED AMARELO");
    Serial.println("BOTAO 3 -> LED VERMELHO");
    Serial.println("BOTAO 4 -> ABRIR PORTA");
    Serial.println("BOTAO 5 -> FECHAR PORTA");
    Serial.println("PIR -> DETECCAO DE MOVIMENTO");
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

    // LEITURA DOS BOTOES

    int botao1 = digitalRead(BOTAO1);
    int botao2 = digitalRead(BOTAO2);
    int botao3 = digitalRead(BOTAO3);
    int botao4 = digitalRead(BOTAO4);
    int botao5 = digitalRead(BOTAO5);

    // LEITURA DO PIR

    int movimento = digitalRead(PIR);

    // DETECTAR PRESSIONAMENTOS

    bool apertou1 = botao1 == LOW && estadoAnterior1 == HIGH;
    bool apertou2 = botao2 == LOW && estadoAnterior2 == HIGH;
    bool apertou3 = botao3 == LOW && estadoAnterior3 == HIGH;
    bool apertou4 = botao4 == LOW && estadoAnterior4 == HIGH;
    bool apertou5 = botao5 == LOW && estadoAnterior5 == HIGH;

    // BOTAO 1 - ALARME

    if (botao1 != estadoAnterior1)
    {
        enviarEstado(topicBotao1, botao1 == LOW);
        estadoAnterior1 = botao1;
    }

    if (apertou1)
    {
        controlarAlarme(!alarmeAtivo);
    }

    // BOTAO 2 - LED AMARELO

    if (botao2 != estadoAnterior2)
    {
        enviarEstado(topicBotao2, botao2 == LOW);
        estadoAnterior2 = botao2;
    }

    if (apertou2)
    {
        led1Manual = !led1Manual;
        atualizarAlarme(movimento == HIGH);
    }

    // BOTAO 3 - LED VERMELHO

    if (botao3 != estadoAnterior3)
    {
        enviarEstado(topicBotao3, botao3 == LOW);
        estadoAnterior3 = botao3;
    }

    if (apertou3)
    {
        led2Manual = !led2Manual;
        atualizarAlarme(movimento == HIGH);
    }

    // BOTAO 4 - ABRIR PORTA

    if (botao4 != estadoAnterior4)
    {
        enviarEstado(topicBotao4, botao4 == LOW);

        estadoAnterior4 = botao4;
    }

    if (apertou4)
    {
        iniciarServo(180);
    }

    // BOTAO 5 - FECHAR PORTA

    if (botao5 != estadoAnterior5)
    {
        enviarEstado(topicBotao5, botao5 == LOW);

        estadoAnterior5 = botao5;
    }

    if (apertou5)
    {
        iniciarServo(0);
    }

    // SENSOR PIR

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

    // ALARME E BUZZER

    atualizarAlarme(movimento == HIGH);

    // ATUALIZAR SERVO

    atualizarServo();

    delay(10);
}
