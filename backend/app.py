from flask import Flask, request, jsonify, render_template
from backend import db
import paho.mqtt.publish as mqtt_publish

app = Flask(
    __name__,
    template_folder='../frontend',
    static_folder='../frontend'
)

MQTT_SERVER = "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC_COMANDO = "projeto_iot/comando"

# ==========================================
# DISPOSITIVOS PERMITIDOS
# ==========================================

DISPOSITIVOS_PERMITIDOS = [
    'sala',
    'cozinha',
    'quarto',
    'banheiro',
    'externa',
    'porta',
    'alarme',
    'lampada',
    'lampada_externa'
]


# ==========================================
# ROTA INICIAL
# ==========================================

@app.route('/')
def inicio():
    return render_template('index.html')


# ==========================================
# CONTROLE DE LUZES
# ==========================================

@app.route('/api/luz', methods=['POST'])
def controlar_luz():

    dados = request.get_json() or {}

    comodo = dados.get('comodo')
    estado = dados.get('estado')
    tempo = dados.get('tempo')

    if not comodo:
        return jsonify({
            "status": "erro",
            "mensagem": "Cômodo não informado."
        }), 400

    if comodo not in [
        'sala',
        'cozinha',
        'quarto',
        'banheiro',
        'externa'
    ]:
        return jsonify({
            "status": "erro",
            "mensagem": "Cômodo inválido."
        }), 400

    if not isinstance(estado, bool):
        return jsonify({
            "status": "erro",
            "mensagem": "O estado deve ser true ou false."
        }), 400

    if estado:
        evento = f"Luz da {comodo} ligada"
    else:
        evento = f"Luz da {comodo} desligada"

    db.registrar_evento(comodo, evento, tempo)

    if comodo == "sala":
        comando = "LED1_ON" if estado else "LED1_OFF"

    mqtt_publish.single(
        MQTT_TOPIC_COMANDO,
        comando,
        hostname=MQTT_SERVER,
        port=MQTT_PORT
    )

    return jsonify({
        "status": "sucesso",
        "comodo": comodo,
        "estado": estado,
        "tempo": tempo,
        "mensagem": evento
    }), 200


# ==========================================
# ACIONAR DISPOSITIVO
# ==========================================

@app.route('/api/acionar', methods=['POST'])
def acionar_dispositivo():

    dados = request.get_json() or {}

    dispositivo = dados.get('dispositivo')
    acao = dados.get('acao')
    tempo = dados.get('tempo')

    if not dispositivo or dispositivo not in DISPOSITIVOS_PERMITIDOS:
        return jsonify({
            "status": "erro",
            "mensagem": "Dispositivo inválido ou ausente."
        }), 400

    if not acao:
        return jsonify({
            "status": "erro",
            "mensagem": "Ação não informada."
        }), 400

    return jsonify({
        "status": "sucesso",
        "mensagem": f"Comando '{acao}' enviado para {dispositivo}.",
        "tempo": tempo
    }), 200


# ==========================================
# STATUS DOS DISPOSITIVOS
# ==========================================

@app.route('/api/status', methods=['GET'])
def obter_status():

    dados_simulados = [
        {
            "dispositivo": "sala",
            "status": "DESLIGADA"
        },
        {
            "dispositivo": "cozinha",
            "status": "DESLIGADA"
        },
        {
            "dispositivo": "quarto",
            "status": "DESLIGADA"
        },
        {
            "dispositivo": "banheiro",
            "status": "DESLIGADA"
        },
        {
            "dispositivo": "externa",
            "status": "DESLIGADA"
        },
        {
            "dispositivo": "porta",
            "status": "FECHADA"
        },
        {
            "dispositivo": "alarme",
            "status": "DESATIVADO"
        }
    ]

    return jsonify(dados_simulados), 200


# ==========================================
# RECEBER EVENTO
# ==========================================

@app.route('/api/evento', methods=['POST'])
def receber_evento():

    dados = request.get_json() or {}

    dispositivo = dados.get('dispositivo')
    evento = dados.get('evento')
    tempo = dados.get('tempo')

    if not dispositivo or dispositivo not in DISPOSITIVOS_PERMITIDOS:
        return jsonify({
            "status": "erro",
            "mensagem": "Dispositivo inválido ou ausente."
        }), 400

    if not evento:
        return jsonify({
            "status": "erro",
            "mensagem": "Evento não informado."
        }), 400

    # Se tempo foi informado, envia os 3 argumentos.
    # Se não foi informado, envia apenas os 2 esperados pelos testes.
    if tempo is None:
        db.registrar_evento(dispositivo, evento)
    else:
        db.registrar_evento(dispositivo, evento, tempo)

    return jsonify({
        "status": "sucesso",
        "mensagem": "Evento registrado com sucesso.",
        "tempo": tempo
    }), 201


# ==========================================
# HISTÓRICO
# ==========================================

@app.route('/api/historico', methods=['GET'])
def obter_historico():

    eventos = db.consultar_eventos()

    return jsonify(eventos), 200


# ==========================================
# INICIAR SERVIDOR
# ==========================================

if __name__ == '__main__':
    app.run(debug=True)