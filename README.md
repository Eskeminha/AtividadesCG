# Atividades de Computação Gráfica

Repositório com as entregas das atividades da disciplina de **Computação Gráfica** — Unisinos.

**Aluno:** Anderson Koefender  
**Linguagem:** C++  
**API Gráfica:** OpenGL 3.3+  (Feito no MacOS)

---

## Atividades

### ✅ Hello3D
Tarefa - Configuração do ambiente de desenvolvimento e execução do primeiro projeto OpenGL.

- Título da janela alterado para `Ola 3D -- Anderson Koefender!`
- Renderização de uma pirâmide 3D colorida com rotação nos eixos X, Y e Z

**Teclas:**
- `X` — rotaciona no eixo X
- `Y` — rotaciona no eixo Y
- `Z` — rotaciona no eixo Z
- `ESC` — fecha a janela

![Print da execução](print/print01.png)

---

### ✅ M2
Tarefa - Instanciando objetos na cena 3D.

- Pirâmide substituída por um cubo com 6 faces, cada uma com uma cor diferente
- 3 cubos instanciados simultaneamente na cena, cada um com transformações independentes
- Controle de rotação, translação e escala uniforme via teclado

**Teclas:**
- `1` / `2` / `3` — seleciona o cubo ativo
- `X` — rotaciona no eixo X
- `Y` — rotaciona no eixo Y
- `Z` — rotaciona no eixo Z
- `W` / `S` — translada no eixo Z
- `A` / `D` — translada no eixo X
- `I` / `J` — translada no eixo Y
- `[` — diminui a escala uniformemente
- `]` — aumenta a escala uniformemente
- `ESC` — fecha a janela

![Print da execução M2](print/print02.png)

---

### ✅ AV1
Vivencial 1 (09/05/26) - Selecionando e aplicando transformações em objetos 3D.

- Leitura de arquivos `.obj` e exibição de múltiplos modelos na cena
- Seleção de objetos via `TAB` (objeto selecionado aparece em destaque)
- Modos de transformação independentes por objeto: rotação, translação e escala

**Teclas:**
- `TAB` — seleciona o próximo objeto
- `R` — modo rotação → `X` / `Y` / `Z` escolhe o eixo
- `T` — modo translação → setas (X/Y) | `W` / `E` (Z)
- `S` — modo escala → `=` aumenta | `-` diminui
- `ESC` — fecha a janela

![Print da execução AV1](print/print03.png)