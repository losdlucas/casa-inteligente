from backend.app import app


def test_rota_inicial():
    client = app.test_client()

    resposta = client.get("/")

    assert resposta.status_code == 200


def test_ligar_luz(monkeypatch):
    client = app.test_client()

    def fake_registrar_evento(dispositivo, evento, tempo=None):
        pass

    monkeypatch.setattr(
        "backend.app.db.registrar_evento",
        fake_registrar_evento
    )

    resposta = client.post(
        "/api/luz",
        json={
            "comodo": "sala",
            "estado": True,
            "tempo": 10
        }
    )

    assert resposta.status_code == 200

    dados = resposta.get_json()

    assert dados["status"] == "sucesso"
    assert dados["comodo"] == "sala"
    assert dados["estado"] is True


def test_desligar_luz(monkeypatch):
    client = app.test_client()

    def fake_registrar_evento(dispositivo, evento, tempo=None):
        pass

    monkeypatch.setattr(
        "backend.app.db.registrar_evento",
        fake_registrar_evento
    )

    resposta = client.post(
        "/api/luz",
        json={
            "comodo": "sala",
            "estado": False,
            "tempo": 10
        }
    )

    assert resposta.status_code == 200

    dados = resposta.get_json()

    assert dados["status"] == "sucesso"
    assert dados["estado"] is False


def test_comodo_invalido(monkeypatch):
    client = app.test_client()

    resposta = client.post(
        "/api/luz",
        json={
            "comodo": "garagem",
            "estado": True
        }
    )

    assert resposta.status_code == 400


def test_estado_invalido(monkeypatch):
    client = app.test_client()

    resposta = client.post(
        "/api/luz",
        json={
            "comodo": "sala",
            "estado": "ligado"
        }
    )

    assert resposta.status_code == 400


def test_comodo_nao_informado(monkeypatch):
    client = app.test_client()

    resposta = client.post(
        "/api/luz",
        json={
            "estado": True
        }
    )

    assert resposta.status_code == 400