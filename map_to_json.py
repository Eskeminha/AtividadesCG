#!/usr/bin/env python3
"""
Converte um mapa ASCII (map.txt) em assets/scene.json para o visualizador Final.

Uso:
    python3 map_to_json.py

Edite o MAP abaixo (ou leia de um arquivo) e rode o script.
O scene.json gerado pode ser copiado para build/assets/ com:
    cp assets/scene.json build/assets/scene.json
"""
import json, math

# ── Configuração do grid ──────────────────────────────────────────────────────
CELL = 1.5          # tamanho de cada célula em unidades do mundo
COLS = 14           # número de colunas
ROWS = 9            # número de linhas

# Origem: coluna 0 fica em X = -(COLS*CELL/2 - CELL/2), linha 0 em Z = -CELL/2
def col_x(c): return round(c * CELL - (COLS * CELL) / 2.0 + CELL / 2.0, 3)
def row_z(r): return round(-(r * CELL + CELL / 2.0), 3)

# ── Tabela de símbolos ────────────────────────────────────────────────────────
BASE = "assets/Modelos3D/kenney_pirate-kit/Models/OBJ format"

# Cada entrada: name, file, scale, dy (offset Y), e opcionalmente rotation_* e trajectory
SYMBOLS = {
    # terreno / ilha
    'G': {"name": "Gramado",    "file": f"{BASE}/patch-grass.obj",             "scale": 1.2,  "dy":  0.0},
    's': {"name": "Praia",      "file": f"{BASE}/patch-sand.obj",              "scale": 1.0,  "dy": -0.25},
    'R': {"name": "PedrasAreia","file": f"{BASE}/rocks-sand-a.obj",            "scale": 0.35, "dy":  0.25},
    'r': {"name": "Rochas",     "file": f"{BASE}/rocks-a.obj",                 "scale": 0.3,  "dy":  0.0},
    # construções
    'T': {"name": "Torre",      "file": f"{BASE}/tower-complete-large.obj",    "scale": 0.55, "dy":  0.25},
    't': {"name": "Torrinha",   "file": f"{BASE}/tower-complete-small.obj",    "scale": 0.45, "dy":  0.25},
    # vegetação
    'P': {"name": "Palmeira",   "file": f"{BASE}/palm-straight.obj",           "scale": 0.55, "dy":  0.25},
    'p': {"name": "PalmCurva",  "file": f"{BASE}/palm-bend.obj",               "scale": 0.5,  "dy":  0.25},
    'Q': {"name": "PalmDetal",  "file": f"{BASE}/palm-detailed-straight.obj",  "scale": 0.5,  "dy":  0.25},
    'q': {"name": "PalmDCurva", "file": f"{BASE}/palm-detailed-bend.obj",      "scale": 0.5,  "dy":  0.25},
    # cais
    'D': {"name": "Doca",       "file": f"{BASE}/structure-platform-dock.obj", "scale": 0.5,  "dy":  0.0},
    # props no cais  (dy=0.655 = topo do dock em scale=0.5)
    'C': {"name": "Canhao",     "file": f"{BASE}/cannon.obj",                  "scale": 0.6,  "dy":  0.655,
          "rotation_axis": [0, 1, 0], "rotation_angle": 90},
    'B': {"name": "Barril",     "file": f"{BASE}/barrel.obj",                  "scale": 0.45, "dy":  0.655},
    'b': {"name": "Bau",        "file": f"{BASE}/chest.obj",                   "scale": 0.5,  "dy":  0.655},
    'x': {"name": "Caixote",    "file": f"{BASE}/crate.obj",                   "scale": 0.45, "dy":  0.655},
    # navios
    'N': {"name": "Navio",      "file": f"{BASE}/ship-pirate-large.obj",       "scale": 0.5,  "dy":  0.0,
          # trajetória oval ao redor da ilha (pontos em sentido horário)
          "trajectory": [
              [ 2.75, 0.0, -6.75],
              [ 0.41, 0.0, -2.51],
              [-5.25, 0.0, -0.75],
              [-10.91,0.0, -2.51],
              [-13.25,0.0, -6.75],
              [-10.91,0.0,-10.99],
              [-5.25, 0.0,-12.75],
              [ 0.41, 0.0,-10.99],
          ]},
    'n': {"name": "Fantasma",   "file": f"{BASE}/ship-ghost.obj",              "scale": 0.45, "dy":  0.0},
    'k': {"name": "Naufragio",  "file": f"{BASE}/ship-wreck.obj",              "scale": 0.4,  "dy": -3.0},
    'm': {"name": "NavioMedio", "file": f"{BASE}/ship-pirate-medium.obj",      "scale": 0.45, "dy":  0.0},
}

