import os
import psycopg2
from dotenv import load_dotenv

load_dotenv()

DATABASE_URL = os.getenv("DATABASE_URL")


def conectar_banco():
    return psycopg2.connect(DATABASE_URL)


def registrar_evento(dispositivo, evento, tempo=None):
    conexao = conectar_banco()

    try:
        cursor = conexao.cursor()

        cursor.execute(
            """
            INSERT INTO eventos (dispositivo, evento, tempo)
            VALUES (%s, %s, %s)
            """,
            (dispositivo, evento, tempo)
        )

        conexao.commit()

    finally:
        cursor.close()
        conexao.close()


def consultar_eventos():
    conexao = conectar_banco()

    try:
        cursor = conexao.cursor()

        cursor.execute(
            """
            SELECT dispositivo, evento, data, tempo
            FROM eventos
            ORDER BY data DESC
            """
        )

        resultados = cursor.fetchall()

        eventos = []

        for dispositivo, evento, data, tempo in resultados:
            eventos.append({
                "dispositivo": dispositivo,
                "evento": evento,
                "data": data.strftime("%Y-%m-%d %H:%M:%S"),
                "tempo": tempo
            })

        return eventos

    finally:
        cursor.close()
        conexao.close()