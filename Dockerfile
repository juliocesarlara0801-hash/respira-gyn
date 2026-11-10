# ============================================================================
# RespiraGyn — Dockerfile multistage
# ----------------------------------------------------------------------------
# Stage 1 (builder): compila o nucleo em C com gcc + make.
# Stage 2 (runtime): imagem Python slim, copia so o binario e o backend.
#
# Build:   docker build -t respiragyn .
# Run:     docker run -p 5000:5000 respiragyn
# Open:    http://localhost:5000
# ============================================================================

# ---- Stage 1: compilador C -------------------------------------------------
FROM gcc:13 AS builder

WORKDIR /src
COPY core/ ./core/

RUN make -C core clean && make -C core

# ---- Stage 2: runtime Python -----------------------------------------------
FROM python:3.12-slim AS runtime

# Usuario sem root por higiene de container
RUN useradd -m -u 1001 respiragyn

WORKDIR /app

# Dependencias Python
COPY backend/requirements.txt ./backend/requirements.txt
RUN pip install --no-cache-dir -r backend/requirements.txt \
    && pip install --no-cache-dir gunicorn==21.2.0

# Copia o binario C ja compilado
COPY --from=builder /src/core/respiragyn ./core/respiragyn

# Copia o restante do projeto
COPY backend/ ./backend/
COPY frontend/ ./frontend/

# Pasta de dados (o SQLite grava em backend/respiragyn.db)
RUN chown -R respiragyn:respiragyn /app
USER respiragyn

ENV PORT=5000
EXPOSE 5000

# gunicorn em producao; app.py expoe `app` em backend/app.py
WORKDIR /app/backend
CMD ["sh", "-c", "gunicorn -w 2 -b 0.0.0.0:${PORT} app:app"]
