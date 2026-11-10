"""
traffic_client.py — Cliente de APIs de tráfego.

Duas implementações:
  - MockTrafficClient: gera dados realistas sem precisar de API key.
    Ideal para a demo em sala e para desenvolvimento local.
  - GoogleMapsTrafficClient: consulta a Google Maps Platform (Roads +
    Distance Matrix) quando GOOGLE_MAPS_API_KEY estiver no ambiente.

A classe base TrafficClient define o contrato: dado um setor, devolve uma
AmostraTrafego com fluxo_veicular, velocidade_media e nivel_trafego.
"""

from __future__ import annotations

import os
import random
import math
from dataclasses import dataclass
from datetime import datetime
from typing import Optional

import requests


@dataclass
class AmostraTrafego:
    id_setor: int
    fluxo_veicular: float   # veículos/hora
    velocidade_media: float  # km/h
    area_km2: float
    nivel_trafego: int      # 0..5

    def to_csv_row(self) -> str:
        """Formato consumido pelo binário C em modo --backend."""
        return (f"{self.id_setor},{self.fluxo_veicular:.2f},"
                f"{self.velocidade_media:.2f},{self.area_km2:.3f},"
                f"{self.nivel_trafego}")


class TrafficClient:
    """Contrato que qualquer provedor de tráfego deve cumprir."""
    def coletar(self, setor: dict) -> AmostraTrafego:
        raise NotImplementedError


# ---------------------------------------------------------------------------
# MOCK — simulação baseada em padrões reais de Goiânia
# ---------------------------------------------------------------------------
class MockTrafficClient(TrafficClient):
    """
    Gera amostras realistas: setores centrais ficam mais carregados em horário
    de pico (7-9h e 17-19h), zonas periféricas ficam mais calmas. Sempre há
    variação aleatória (±15%) para dar movimento ao mapa de calor.
    """

    def coletar(self, setor: dict) -> AmostraTrafego:
        hora = datetime.now().hour
        pico = hora in range(7, 10) or hora in range(17, 20)

        # Peso central: setores com id<=4 são corredores conhecidos de Goiânia
        central = setor["id"] <= 4

        base_fluxo    = 2500 if central else 800
        base_vel      = 22   if central else 50
        base_nivel    = 4    if central else 2

        if pico:
            base_fluxo *= 1.6
            base_vel   *= 0.6
            base_nivel  = min(5, base_nivel + 1)

        # ruído aleatório ±15%
        noise = lambda v, p=0.15: v * (1 + random.uniform(-p, p))

        return AmostraTrafego(
            id_setor=setor["id"],
            fluxo_veicular=max(50.0,  noise(base_fluxo)),
            velocidade_media=max(5.0, noise(base_vel)),
            area_km2=setor["area_km2"],
            nivel_trafego=base_nivel,
        )


# ---------------------------------------------------------------------------
# GOOGLE MAPS — requer GOOGLE_MAPS_API_KEY
# ---------------------------------------------------------------------------
class GoogleMapsTrafficClient(TrafficClient):
    """
    Consulta a Distance Matrix API com `departure_time=now` para obter
    duration_in_traffic, do qual derivamos velocidade e nível.
    Fluxo veicular é estimado heuristicamente a partir da área e do nível.

    Nota: para o trabalho do Inovatech o MockTrafficClient já é suficiente
    para demonstrar o conceito. Este cliente existe para mostrar que a
    arquitetura está pronta para produção quando a equipe tiver a chave.
    """

    ENDPOINT = "https://maps.googleapis.com/maps/api/distancematrix/json"

    def __init__(self, api_key: Optional[str] = None):
        self.api_key = api_key or os.getenv("GOOGLE_MAPS_API_KEY")
        if not self.api_key:
            raise RuntimeError(
                "GOOGLE_MAPS_API_KEY ausente. Use MockTrafficClient em dev."
            )

    def coletar(self, setor: dict) -> AmostraTrafego:
        # Rota curta de 500m dentro do setor para medir velocidade local
        origem = f"{setor['lat']},{setor['lng']}"
        destino_lat = setor["lat"] + 0.004   # ~ 440 m
        destino_lng = setor["lng"]
        destino = f"{destino_lat},{destino_lng}"

        params = {
            "origins": origem,
            "destinations": destino,
            "departure_time": "now",
            "traffic_model": "best_guess",
            "key": self.api_key,
        }

        resp = requests.get(self.ENDPOINT, params=params, timeout=10)
        data = resp.json()

        try:
            elem = data["rows"][0]["elements"][0]
            dist_m = elem["distance"]["value"]
            dur_s  = elem["duration_in_traffic"]["value"]
            vel_kmh = (dist_m / dur_s) * 3.6
        except (KeyError, IndexError, ZeroDivisionError):
            # fallback caso a API falhe
            vel_kmh = 40.0

        # Nível de tráfego derivado da velocidade
        if vel_kmh < 10:   nivel = 5
        elif vel_kmh < 20: nivel = 4
        elif vel_kmh < 35: nivel = 3
        elif vel_kmh < 50: nivel = 2
        else:              nivel = 1

        # Fluxo veicular estimado: setor carregado = mais carros
        fluxo = 500 + (5 - nivel) * -300 + 2500 * (nivel / 5)

        return AmostraTrafego(
            id_setor=setor["id"],
            fluxo_veicular=max(50.0, fluxo),
            velocidade_media=vel_kmh,
            area_km2=setor["area_km2"],
            nivel_trafego=nivel,
        )


def criar_cliente() -> TrafficClient:
    """Factory: usa Google Maps se tiver key, senão mock."""
    if os.getenv("GOOGLE_MAPS_API_KEY"):
        return GoogleMapsTrafficClient()
    return MockTrafficClient()
