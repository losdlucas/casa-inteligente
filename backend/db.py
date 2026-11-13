
import os
import json
import time
import threading
from datetime import datetime

import psycopg2
from psycopg2.extras import RealDictCursor
from dotenv import load_dotenv
import paho.mqtt.client as mqtt


# ============================================================
# CONFIGURACAO
# ============================================================

load_dotenv()

DATABASE_URL = os.getenv("DATABASE_URL")

MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_BASE_TOPIC = "projeto_iot/esp32"

_ultimo_valor = {}
_cache_carregado = False
_cache_lock = threading.Lock()


# ============================================================
# CONEXAO COM NEON
# ============================================================

def conectar_banco():
    if not DATABASE_URL:
        raise RuntimeError("DATABASE_URL nao encontrada no .env")

    return psycopg2.connect(
        DATABASE_URL,
        sslmode="require",
        connect_timeout=20,
        keepalives=1,
        keepalives_idle=30,
        keepalives_interval=10,
        keepalives_count=3
    )


def executar(callback):
    ultimo_erro = None

    for tentativa in range(3):
        conexao = None

        try:
            conexao = conectar_banco()

            with conexao.cursor(cursor_factory=RealDictCursor) as cursor:
                resultado = callback(cursor)

            conexao.commit()
            return resultado

        except (psycopg2.OperationalError, psycopg2.InterfaceError) as erro:
            ultimo_erro = erro

            if conexao:
                try:
                    conexao.rollback()
                except Exception:
                    pass

            print(
                f"[BANCO] Falha na conexao. Tentativa {tentativa + 1}/3: {erro}",
                flush=True
            )

            if tentativa < 2:
                time.sleep(2)

        except Exception:
            if conexao:
                try:
                    conexao.rollback()
                except Exception:
                    pass
            raise

        finally:
            if conexao:
                try:
                    conexao.close()
                except Exception:
                    pass

    raise RuntimeError(f"Neon indisponivel: {ultimo_erro}")


def verificar_banco():
    try:
        executar(lambda cursor: cursor.execute("SELECT 1"))
        print("[BANCO] Neon conectado!", flush=True)
        return True

    except Exception as erro:
        print(f"[BANCO] Sem conexao: {erro}", flush=True)
        return False


# ============================================================
# CRIAR TABELA
# ============================================================

def criar_tabela():

    def criar(cursor):
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

    executar(criar)
    print("[BANCO] Tabela dispositivos pronta!", flush=True)


# ============================================================
# SALVAR DADOS
# ============================================================

def salvar_dado(dispositivo, valor):

    def inserir(cursor):
        cursor.execute("""
            INSERT INTO dispositivos (dispositivo, valor, data_hora)
            VALUES (%s, %s, %s);
        """, (dispositivo, str(valor), datetime.now()))

    try:
        executar(inserir)
        print(f"[BANCO] Salvo: {dispositivo} = {valor}", flush=True)
        return True

    except Exception as erro:
        print(f"[BANCO] Erro ao salvar: {erro}", flush=True)
        return False


# ============================================================
# ULTIMOS DADOS
# ============================================================

def buscar_ultimos_dados():

    def consultar(cursor):
        cursor.execute("""
            SELECT DISTINCT ON (dispositivo)
                id,
                dispositivo,
                valor,
                data_hora
            FROM dispositivos
            ORDER BY dispositivo, data_hora DESC, id DESC;
        """)

        return cursor.fetchall()

    resultados = executar(consultar)

    dados = {}

    for registro in resultados:
        dados[registro["dispositivo"]] = {
            "valor": registro["valor"],
            "data_hora": registro["data_hora"].isoformat()
        }

    return dados


# ============================================================
# HISTORICO
# ============================================================

def buscar_historico(limite=50):

    def consultar(cursor):
        cursor.execute("""
            SELECT
                id,
                dispositivo,
                valor,
                data_hora
            FROM dispositivos
            WHERE dispositivo <> 'comando'
              AND dispositivo NOT LIKE 'botao%%'
            ORDER BY data_hora DESC, id DESC
            LIMIT %s;
        """, (limite,))

        return cursor.fetchall()

    return executar(consultar)


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
            valor = json.dumps(dados, ensure_ascii=False)
        else:
            valor = dados

    except (json.JSONDecodeError, TypeError):
        valor = payload

    return dispositivo, valor


# ============================================================
# CACHE
# ============================================================

def carregar_cache():

    global _cache_carregado

    with _cache_lock:
        if _cache_carregado:
            return

        try:
            dados = buscar_ultimos_dados()

            for nome, info in dados.items():
                _ultimo_valor[nome] = str(info["valor"])

            print("[MQTT] Cache carregado!", flush=True)

        except Exception as erro:
            print(f"[MQTT] Cache indisponivel: {erro}", flush=True)

        finally:
            _cache_carregado = True


# ============================================================
# CONEXAO MQTT
# ============================================================

def on_connect(client, userdata, flags, reason_code, properties=None):

    if not reason_code.is_failure:
        print("[MQTT] Conectado ao HiveMQ!", flush=True)

        client.subscribe(f"{MQTT_BASE_TOPIC}/#")

        print(
            f"[MQTT] Escutando {MQTT_BASE_TOPIC}/#",
            flush=True
        )

    else:
        print(f"[MQTT] Falha na conexao: {reason_code}", flush=True)


def on_disconnect(client, userdata, disconnect_flags, reason_code, properties=None):
    print(f"[MQTT] Desconectado: {reason_code}", flush=True)


# ============================================================
# RECEBER MENSAGENS
# ============================================================

def on_message(client, userdata, message):

    try:
        topic = message.topic
        payload = message.payload.decode("utf-8")

        dispositivo, valor = interpretar_mensagem(topic, payload)

        if dispositivo == "comando":
            return

        if dispositivo.startswith("botao"):
            return

        carregar_cache()

        valor_texto = str(valor)

        evento_status = (
            dispositivo == "status"
            and not message.retain
        )

        if (
            not evento_status
            and _ultimo_valor.get(dispositivo) == valor_texto
        ):
            return

        print(f"[MQTT] {dispositivo} = {valor_texto}", flush=True)

        if salvar_dado(dispositivo, valor_texto):
            _ultimo_valor[dispositivo] = valor_texto

    except Exception as erro:
        print(f"[MQTT] Erro ao processar mensagem: {erro}", flush=True)


# ============================================================
# INICIAR MQTT
# ============================================================

def iniciar_mqtt():

    client = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2
    )

    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message

    client.reconnect_delay_set(
        min_delay=1,
        max_delay=30
    )

    print("[MQTT] Iniciando HiveMQ...", flush=True)

    client.connect_async(
        MQTT_BROKER,
        MQTT_PORT,
        60
    )

    client.loop_start()

    print("[MQTT] Cliente iniciado!", flush=True)

    return client


if __name__ == "__main__":

    print("CASA INTELIGENTE - BANCO E MQTT")

    if not verificar_banco():
        print("[AVISO] Banco indisponivel.")

    else:
        criar_tabela()

    cliente = iniciar_mqtt()

    try:
        while True:
            time.sleep(1)

    except KeyboardInterrupt:
        cliente.loop_stop()
        cliente.disconnect()
        print("[MQTT] Encerrado.")
