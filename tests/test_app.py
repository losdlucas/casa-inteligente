import pytest
from unittest.mock import patch
from backend.app import app


@pytest.fixture
def client():
    app.config['TESTING'] = True

    with app.test_client() as client:
        yield client


def test_obter_historico_sucesso(client):
    with patch('backend.app.db.consultar_eventos') as mock_consultar:
        mock_consultar.return_value = [
            {
                "dispositivo": "porta",
                "evento": "abriu a porta",
                "data": "2026-03-28 10:00:00"
            }
        ]

        resposta = client.get('/api/historico')

        assert resposta.status_code == 200
        assert resposta.json[0]['dispositivo'] == "porta"


def test_evento_dispositivo_invalido(client):
    resposta = client.post(
        '/api/evento',
        json={
            "dispositivo": "televisao",
            "evento": "ligou"
        }
    )

    assert resposta.status_code == 400
    assert resposta.json['status'] == "erro"


@pytest.mark.parametrize("dispositivo, evento", [
    ("lampada", "ligou a lampada"),
    ("lampada_externa", "desligou a lampada externa"),
    ("porta", "abriu a porta"),
    ("alarme", "disparou o alarme")
])
def test_receber_eventos_validos(client, dispositivo, evento):
    with patch('backend.app.db.registrar_evento') as mock_registrar:

        resposta = client.post(
            '/api/evento',
            json={
                "dispositivo": dispositivo,
                "evento": evento
            }
        )

        assert resposta.status_code == 201
        assert resposta.json['status'] == "sucesso"

        mock_registrar.assert_called_once_with(
            dispositivo,
            evento
        )