# ── Mapa ASCII ────────────────────────────────────────────────────────────────
# Cada linha deve ter exatamente COLS caracteres.
# '.' = água (nenhum objeto gerado)
# 'C' = canhão (sempre colocado como índice 0, âncora das luzes)
# EDITE AQUI ou carregue de um arquivo externo.
#
# Legenda rápida:
#   .=água  G=grama  s=areia  R=pedra-areia  r=rocha
#   T=torre grande  t=torre pequena
#   P=palmeira  p=palma-curva  Q=palma-detal  q=palma-detal-curva
#   D=doca  C=canhão  B=barril  b=baú  x=caixote
#   N=navio pirata (órbita ilha)  n=fantasma  k=naufrágio  m=navio médio
#
#        col:  0  1  2  3  4  5  6  7  8  9  10 11 12 13
#        X:  -9.75 ...              ... 0.75 ...        9.75
MAP = [
    "..............",   # row0  Z≈ -0.75
    ".PGGss........",   # row1  Z≈ -2.25  praia norte + palmeira
    ".GGTGGrD......",   # row2  Z≈ -3.75  torre + início do cais
    ".GGGGGrDC.....",   # row3  Z≈ -5.25  canhão
    ".GpGGGrDb.....",   # row4  Z≈ -6.75  baú
    ".RGGGGrDB.N...",   # row5  Z≈ -8.25  barril + navio pirata
    "..RGGtDx......",   # row6  Z≈ -9.75  torrinha + caixote
    "...GPq.....n..",   # row7  Z≈-11.25  palmeiras sul + fantasma
    "...........k..",   # row8  Z≈-12.75  naufrágio
]

# ── Configuração da câmera e luzes (pode ser editado) ─────────────────────────
CAMERA = {
    "position": [0.0, 10.0, 16.0],
    "yaw":  -90.0,
    "pitch": -25.0,
    "fov":   45.0,
    "near":   0.1,
    "far":  100.0
}
LIGHTS = [
    {"intensity": 5.0, "on": True},
    {"intensity": 2.0, "on": True},
    {"intensity": 2.5, "on": True}
]

# ── Geração dos objetos ───────────────────────────────────────────────────────
def build_objects():
    objects  = []
    counters = {}

    def unique_name(base):
        counters[base] = counters.get(base, 0) + 1
        n = counters[base]
        return base if n == 1 else f"{base}{n}"

    def make(sym, c, r):
        d  = SYMBOLS[sym]
        cx = col_x(c)
        cz = row_z(r)
        obj = {
            "name":     unique_name(d["name"]),
            "file":     d["file"],
            "position": [cx, d["dy"], cz],
            "scale":    d["scale"],
        }
        if "rotation_axis"  in d: obj["rotation_axis"]  = d["rotation_axis"]
        if "rotation_angle" in d: obj["rotation_angle"] = d["rotation_angle"]
        if "trajectory"     in d: obj["trajectory"]     = d["trajectory"]
        return obj

    # ── Índice 0: Canhão (âncora das luzes) ──────────────────────────────────
    # Procura 'C' no mapa para obter posição; usa centro da cena como fallback
    cx0, cz0 = col_x(COLS // 2), row_z(ROWS // 2)
    for r, row in enumerate(MAP):
        for c, ch in enumerate(row):
            if ch == 'C':
                cx0, cz0 = col_x(c), row_z(r)
    # Sobrescreve contagem para garantir nome "Canhao" sem número
    counters["Canhao"] = 1
    objects.append({
        "name":           "Canhao",
        "file":           f"{BASE}/cannon.obj",
        "position":       [cx0, SYMBOLS['C']['dy'], cz0],
        "scale":          SYMBOLS['C']['scale'],
        "rotation_axis":  [0, 1, 0],
        "rotation_angle": 90
    })

    # ── Índice 1: Bala de canhão (trajetória automática) ─────────────────────
    traj_bala = [
        [round(cx0 + 0.5, 3), 1.0,  round(cz0 + 1.0, 3)],
        [round(cx0 + 2.5, 3), 4.5,  round(cz0 - 1.0, 3)],
        [round(cx0 + 4.0, 3), 1.5,  round(cz0 - 3.0, 3)],
        [round(cx0 + 3.0, 3), 3.5,  round(cz0 - 6.0, 3)],
        [round(cx0 + 0.5, 3), 5.0,  round(cz0 - 7.5, 3)],
        [round(cx0 - 1.0, 3), 2.0,  round(cz0 - 5.0, 3)],
    ]
    objects.append({
        "name":       "Bala",
        "file":       f"{BASE}/cannon-ball.obj",
        "position":   traj_bala[0],
        "scale":      0.4,
        "trajectory": traj_bala
    })

    # ── Restante do mapa (exceto '.' e 'C') ──────────────────────────────────
    for r, row in enumerate(MAP):
        if len(row) != COLS:
            print(f"Aviso: linha {r} tem {len(row)} colunas (esperado {COLS})")
        for c, ch in enumerate(row):
            if ch in ('.', 'C'):
                continue
            if ch not in SYMBOLS:
                print(f"Aviso: símbolo '{ch}' desconhecido em linha {r}, col {c}")
                continue
            objects.append(make(ch, c, r))

    return objects


# ── Geração do JSON ───────────────────────────────────────────────────────────
scene = {
    "camera":  CAMERA,
    "lights":  LIGHTS,
    "objects": build_objects()
}

out = "assets/scene.json"
with open(out, "w", encoding="utf-8") as f:
    json.dump(scene, f, indent=4, ensure_ascii=False)

print(f"Gerado {out} com {len(scene['objects'])} objetos.")
print()
print("Legenda de símbolos:")
print("  .=água (vazio)   G=grama   s=areia   R=pedra-areia   r=rocha")
print("  T=torre  t=torrinha  P=palmeira  p=palma-curva  Q=palmDetal  q=palmDetal-curva")
print("  D=doca  C=canhão  B=barril  b=baú  x=caixote")
print("  N=navio pirata (órbita)  n=fantasma  k=naufrágio  m=navio médio")
print()
print("Após gerar, copie para o build:")
print("  cp assets/scene.json build/assets/scene.json")
