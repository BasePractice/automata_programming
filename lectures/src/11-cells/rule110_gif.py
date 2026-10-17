#!/usr/bin/env python3
"""Перерисовка анимации «One-d-cellular-automaton-rule-110.gif» в высоком разрешении.

Геометрия задаётся в единицах исходника (500×340), итог масштабируется в SCALE раз.
Запуск: python3 rule110_gif.py [SCALE]   (по умолчанию 4 → 2000×1360)
Результат: images/rule110-step.gif рядом со скриптом.
"""
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

SCALE = int(sys.argv[1]) if len(sys.argv) > 1 else 4
SS = 3                      # суперсэмплинг для сглаживания
K = SCALE * SS              # пикселей на единицу исходника
W, H = 500, 340

RULE = 110
INIT = "110100001001000100000100000011"   # начальная строка исходного GIF
N = len(INIT)

WHITE = (255, 255, 255, 255)
BLACK = (0, 0, 0, 255)
YELLOW = (255, 215, 0, 255)
GRID = (176, 166, 176, 255)
GRID_ON_YELLOW = (120, 96, 30, 255)
OUTLINE = (128, 128, 128, 255)
TEXT = (0, 0, 0, 255)

# Чертёжный шрифт по ГОСТ 2.304-81 — тот же osifont, что в надписях рисунков
# лекций (template/theme.typ); ставится `make fonts` в lectures/fonts/.
FONT_PATH = Path(__file__).resolve().parents[2] / "fonts" / "osifont-lgpl3fe.ttf"
font = ImageFont.truetype(str(FONT_PATH), int(13 * K))
font_label = ImageFont.truetype(str(FONT_PATH), int(13.5 * K))

TOP_Y, BOT_Y = 69, 289      # верх строк автомата
CELL = 15                   # шаг клетки
ROW_X = 25
RULE_Y = 182                # верх тройки в таблице правила
SQ = 11                     # размер квадратика в таблице правила


def s(*v):
    return [round(x * K) for x in v]


def rect(d, x0, y0, x1, y1, **kw):
    d.rectangle(s(x0, y0, x1, y1), **kw)


def text_c(d, cx, y, txt, f):
    d.text((cx * K, y * K), txt, font=f, fill=TEXT, anchor="mm")


def step(cells):
    return [(RULE >> (cells[(i - 1) % N] * 4 + cells[i] * 2 + cells[(i + 1) % N])) & 1
            for i in range(N)]


def draw_row(d, y, cells, known, hl=()):
    """Строка клеток. known — какие клетки уже определены; hl — подсвеченные индексы."""
    for i in hl:
        x = ROW_X + CELL * i
        rect(d, x - 3, y - 3, x + CELL + 3, y + CELL + 4, fill=YELLOW)
    lw = 1.6
    for i in range(N):
        x = ROW_X + CELL * i
        c = GRID_ON_YELLOW if i in hl else GRID
        # вертикальная граница слева от клетки
        rect(d, x, y, x + lw, y + CELL + 1, fill=c)
        rect(d, x, y, x + CELL, y + lw, fill=c)
        rect(d, x, y + CELL + 1 - lw, x + CELL, y + CELL + 1, fill=c)
        if known[i] and cells[i]:
            rect(d, x + 3, y + 3, x + CELL - 2, y + CELL - 2, fill=BLACK)
    x = ROW_X + CELL * N
    c = GRID_ON_YELLOW if (N - 1) in hl else GRID
    rect(d, x, y, x + lw, y + CELL + 1, fill=c)


def draw_square(d, x, y, v, outline=OUTLINE, w=1.0):
    if v:
        rect(d, x, y, x + SQ, y + SQ, fill=BLACK)
    else:
        rect(d, x, y, x + SQ, y + SQ, fill=outline)
        rect(d, x + w, y + w, x + SQ - w, y + SQ - w, fill=WHITE)


def draw_rule(d, active):
    text_c(d, 250, 135, f"правило {RULE} ({RULE:08b})", font)
    for j in range(8):
        pat = 7 - j
        cx = 40 + 60 * j
        text_c(d, cx, 166, str(pat), font_label)
        bits = [(pat >> 2) & 1, (pat >> 1) & 1, pat & 1]
        if pat == active:
            rect(d, cx - 24, RULE_Y - 4, cx + 24, RULE_Y + SQ + 2, fill=YELLOW)
        for b in range(3):
            x = cx - SQ / 2 - CELL + CELL * b
            draw_square(d, x, RULE_Y, bits[b],
                        outline=GRID_ON_YELLOW if pat == active else OUTLINE)
        out = (RULE >> pat) & 1
        ox, oy = cx - SQ / 2, RULE_Y + 15
        if pat == active:
            rect(d, ox - 1.5, oy - 1.5, ox + SQ + 1.5, oy + SQ + 1.5, fill=(150, 150, 150, 255))
            rect(d, ox - 0.5, oy - 0.5, ox + SQ + 0.5, oy + SQ + 0.5, fill=(90, 90, 90, 255))
            rect(d, ox + 0.5, oy + 0.5, ox + SQ - 0.5, oy + SQ - 0.5,
                 fill=BLACK if out else WHITE)
        else:
            draw_square(d, ox, oy, out)


def frame(cur, nxt, k):
    """Кадр k: вычисляется клетка k+1 следующего поколения по клеткам k, k+1, k+2."""
    img = Image.new("RGBA", (W * K, H * K), WHITE)
    d = ImageDraw.Draw(img)
    text_c(d, 250, 25, "текущее состояние автомата", font)
    text_c(d, 250, 245, "следующее поколение автомата", font)
    idx = [(k + o) % N for o in range(3)]
    draw_row(d, TOP_Y, cur, [True] * N, hl=idx)
    pat = cur[idx[0]] * 4 + cur[idx[1]] * 2 + cur[idx[2]]
    draw_rule(d, pat)
    known = [1 <= i <= k + 1 for i in range(N)]
    draw_row(d, BOT_Y, nxt, known, hl=(idx[1],))
    return img.resize((W * SCALE, H * SCALE), Image.LANCZOS).convert("RGB")


def main():
    cur = [int(c) for c in INIT]
    nxt = step(cur)
    frames = [frame(cur, nxt, k) for k in range(N - 1)]
    # общая палитра без дизеринга — чистые цвета и меньший размер файла
    pal = frames[len(frames) // 2].quantize(colors=128, method=Image.Quantize.FASTOCTREE,
                                            dither=Image.Dither.NONE)
    # квантователь округляет 255 → 254; возвращаем чистые белый/чёрный/жёлтый
    pal.putpalette([255 if v >= 252 else 0 if v <= 3 else v for v in pal.getpalette()])
    q = [f.quantize(palette=pal, dither=Image.Dither.NONE) for f in frames]
    out = Path(__file__).resolve().parent / "images" / "rule110-step.gif"
    durations = [1420 if k % 7 == 3 else 1430 for k in range(N - 1)]
    q[0].save(out, save_all=True, append_images=q[1:], duration=durations, loop=0,
              optimize=True, disposal=1)
    print(out)


if __name__ == "__main__":
    main()
