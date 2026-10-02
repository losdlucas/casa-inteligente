import os
import json
import threading
from datetime import datetime

import psycopg2
from psycopg2.extras import RealDictCursor
from dotenv import load_dotenv

import paho.mqtt.client as mqtt


# ============================================================
# CARREGAR .ENV
# ============================================================

load_dotenv()


# ============================================================
# CONFIGURAÇÕES DO POSTGRES / NEON
# ============================================================

DATABASE_URL = os.getenv("DATABASE_URL")

if not DATABASE_URL:
    raise RuntimeError(
        "DATABASE_URL não foi encontrada no arquivo .env"
    )


# ============================================================
# CONFIGURAÇÕES MQTT - HIVEMQ
# ============================================================

MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883

MQTT_BASE_TOPIC = "projeto_iot/esp32"


# ============================================================
# CONEXÃO COM O NEON
# ============================================================

def conectar_banco():
    """
    Cria uma conexão com o PostgreSQL do Neon.
    """

    print("[BANCO] Tentando conectar ao Neon...", flush=True)

    try:

        conexao = psycopg2.connect(
            DATABASE_URL,
            connect_timeout=10
        )

        print(
            "[BANCO] Conectado ao Neon!",
            flush=True
        )

        return conexao

    except Exception as erro:

        print(
            f"[BANCO] ERRO: {erro}",
            flush=True
        )

        raise


# ============================================================
# VERIFICAR BANCO
# ============================================================

def verificar_banco():

    try:

        conexao = conectar_banco()

        cursor = conexao.cursor()

        cursor.execute("""
            SELECT 1;
        """)

        resultado = cursor.fetchone()

        cursor.close()
        conexao.close()

        if resultado:
            print("[BANCO] PostgreSQL conectado com sucesso!")

            return True

    except Exception as erro:

        print(
            f"[BANCO] Não foi possível conectar: {erro}"
        )

        return False


# ============================================================
# CRIAR TABELA
# ============================================================

