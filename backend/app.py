from flask import Flask, render_template, jsonify

from db import (
    criar_tabela,
    iniciar_mqtt,
    buscar_ultimos_dados,
    buscar_historico
)


# ============================================================
# FLASK
# ============================================================

app = Flask(__name__)

print("[FLASK] Iniciando aplicação...", flush=True)

# ============================================================
# INICIALIZAÇÃO DO BANCO E MQTT
# ============================================================

criar_tabela()

mqtt_client = iniciar_mqtt()


# ============================================================
# PÁGINA PRINCIPAL
# ============================================================

@app.route("/")
def home():

    dispositivos = buscar_ultimos_dados()

    return render_template(
        "index.html",
        dispositivos=dispositivos
    )


# ============================================================
# API - ÚLTIMO ESTADO DOS DISPOSITIVOS
# ============================================================

@app.route("/api/dispositivos")
def api_dispositivos():

    try:

        dados = buscar_ultimos_dados()

        return jsonify(dados)

    except Exception as erro:

        print(
            f"[API] Erro ao buscar dispositivos: {erro}"
        )

        return jsonify({
            "erro": "Não foi possível consultar os dispositivos"
        }), 500


# ============================================================
# API - HISTÓRICO
# ============================================================

@app.route("/api/historico")
def api_historico():

    try:

        dados = buscar_historico()

        historico = []

        for registro in dados:

            historico.append({
                "id": registro["id"],
                "dispositivo": registro["dispositivo"],
                "valor": registro["valor"],
                "data_hora": registro["data_hora"].isoformat()
            })

        return jsonify(historico)

    except Exception as erro:

        print(
            f"[API] Erro ao buscar histórico: {erro}"
        )

        return jsonify({
            "erro": "Não foi possível consultar o histórico"
        }), 500


# ============================================================
# EXECUTAR SERVIDOR
# ============================================================

if __name__ == "__main__":

    print()
    print("======================================")
    print("       CASA INTELIGENTE - FLASK")
    print("======================================")
    print()
    print("Site: http://localhost:5000")
    print("MQTT: HiveMQ")
    print("Banco: Neon PostgreSQL")
    print()

    app.run(
        host="0.0.0.0",
        port=5000,
        debug=True,
        use_reloader=False
    )