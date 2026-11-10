"""
db.py — Camada de persistência do RespiraGyn (SQLite stdlib, sem ORM).

Responsabilidades:
  - Criar o schema na primeira inicialização (idempotente).
  - Gravar cada leitura processada pelo núcleo C.
  - Devolver a série temporal de um setor para o dashboard chart.js.

Schema:
  CREATE TABLE leituras (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    id_setor       INTEGER NOT NULL,
    ppm            REAL    NOT NULL,
    nivel_trafego  INTEGER NOT NULL,
    fluxo          REAL    NOT NULL,
    velocidade     REAL    NOT NULL,
    ts             INTEGER NOT NULL          -- unix epoch em segundos
  );
  CREATE INDEX ... (id_setor, ts)            -- consulta por setor é a mais quente

Justificativa de design:
  - sqlite3 da stdlib: zero deploy, zero dependência, zero config.
    Suficiente para a demo do Inovatech e para um piloto piloto da banca.
  - Nenhum ORM: o schema tem 1 tabela e o SQL cabe em 3 linhas.
    SQLAlchemy seria mais código do que o problema justifica.
  - PRIMARY KEY AUTOINCREMENT + índice (id_setor, ts): consultas de histórico
    por setor ficam O(log n) sem dependência de ordem de inserção.
"""

from __future__ import annotations

import sqlite3
import time
from pathlib import Path
from typing import Iterable


BASE_DIR = Path(__file__).resolve().parent
DB_PATH  = BASE_DIR / "respiragyn.db"


_SCHEMA = """
CREATE TABLE IF NOT EXISTS leituras (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    id_setor       INTEGER NOT NULL,
    ppm            REAL    NOT NULL,
    nivel_trafego  INTEGER NOT NULL,
    fluxo          REAL    NOT NULL,
    velocidade     REAL    NOT NULL,
    ts             INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_leituras_setor_ts
    ON leituras (id_setor, ts DESC);
"""


def _conectar() -> sqlite3.Connection:
    """
    Abre conexão nova por chamada — Flask usa threads e sqlite3 não aceita
    compartilhar conexão entre threads sem `check_same_thread=False`.
    Como as operações são curtas, abrir/fechar a cada chamada é mais seguro
    do que pool compartilhado.
    """
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


def init_db() -> None:
    """Cria o schema se ainda não existir. Idempotente."""
    with _conectar() as conn:
        conn.executescript(_SCHEMA)


def inserir_leitura(
    id_setor: int,
    ppm: float,
    nivel_trafego: int,
    fluxo: float,
    velocidade: float,
    ts: int | None = None,
) -> None:
    """Grava uma leitura. Se `ts` for None, usa o relógio local."""
    if ts is None:
        ts = int(time.time())

    with _conectar() as conn:
        conn.execute(
            "INSERT INTO leituras "
            "(id_setor, ppm, nivel_trafego, fluxo, velocidade, ts) "
            "VALUES (?, ?, ?, ?, ?, ?)",
            (id_setor, float(ppm), int(nivel_trafego),
             float(fluxo), float(velocidade), int(ts)),
        )


def inserir_leituras_batch(linhas: Iterable[tuple]) -> None:
    """
    Insere várias leituras em uma só transação (a cada /api/leituras cai
    uma leitura por setor — vale fazer em batch para não fsync N vezes).
    Cada tupla: (id_setor, ppm, nivel_trafego, fluxo, velocidade, ts).
    """
    with _conectar() as conn:
        conn.executemany(
            "INSERT INTO leituras "
            "(id_setor, ppm, nivel_trafego, fluxo, velocidade, ts) "
            "VALUES (?, ?, ?, ?, ?, ?)",
            linhas,
        )


def historico(id_setor: int, horas: int = 6) -> list[dict]:
    """
    Devolve a série temporal (asc por ts) das últimas `horas` horas para o
    setor pedido. Formato pronto para o chart.js: lista de dicts com `ts`
    (epoch segundos) e `ppm`.
    """
    corte = int(time.time()) - max(1, horas) * 3600
    with _conectar() as conn:
        rows = conn.execute(
            "SELECT ts, ppm, nivel_trafego, fluxo, velocidade "
            "FROM leituras "
            "WHERE id_setor = ? AND ts >= ? "
            "ORDER BY ts ASC",
            (int(id_setor), corte),
        ).fetchall()
    return [dict(r) for r in rows]


def ultima_leitura(id_setor: int) -> dict | None:
    """Última leitura gravada daquele setor (ou None se nunca gravou)."""
    with _conectar() as conn:
        row = conn.execute(
            "SELECT ts, ppm, nivel_trafego, fluxo, velocidade "
            "FROM leituras "
            "WHERE id_setor = ? "
            "ORDER BY ts DESC LIMIT 1",
            (int(id_setor),),
        ).fetchone()
    return dict(row) if row else None


def contar_leituras() -> int:
    """Diagnóstico: quantas leituras há no banco."""
    with _conectar() as conn:
        (n,) = conn.execute("SELECT COUNT(*) FROM leituras").fetchone()
    return int(n)