def criar_tabela():

    conexao = conectar_banco()

    cursor = conexao.cursor()

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS dispositivos (
            id SERIAL PRIMARY KEY,
            dispositivo VARCHAR(50) NOT NULL,
            valor TEXT,
            data_hora TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
        );
    """)

    cursor.execute("""
        CREATE INDEX IF NOT EXISTS idx_dispositivos_dispositivo
        ON dispositivos(dispositivo);
    """)

    cursor.execute("""
        CREATE INDEX IF NOT EXISTS idx_dispositivos_data_hora
        ON dispositivos(data_hora DESC);
    """)

    conexao.commit()

    cursor.close()
    conexao.close()

    print("[BANCO] Tabela dispositivos pronta!")


# ============================================================
# SALVAR DADO RECEBIDO PELO MQTT
# ============================================================

def salvar_dado(dispositivo, valor):

    try:

        conexao = conectar_banco()

        cursor = conexao.cursor()

        cursor.execute("""
            INSERT INTO dispositivos
            (dispositivo, valor, data_hora)
            VALUES (%s, %s, %s);
        """, (
            dispositivo,
            str(valor),
            datetime.now()
        ))

        conexao.commit()

        cursor.close()
        conexao.close()

        print(
            f"[BANCO] Salvo: "
            f"{dispositivo} = {valor}"
        )

    except Exception as erro:

        print(
            f"[BANCO] Erro ao salvar dado: {erro}"
        )


# ============================================================
# BUSCAR ÚLTIMO VALOR DE CADA DISPOSITIVO
# ============================================================

def buscar_ultimos_dados():

    conexao = conectar_banco()

    cursor = conexao.cursor(
        cursor_factory=RealDictCursor
    )

    cursor.execute("""
        SELECT DISTINCT ON (dispositivo)
            id,
            dispositivo,
            valor,
            data_hora
        FROM dispositivos
        ORDER BY dispositivo, data_hora DESC;
    """)

    resultados = cursor.fetchall()

    cursor.close()
    conexao.close()

    dados = {}

    for registro in resultados:

        dados[registro["dispositivo"]] = {
            "valor": registro["valor"],
            "data_hora": registro["data_hora"].isoformat()
        }

    return dados


# ============================================================
# BUSCAR HISTÓRICO
# ============================================================

def buscar_historico(limite=50):

    conexao = conectar_banco()

    cursor = conexao.cursor(
        cursor_factory=RealDictCursor
    )

    cursor.execute("""
        SELECT
            id,
            dispositivo,
            valor,
            data_hora
        FROM dispositivos
        ORDER BY data_hora DESC
        LIMIT %s;
    """, (
        limite,
    ))

    resultados = cursor.fetchall()

    cursor.close()
    conexao.close()

    return resultados


# ============================================================
# INTERPRETAR MQTT
# ============================================================

def interpretar_mensagem(topic, payload):

    prefixo = MQTT_BASE_TOPIC + "/"

    if topic.startswith(prefixo):

        dispositivo = topic[len(prefixo):]

    else:

        dispositivo = topic

    try:

        dados = json.loads(payload)

        if isinstance(dados, dict):

            valor = json.dumps(
                dados,
                ensure_ascii=False
            )

        else:

            valor = dados

    except (json.JSONDecodeError, TypeError):

        valor = payload

    return dispositivo, valor


# ============================================================
# QUANDO CONECTAR AO HIVEMQ
# ============================================================

def on_connect(
    client,
    userdata,
    flags,
    reason_code,
    properties=None
):

    if reason_code == 0:

        print("[MQTT] Conectado ao HiveMQ!")

        client.subscribe(
            f"{MQTT_BASE_TOPIC}/#"
        )

        print(
            f"[MQTT] Escutando: "
            f"{MQTT_BASE_TOPIC}/#"
        )

    else:

        print(
            f"[MQTT] Erro ao conectar: "
            f"{reason_code}"
        )


# ============================================================
# QUANDO RECEBER UMA MENSAGEM
# ============================================================

def on_message(client, userdata, message):

    try:

        topic = message.topic

        payload = message.payload.decode(
            "utf-8"
        )

        dispositivo, valor = interpretar_mensagem(
            topic,
            payload
        )

        print()
        print("================================")
        print("       NOVA MENSAGEM MQTT")
        print("================================")
        print(f"Tópico:      {topic}")
        print(f"Dispositivo: {dispositivo}")
        print(f"Valor:       {valor}")
        print("================================")
        print()

        salvar_dado(
            dispositivo,
            valor
        )

    except Exception as erro:

        print(
            f"[MQTT] Erro ao processar mensagem: "
            f"{erro}"
        )


# ============================================================
# INICIAR MQTT
# ============================================================

def iniciar_mqtt():

    client = mqtt.Client(
        mqtt.CallbackAPIVersion.VERSION2
    )

    client.on_connect = on_connect
    client.on_message = on_message

    print("[MQTT] Conectando ao HiveMQ...")

    client.connect(
        MQTT_BROKER,
        MQTT_PORT,
        60
    )

    thread = threading.Thread(
        target=client.loop_forever,
        daemon=True
    )

    thread.start()

    print("[MQTT] Cliente MQTT iniciado!")

    return client


# ============================================================
# EXECUTAR DB.PY DIRETAMENTE
# ============================================================

if __name__ == "__main__":

    print()
    print("========================================")
    print("      CASA INTELIGENTE - DB + MQTT")
    print("========================================")
    print()

    # Testa conexão com Neon
    if not verificar_banco():

        print(
            "[ERRO] Não foi possível conectar "
            "ao banco."
        )

        exit(1)

    # Cria tabela
    criar_tabela()

    # Inicia MQTT
    iniciar_mqtt()

    print()
    print("Sistema iniciado.")
    print("Aguardando dados do ESP32...")
    print()

    try:

        while True:
            pass

    except KeyboardInterrupt:

        print()
        print("Sistema encerrado.")