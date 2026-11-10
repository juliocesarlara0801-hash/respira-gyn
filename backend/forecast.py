"""
forecast.py — Adaptador fino sobre o núcleo C em modo --prever.

A LÓGICA de previsão (WMA + fator horário) está em core/forecast.c.
Este módulo só serializa a série histórica como PPMs em stdin para o
binário C e parseia o JSON que ele devolve. Mantém a promessa de
"projeto predominantemente em C" exigida pelo edital.
"""

from __future__ import annotations

import json
import subprocess
from pathlib import Path
from typing import Iterable


def prever_via_nucleo_c(bin_c: Path, ppms: Iterable[float]) -> dict:
    """
    Chama `./respiragyn --prever`, envia uma série de PPMs (um por linha)
    e devolve o JSON parseado. Nunca lança por falta de histórico —
    nesse caso o próprio C devolve pontos_usados=0 e previsões sobre a
    atmosfera base.
    """
    entrada = "\n".join(f"{v:.2f}" for v in ppms) + "\n"

    proc = subprocess.run(
        [str(bin_c), "--prever"],
        input=entrada,
        capture_output=True, text=True, timeout=5,
    )
    if proc.returncode != 0:
        # Fallback silencioso: não derruba o endpoint por falha no pipe.
        return {
            "base_ppm":      420.0,
            "pontos_usados": 0,
            "pico_proximo":  False,
            "previsoes":     [],
            "erro":          proc.stderr.strip() or "nucleo C falhou",
        }

    saida = proc.stdout.strip().splitlines()[-1]
    try:
        return json.loads(saida)
    except json.JSONDecodeError as e:
        return {
            "base_ppm":      420.0,
            "pontos_usados": 0,
            "pico_proximo":  False,
            "previsoes":     [],
            "erro":          f"json invalido do nucleo C: {e}",
        }
