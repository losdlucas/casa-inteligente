
from flask import Flask, render_template, jsonify

from db import (
    criar_tabela,
    iniciar_mqtt,
    buscar_ultimos_dados,
    buscar_historico
)

app = Flask(__name__)

print("[FLASK] Iniciando aplicação...", flush=True)

mqtt_client = None

try:
    criar_tabela()
    print("[BANCO] Tabelas verificadas.", flush=True)
except Exception as erro:
    print(f"[BANCO] Erro: {erro}", flush=True)


@app.route("/")
def home():
    try:
        dispositivos = buscar_ultimos_dados()
        return render_template(
            "index.html",
            dispositivos=dispositivos
        )
    except Exception as erro:
        print(f"[HOME] Erro: {erro}", flush=True)
        return "Erro ao carregar dispositivos", 500


@app.route("/api/dispositivos")
def api_dispositivos():
    try:
        return jsonify(buscar_ultimos_dados())
    except Exception as erro:
        print(f"[API] Erro: {erro}", flush=True)
        return jsonify({
            "erro": "Não foi possível consultar os dispositivos"
        }), 500


@app.route("/api/historico")
def api_historico():
    try:
        dados = buscar_historico()

        historico = [
            {
                "id": registro["id"],
                "dispositivo": registro["dispositivo"],
                "valor": registro["valor"],
                "data_hora": registro["data_hora"].isoformat()
            }
            for registro in dados
        ]

        return jsonify(historico)

    except Exception as erro:
        print(f"[API] Erro: {erro}", flush=True)
        return jsonify({
            "erro": "Não foi possível consultar o histórico"
        }), 500


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)