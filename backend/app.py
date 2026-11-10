"""
app.py — API Flask do RespiraGyn.

Orquestração:
    1. Lê os setores cadastrados em setores_goiania.json
    2. Coleta tráfego de cada setor (Google Maps ou mock)
    3. Converte tudo em CSV e manda pro binário C (./core/respiragyn --backend)
    4. O C calcula o PPM com a Pilha LIFO + recomendação de árvores
    5. O backend junta com a info de espécie arbórea, GRAVA em SQLite
       e serve ao frontend
    6. /api/previsao chama o C em modo --prever (lógica também em C)

Rodar em dev:
    make -C ../core             # compila o binário C uma vez
    pip install -r requirements.txt
    python app.py
    # abrir http://localhost:5000
"""

import json
import os
import subprocess
import time
from pathlib import Path

from flask import Flask, jsonify, request, send_from_directory
from flask_cors import CORS

from traffic_client import criar_cliente
import db
import forecast


BASE_DIR     = Path(__file__).resolve().parent
PROJECT_ROOT = BASE_DIR.parent
BIN_C        = PROJECT_ROOT / "core" / "respiragyn"
FRONTEND_DIR = PROJECT_ROOT / "frontend"
SETORES_FILE = BASE_DIR / "setores_goiania.json"


app = Flask(__name__, static_folder=str(FRONTEND_DIR))
CORS(app)

# Cria o schema do banco uma vez na subida do processo.
db.init_db()


def carregar_setores():
    with open(SETORES_FILE, "r", encoding="utf-8") as f:
        return json.load(f)


def processar_no_nucleo_c(amostras_csv: list[str]) -> list[dict]:
    """
    Pipe: envia CSV pro binário em C, recebe JSON de volta.
    Cada linha de amostras_csv é "id,fluxo,vel,area,nivel".
    Cada linha de saída é um dict JSON com ppm, nivel, arvores, empilhado.
    """
    if not BIN_C.exists():
        raise RuntimeError(
            f"Binário C não encontrado em {BIN_C}. "
            f"Rode `make -C ../core` antes."
        )

    proc = subprocess.run(
        [str(BIN_C), "--backend"],
        input="\n".join(amostras_csv),
        capture_output=True, text=True, timeout=10,
    )

    if proc.returncode != 0:
        raise RuntimeError(f"Núcleo C falhou: {proc.stderr}")

    saidas = []
    for linha in proc.stdout.strip().splitlines():
        try:
            saidas.append(json.loads(linha))
        except json.JSONDecodeError:
            pass
    return saidas


# --------------------------------------------------------------------------
# Rotas
# --------------------------------------------------------------------------

@app.route("/api/setores")
def api_setores():
    """Lista os setores cadastrados (sem coletar tráfego)."""
    return jsonify(carregar_setores())


@app.route("/api/leituras")
def api_leituras():
    """
    Coleta tráfego de todos os setores, processa no núcleo C, grava em
    SQLite e devolve pro frontend desenhar o mapa de calor + recomendações.

    A espécie arbórea é fixa por setor (decisão de planejamento urbano,
    não muda por hora). A QUANTIDADE de árvores sugeridas vem do núcleo
    C a partir do PPM × área do setor.
    """
    setores = carregar_setores()
    cliente_trafego = criar_cliente()

    amostras = [cliente_trafego.coletar(s) for s in setores]
    csv_lines = [a.to_csv_row() for a in amostras]

    ppm_resultados = processar_no_nucleo_c(csv_lines)
    ppm_por_setor = {r["id_setor"]: r for r in ppm_resultados}

    ts_agora = int(time.time())
    para_gravar: list[tuple] = []
    resposta = []
    for s, a in zip(setores, amostras):
        resultado = ppm_por_setor.get(s["id"], {
            "ppm": 420.0, "empilhado": False, "arvores": 0,
        })
        ppm = resultado["ppm"]
        arvores_dyn = int(resultado.get("arvores", 0))

        resposta.append({
            "id": s["id"],
            "nome": s["nome"],
            "lat": s["lat"],
            "lng": s["lng"],
            "ppm": ppm,
            "nivel_trafego": a.nivel_trafego,
            "fluxo_veicular": round(a.fluxo_veicular),
            "velocidade_media": round(a.velocidade_media, 1),
            "especie_recomendada": s["especie_recomendada"],
            "arvores_sugeridas": arvores_dyn,           # dinâmico, via C
            "area_km2":          s["area_km2"],
            "prioridade": _prioridade(ppm),
        })
        if resultado.get("empilhado", False):
            para_gravar.append((
                s["id"], float(ppm), int(a.nivel_trafego),
                float(a.fluxo_veicular), float(a.velocidade_media), ts_agora,
            ))

    if para_gravar:
        try:
            db.inserir_leituras_batch(para_gravar)
        except Exception as e:
            app.logger.error("Falha ao gravar leituras: %s", e)

    return jsonify({
        "fonte_trafego": type(cliente_trafego).__name__,
        "total_setores": len(resposta),
        "leituras": resposta,
    })


@app.route("/api/historico/<int:id_setor>")
def api_historico(id_setor: int):
    """Série temporal das últimas N horas (default 6) de um setor."""
    try:
        horas = int(request.args.get("horas", 6))
    except (TypeError, ValueError):
        horas = 6
    horas = max(1, min(horas, 72))

    serie = db.historico(id_setor, horas=horas)
    return jsonify({
        "id_setor": id_setor,
        "horas":    horas,
        "pontos":   serie,
    })


@app.route("/api/previsao/<int:id_setor>")
def api_previsao(id_setor: int):
    """
    Previsão de PPM para +1h / +3h / +6h.
    A lógica mora no núcleo C (forecast.c, chamado em modo --prever);
    este endpoint é só o adaptador HTTP.
    """
    serie = db.historico(id_setor, horas=6)
    ppms  = [float(p["ppm"]) for p in serie]
    resultado = forecast.prever_via_nucleo_c(BIN_C, ppms)
    resultado["id_setor"] = id_setor
    return jsonify(resultado)


def _prioridade(ppm: float) -> str:
    if ppm >= 1200: return "CRITICA"
    if ppm >=  800: return "ALTA"
    if ppm >=  550: return "MEDIA"
    return "BAIXA"


# Serve o frontend estático
@app.route("/")
def index():
    return send_from_directory(str(FRONTEND_DIR), "index.html")


@app.route("/<path:path>")
def static_files(path):
    return send_from_directory(str(FRONTEND_DIR), path)


if __name__ == "__main__":
    port = int(os.getenv("PORT", "5000"))
    print(f"\n>> RespiraGyn rodando em http://localhost:{port}\n")
    app.run(host="0.0.0.0", port=port, debug=True)
