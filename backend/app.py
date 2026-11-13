
from pathlib import Path

from flask import Flask, render_template, jsonify, request

try:
    from .db import (
        criar_tabela,
        iniciar_mqtt,
        buscar_ultimos_dados,
        buscar_historico,
        MQTT_BASE_TOPIC
    )
except ImportError:
    from db import (
        criar_tabela,
        iniciar_mqtt,
        buscar_ultimos_dados,
        buscar_historico,
        MQTT_BASE_TOPIC
    )


# ============================================================
# FLASK
# ============================================================

BASE_DIR = Path(__file__).resolve().parent.parent
FRONTEND_DIR = BASE_DIR / "frontend"

app = Flask(
    __name__,
    template_folder=str(FRONTEND_DIR),
    static_folder=str(FRONTEND_DIR),
    static_url_path="/static"
)


# ============================================================
# COMANDOS
# ============================================================

COMANDOS_VALIDOS = {
    "LED1_ON",
    "LED1_OFF",
    "LED2_ON",
    "LED2_OFF",
    "ALARME_ON",
    "ALARME_OFF",
    "BUZZER_ON",
    "BUZZER_OFF",
    "SERVO_ABRIR",
    "SERVO_FECHAR"
}


SERVO_ESTADOS = {
    "MOVENDO_0_180": "abrindo",
    "MOVENDO_180_0": "fechando",
    "180_GRAUS": "aberto",
    "PARADO_180": "aberto",
    "0_GRAUS": "fechado",
    "PARADO_0": "fechado"
}


# ============================================================
# INICIAR SERVICOS
# ============================================================

print("[FLASK] Iniciando Casa Inteligente...", flush=True)

print(f"[FLASK] Frontend: {FRONTEND_DIR}", flush=True)

print(
    f"[FLASK] HTML encontrado: {(FRONTEND_DIR / 'index.html').exists()}",
    flush=True
)

mqtt_client = None

try:
    criar_tabela()
    print("[BANCO] Banco verificado!", flush=True)

except Exception as erro:
    print(
        f"[BANCO] Indisponivel na inicializacao: {erro}",
        flush=True
    )

try:
    mqtt_client = iniciar_mqtt()
    print("[MQTT] Cliente iniciado!", flush=True)

except Exception as erro:
    print(f"[MQTT] Erro ao iniciar: {erro}", flush=True)


# ============================================================
# CONSULTAR BANCO COM SEGURANCA
# ============================================================

def obter_dispositivos():
    try:
        return buscar_ultimos_dados()

    except Exception as erro:
        print(f"[BANCO] Consulta indisponivel: {erro}", flush=True)
        return {}


# ============================================================
# PAGINA INICIAL
# ============================================================

@app.route("/")
def home():

    dispositivos = obter_dispositivos()

    return render_template(
        "index.html",
        dispositivos=dispositivos
    )


# ============================================================
# API DISPOSITIVOS
# ============================================================

@app.route("/api/dispositivos")
def api_dispositivos():

    return jsonify(obter_dispositivos())


# ============================================================
# API HISTORICO
# ============================================================

@app.route("/api/historico")
def api_historico():

    try:
        limite = request.args.get(
            "limite",
            default=50,
            type=int
        )

        limite = max(1, min(limite, 200))

        dados = buscar_historico(limite)

        resultado = []

        for registro in dados:
            resultado.append({
                "id": registro["id"],
                "dispositivo": registro["dispositivo"],
                "valor": registro["valor"],
                "data_hora": registro["data_hora"].isoformat()
            })

        return jsonify(resultado)

    except Exception as erro:
        print(f"[HISTORICO] Erro: {erro}", flush=True)
        return jsonify([])


# ============================================================
# API ESTADO
# ============================================================

@app.route("/api/estado")
def api_estado():

    dados = obter_dispositivos()

    def valor(nome):
        item = dados.get(nome) or {}
        return item.get("valor")

    status = valor("status")

    esp32 = (
        "online"
        if status == "ESP32_ONLINE"
        else "offline"
        if status == "ESP32_OFFLINE"
        else "desconhecido"
    )

    return jsonify({
        "esp32": esp32,
        "led1": valor("led1") == "LIGADO",
        "led2": valor("led2") == "LIGADO",
        "buzzer": valor("buzzer") == "LIGADO",
        "alarme": valor("alarme") == "ATIVADO",
        "pir": valor("pir") == "MOVIMENTO_DETECTADO",
        "pir_conhecido": valor("pir") is not None,
        "servo": SERVO_ESTADOS.get(
            valor("servo"),
            "desconhecido"
        )
    })


# ============================================================
# ENVIAR COMANDOS MQTT
# ============================================================

@app.route("/api/comando", methods=["POST"])
def enviar_comando():

    try:
        dados = request.get_json(silent=True) or {}

        comando = dados.get("comando")

        if (
            not isinstance(comando, str)
            or comando not in COMANDOS_VALIDOS
        ):
            return jsonify({
                "erro": "Comando invalido"
            }), 400

        if mqtt_client is None or not mqtt_client.is_connected():
            return jsonify({
                "erro": "MQTT nao esta conectado"
            }), 503

        resultado = mqtt_client.publish(
            f"{MQTT_BASE_TOPIC}/comando",
            comando,
            qos=0,
            retain=False
        )

        if resultado.rc != 0:
            return jsonify({
                "erro": "Falha ao enviar comando"
            }), 500

        print(f"[SITE] Comando enviado: {comando}", flush=True)

        return jsonify({
            "mensagem": "Comando enviado com sucesso",
            "comando": comando
        })

    except Exception as erro:
        print(f"[COMANDO] Erro: {erro}", flush=True)

        return jsonify({
            "erro": "Erro ao enviar comando"
        }), 500


# ============================================================
# EXECUTAR
# ============================================================

if __name__ == "__main__":

    app.run(
        host="0.0.0.0",
        port=5000,
        debug=True,
        use_reloader=False
    )
