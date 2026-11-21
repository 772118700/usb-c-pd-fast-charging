#!/usr/bin/env python3
"""Draw a clean net-label breadboard wiring plan for the USB-C PD system.

Same-label tags are connected with one Dupont wire.  The 4 A power path is
shown separately and must NOT be routed through the breadboard.
"""
from PIL import Image, ImageDraw, ImageFont

S = 2
W, H = 2800, 2000
FONT = r"C:\Windows\Fonts\msyh.ttc"
FONT_B = r"C:\Windows\Fonts\msyhbd.ttc"

img = Image.new("RGB", (W * S, H * S), (247, 249, 252))
d = ImageDraw.Draw(img)

F_TITLE = ImageFont.truetype(FONT_B, 40 * S)
F_SUB = ImageFont.truetype(FONT, 18 * S)
F_H = ImageFont.truetype(FONT_B, 24 * S)
F_SECTION = ImageFont.truetype(FONT_B, 21 * S)
F_BODY = ImageFont.truetype(FONT, 17 * S)
F_SMALL = ImageFont.truetype(FONT, 15 * S)
F_TINY = ImageFont.truetype(FONT, 13 * S)
F_PIN = ImageFont.truetype(FONT, 13 * S)
F_TAG = ImageFont.truetype(FONT_B, 12 * S)

INK = (30, 38, 50)
MUTED = (92, 101, 113)
PWR_RED = (196, 42, 42)
PWR_BLK = (38, 38, 38)
V33 = (207, 48, 48)
V5 = (226, 122, 34)
GND = (43, 50, 52)
SCL = (38, 122, 180)
SDA = (34, 160, 90)
ADC = (176, 98, 28)
NTC = (136, 64, 166)
LED_C = (220, 170, 10)
CTRL = (115, 128, 140)
OPT = (160, 80, 80)
BOARD = (244, 241, 231)


def sc(v):
    return int(round(v * S))


def rr(x, y, w, h, r, fill, outline, width=2):
    d.rounded_rectangle([sc(x), sc(y), sc(x + w), sc(y + h)],
                        radius=sc(r), fill=fill, outline=outline,
                        width=max(1, int(width * S)))


def txt(x, y, s, font, fill=INK, anchor="la"):
    d.text((sc(x), sc(y)), s, font=font, fill=fill, anchor=anchor)


def pin_dot(x, y, color=(202, 158, 55)):
    d.rounded_rectangle([sc(x - 5), sc(y - 5), sc(x + 5), sc(y + 5)],
                        radius=sc(2), fill=color, outline=(110, 82, 26),
                        width=max(1, S))


def wire(points, color, width=3, dashed=False):
    pts = [(sc(x), sc(y)) for x, y in points]
    d.line([(x + 2, y + 3) for x, y in pts], fill=(216, 216, 216),
           width=max(1, int((width + 1) * S)), joint="curve")
    if not dashed:
        d.line(pts, fill=color, width=max(1, int(width * S)), joint="curve")
    else:
        for i in range(len(pts) - 1):
            x1, y1 = pts[i]
            x2, y2 = pts[i + 1]
            total = max(abs(x2 - x1), abs(y2 - y1))
            steps = max(1, int(total / (9 * S)))
            for k in range(steps):
                if k % 2 == 0:
                    xa = x1 + (x2 - x1) * k / steps
                    ya = y1 + (y2 - y1) * k / steps
                    xb = x1 + (x2 - x1) * min(k + 1, steps) / steps
                    yb = y1 + (y2 - y1) * min(k + 1, steps) / steps
                    d.line([(xa, ya), (xb, yb)], fill=color,
                           width=max(1, int(width * S)))


def dot(x, y, color, r=5):
    d.ellipse([sc(x - r), sc(y - r), sc(x + r), sc(y + r)],
              fill=color, outline=(255, 255, 255), width=max(1, S))


def tag(x, y, s, color, side="right"):
    bbox = d.textbbox((0, 0), s, font=F_TAG)
    w = (bbox[2] - bbox[0]) / S + 12
    h = (bbox[3] - bbox[1]) / S + 8
    if side == "right":
        x0, y0 = x, y - h / 2
    elif side == "left":
        x0, y0 = x - w, y - h / 2
    elif side == "up":
        x0, y0 = x - w / 2, y - h
    else:
        x0, y0 = x - w / 2, y
    rr(x0, y0, w, h, 5, (255, 255, 255), color, width=1)
    txt(x0 + 6, y0 + 3, s, F_TAG, color)
    return (x0, y0, w, h)


def stub_tag(px, py, net, color, dx=36, dy=0):
    """Short Dupont stub from a pin to a net tag."""
    wire([(px, py), (px + dx, py + dy)], color, 3)
    tag(px + dx + 4, py + dy, net, color, "right")


def resistor(x, y, w, h, name, color=ADC, vertical=True):
    if vertical:
        rr(x - 11, y, 22, h, 4, (241, 227, 205), color, width=2)
        txt(x, y + h / 2 - 8, name, F_TINY, color, anchor="ma")
    else:
        rr(x, y - 11, w, 22, 4, (241, 227, 205), color, width=2)
        txt(x + w / 2, y - 20, name, F_TINY, color, anchor="ma")


def cap(x, y):
    d.line([sc(x), sc(y - 9), sc(x), sc(y - 3)], fill=ADC, width=2 * S)
    d.line([sc(x - 10), sc(y - 3), sc(x + 10), sc(y - 3)], fill=ADC, width=2 * S)
    d.line([sc(x - 10), sc(y + 3), sc(x + 10), sc(y + 3)], fill=ADC, width=2 * S)
    d.line([sc(x), sc(y + 3), sc(x), sc(y + 9)], fill=ADC, width=2 * S)


def card(x, y, w, h, title, lines, color, fill):
    rr(x, y, w, h, 9, fill, color, width=2)
    txt(x + 13, y + 10, title, F_BODY, color)
    yy = y + 38
    for line in lines:
        txt(x + 13, yy, line, F_TINY, MUTED)
        yy += 20


# ============================================================================
# Header
# ============================================================================
txt(55, 28, "USB-C PD 智能快充系统 · 面包板接线图", F_TITLE, INK)
txt(57, 84, "STM32G071CBT6 + 2S 18650 + IP5389 30W 双向 PD  |  网络标号法  |  同名标签 = 一根杜邦线",
    F_SUB, MUTED)
txt(1980, 48, "4A 功率回路不经过面包板", F_H, (165, 55, 45))
txt(1980, 82, "18AWG / XT30 / 螺钉端子", F_SUB, (165, 55, 45))
d.line([sc(55), sc(120), sc(2070), sc(120)], fill=(208, 214, 222), width=2 * S)

# ============================================================================
# Power panel
# ============================================================================
rr(55, 140, 2010, 370, 12, (255, 249, 244), (198, 105, 72), width=2)
txt(75, 152, "① 功率回路（板外，不经过面包板）", F_SECTION, (160, 55, 45))
txt(75, 181, "电池 → F1 → BMS → INA226 分流 → PD 模块；MP1584 取 5V 供 MCU。粗线用 18AWG。",
    F_SMALL, MUTED)

# battery
rr(85, 220, 245, 250, 10, (235, 244, 255), (55, 105, 165), width=2)
txt(98, 230, "2S 18650", F_BODY, (40, 80, 130))
for yy in (270, 355):
    rr(105, yy, 85, 62, 10, (55, 115, 195), (25, 65, 125), width=2)
    txt(147, yy + 20, "3.7V", F_TINY, (255, 255, 255), anchor="ma")
txt(200, 285, "+", F_BODY, PWR_RED, anchor="ma")
txt(200, 370, "−", F_BODY, PWR_BLK, anchor="ma")
pin_dot(330, 260)
pin_dot(330, 340)
pin_dot(330, 430)
txt(342, 252, "B+", F_TINY, INK)
txt(342, 332, "BM", F_TINY, INK)
txt(342, 422, "B−", F_TINY, INK)
wire([(190, 332), (190, 355)], PWR_RED, 5)

# fuse
rr(372, 315, 70, 52, 6, (255, 235, 235), (175, 45, 45), width=2)
txt(407, 297, "F1 10A", F_TINY, (155, 45, 45), anchor="ma")
wire([(330, 260), (352, 260), (352, 341), (372, 341)], PWR_RED, 8)
wire([(442, 341), (480, 341), (480, 260)], PWR_RED, 8)

# BMS
rr(480, 220, 280, 250, 10, (238, 250, 240), (45, 115, 75), width=2)
txt(495, 230, "2S BMS 10A 同口", F_BODY, (30, 95, 55))
txt(495, 258, "B− / B1 / B+ 接电芯", F_TINY, MUTED)
txt(495, 278, "P− / P+ 接负载", F_TINY, MUTED)
pin_dot(480, 260)
pin_dot(480, 340)
pin_dot(480, 430)
pin_dot(760, 260)
pin_dot(760, 430)
txt(466, 252, "B+", F_TINY, INK, anchor="ra")
txt(466, 332, "B1", F_TINY, INK, anchor="ra")
txt(466, 422, "B−", F_TINY, INK, anchor="ra")
txt(746, 252, "P+", F_TINY, INK, anchor="ra")
txt(746, 422, "P−", F_TINY, INK, anchor="ra")
wire([(330, 340), (480, 340)], ADC, 3)
wire([(330, 430), (480, 430)], PWR_BLK, 8)

# INA
rr(800, 220, 290, 250, 10, (255, 251, 236), (175, 125, 40), width=2)
txt(815, 230, "INA226 + 10mΩ", F_BODY, (145, 90, 18))
txt(815, 258, "高侧双向电流/电压", F_TINY, MUTED)
txt(815, 278, "地址 0x40", F_TINY, MUTED)
pin_dot(800, 260)
pin_dot(1090, 260)
txt(790, 240, "IN−", F_TINY, INK, anchor="ra")
txt(1100, 240, "IN+", F_TINY, INK)
txt(945, 360, "逻辑侧", F_TINY, MUTED, anchor="ma")
txt(945, 380, "VCC/GND/SCL/SDA", F_TINY, MUTED, anchor="ma")
wire([(760, 260), (800, 260)], PWR_RED, 8)
wire([(1090, 260), (1130, 260)], PWR_RED, 8)
dot(760, 260, PWR_RED)
dot(1090, 260, PWR_RED)

# PD
rr(1130, 220, 320, 250, 10, (245, 240, 255), (90, 70, 145), width=2)
txt(1145, 230, "IP5389 PD 模块", F_BODY, (75, 55, 125))
txt(1145, 258, "2S / 30W / 双向 PD3.0", F_TINY, MUTED)
txt(1145, 278, "BAT+ / BAT− / USB-C", F_TINY, MUTED)
pin_dot(1130, 260)
pin_dot(1130, 430)
rr(1420, 305, 55, 45, 6, (55, 55, 65), (25, 25, 30), width=2)
txt(1448, 328, "C", F_BODY, (255, 255, 255), anchor="ma")
txt(1448, 288, "USB-C", F_TINY, MUTED, anchor="ma")
wire([(760, 430), (760, 485), (1130, 485), (1130, 430)], PWR_BLK, 8)

# MP1584
rr(1490, 235, 240, 205, 10, (255, 245, 235), (195, 115, 55), width=2)
txt(1505, 245, "MP1584 5V", F_BODY, (155, 75, 28))
txt(1505, 273, "8.4V → 5V / 3A", F_TINY, MUTED)
pin_dot(1490, 310)
pin_dot(1490, 400)
pin_dot(1730, 310)
pin_dot(1730, 400)
txt(1476, 302, "IN+", F_TINY, INK, anchor="ra")
txt(1476, 392, "IN−", F_TINY, INK, anchor="ra")
txt(1744, 302, "OUT+", F_TINY, INK)
txt(1744, 392, "OUT−", F_TINY, INK)
wire([(1130, 260), (1130, 200), (1490, 200), (1490, 310)], PWR_RED, 6)
dot(1130, 260, PWR_RED)
wire([(1130, 485), (1490, 485), (1490, 400)], PWR_BLK, 6)
dot(1130, 485, PWR_BLK)

# "到面包板" terminal tags: only 5V/GND; PACK/MID sense go directly from BMS/BM
rr(1800, 215, 195, 250, 10, (245, 247, 250), (110, 120, 135), width=2)
txt(1815, 225, "到面包板", F_BODY, (80, 90, 105))
tag(1815, 285, "5V", V5, "right")
tag(1815, 345, "GND", GND, "right")
txt(1815, 390, "5V/GND 接面包板轨", F_TINY, MUTED)
txt(1815, 410, "PACK/MID 从 BMS/BM", F_TINY, MUTED)
txt(1815, 428, "直接引到分压上端", F_TINY, MUTED)
wire([(1730, 310), (1775, 310), (1775, 285), (1815, 285)], V5, 4)
wire([(1730, 400), (1785, 400), (1785, 345), (1815, 345)], GND, 4)
# power sense tags at source (terminal block only; table maps source)

# ============================================================================
# Breadboard
# ============================================================================
rr(55, 560, 2010, 1400, 16, BOARD, (165, 158, 142), width=3)
txt(75, 572, "② 面包板（只走信号 / 3.3V / 5V 小电流）", F_SECTION, (75, 85, 95))
txt(75, 600, "同名标签之间用一根杜邦线互连；图中短线只把引脚引到标签。", F_SMALL, MUTED)

# rails
for y, col, name in ((640, V33, "3.3V"), (680, GND, "GND"),
                     (1600, V5, "5V"), (1640, GND, "GND")):
    d.line([sc(95), sc(y), sc(2000), sc(y)], fill=col, width=2 * S)
    for x in range(105, 2001, 22):
        d.ellipse([sc(x - 3), sc(y - 3), sc(x + 3), sc(y + 3)],
                  fill=(234, 230, 219), outline=col, width=max(1, S))
    txt(70, y - 8, name, F_SMALL, col, anchor="ra")

# grid holes
for c in range(63):
    x = 300 + c * 22
    for r in range(10):
        for yy in (820 + r * 22, 1060 + r * 22):
            d.ellipse([sc(x - 2), sc(yy - 2), sc(x + 2), sc(yy + 2)],
                      fill=(220, 215, 203), outline=(194, 188, 175), width=max(1, S))
for c in range(0, 63, 5):
    txt(300 + c * 22, 796, str(c + 1), F_TINY, MUTED, anchor="ma")
d.rectangle([sc(300), sc(1038), sc(1664), sc(1056)], fill=(224, 219, 207))

# GND rail jumper
wire([(2020, 680), (2020, 1640)], GND, 4)
dot(2020, 680, GND)
dot(2020, 1640, GND)

# ---------------------------------------------------------------------------
# MCU
# ---------------------------------------------------------------------------
rr(90, 750, 280, 760, 10, (27, 50, 82), (12, 30, 50), width=2)
txt(105, 762, "STM32G071CBT6", F_SECTION, (235, 242, 250))
txt(105, 792, "最小系统板", F_BODY, (198, 218, 238))
txt(105, 1450, "按板子丝印找引脚", F_TINY, (170, 195, 220))
for mx, my in ((108, 770), (345, 770), (108, 1480), (345, 1480)):
    d.ellipse([sc(mx - 5), sc(my - 5), sc(mx + 5), sc(my + 5)],
              fill=(145, 165, 185), outline=(75, 95, 115), width=max(1, S))

# top power pins / wires to rails
for x, name in ((140, "5V"), (190, "GND"), (240, "3V3")):
    pin_dot(x, 750)
    txt(x, 726, name, F_PIN, INK, anchor="ma")
wire([(140, 745), (75, 745), (75, 1600), (130, 1600)], V5, 4)
wire([(190, 745), (190, 680)], GND, 4)
wire([(240, 745), (240, 640)], V33, 4)
dot(190, 680, GND)
dot(240, 640, V33)

# right signal pins -> tag
signals = [
    ("PB6/SCL", 880, "SCL", SCL),
    ("PB7/SDA", 925, "SDA", SDA),
    ("PA0", 1000, "PACK", ADC),
    ("PA1", 1045, "MID", ADC),
    ("PA4", 1090, "NTC1", NTC),
    ("PA5", 1135, "NTC2", NTC),
    ("PA6", 1200, "LED", LED_C),
    ("PB0", 1245, "KEY", CTRL),
    ("PA7", 1290, "INH", OPT),
    ("PA2", 1350, "TX", CTRL),
    ("PA3", 1395, "RX", CTRL),
]
for name, yy, net, col in signals:
    pin_dot(370, yy)
    txt(360, yy - 8, name, F_PIN, (220, 232, 246), anchor="ra")
    stub_tag(370, yy, net, col, 34)

# ---------------------------------------------------------------------------
# OLED
# ---------------------------------------------------------------------------
rr(520, 780, 180, 160, 8, (19, 27, 37), (8, 13, 20), width=2)
d.rectangle([sc(532), sc(792), sc(688), sc(900)], fill=(7, 17, 29))
txt(610, 820, "OLED", F_SECTION, (110, 215, 250), anchor="ma")
txt(610, 848, "128×64", F_SMALL, (180, 220, 240), anchor="ma")
txt(610, 872, "0x3C", F_TINY, (180, 220, 240), anchor="ma")
for x, name, net, col, tx, ty in (
        (550, "GND", "GND", GND, 550, 990),
        (590, "VCC", "3V3", V33, 610, 990),
        (630, "SCL", "SCL", SCL, 550, 1025),
        (670, "SDA", "SDA", SDA, 610, 1025)):
    pin_dot(x, 940)
    txt(x, 946, name, F_TINY, (210, 225, 240), anchor="ma")
    tag(tx, ty, net, col, "down")

wire([(550, 940), (550, 990)], GND, 2)
wire([(590, 940), (590, 960), (610, 960), (610, 990)], V33, 2)
wire([(630, 940), (630, 970), (550, 970), (550, 1025)], SCL, 2)
wire([(670, 940), (670, 980), (610, 980), (610, 1025)], SDA, 2)

# ---------------------------------------------------------------------------
# INA logic header
# ---------------------------------------------------------------------------
rr(740, 780, 160, 120, 8, (255, 251, 236), (175, 125, 40), width=2)
txt(752, 790, "INA226 逻辑侧", F_BODY, (145, 90, 18))
txt(752, 814, "从模块引出", F_TINY, MUTED)
txt(752, 832, "IN+/IN− 在功率区", F_TINY, MUTED)
for x, name, net, col, tx, ty in (
        (770, "VCC", "3V3", V33, 770, 934),
        (805, "GND", "GND", GND, 830, 934),
        (840, "SCL", "SCL", SCL, 770, 969),
        (875, "SDA", "SDA", SDA, 830, 969)):
    pin_dot(x, 900)
    txt(x, 906, name, F_TINY, MUTED, anchor="ma")
    tag(tx, ty, net, col, "down")

wire([(770, 900), (770, 934)], V33, 2)
wire([(805, 900), (805, 916), (830, 916), (830, 934)], GND, 2)
wire([(840, 900), (840, 950), (770, 950), (770, 969)], SCL, 2)
wire([(875, 900), (875, 960), (830, 960), (830, 969)], SDA, 2)

# ---------------------------------------------------------------------------
# ADC dividers
# ---------------------------------------------------------------------------
rr(950, 800, 130, 320, 8, (255, 252, 242), (185, 145, 70), width=1)
txt(965, 808, "PACK 分压", F_BODY, ADC)
tag(1015, 782, "PACK_SENSE", ADC, "up")
pin_dot(1015, 800)
wire([(1015, 800), (1015, 850)], ADC, 3)
resistor(1015, 850, 22, 95, "100k", ADC, True)
dot(1015, 955, ADC)
resistor(1015, 960, 22, 95, "47k", ADC, True)
wire([(1015, 1055), (1015, 1080)], GND, 3)
tag(1015, 1094, "GND", GND, "down")
cap(1060, 955)
wire([(1015, 955), (1050, 955)], ADC, 2)
wire([(1060, 964), (1060, 995), (1015, 995)], ADC, 2)
wire([(1060, 995), (1060, 1030)], GND, 2)
tag(1060, 1044, "GND", GND, "down")
tag(930, 955, "PACK", ADC, "left")
wire([(1015, 955), (930, 955)], ADC, 2)

rr(1120, 800, 130, 320, 8, (255, 252, 242), (185, 145, 70), width=1)
txt(1135, 808, "MID 分压", F_BODY, ADC)
tag(1185, 782, "MID_SENSE", ADC, "up")
pin_dot(1185, 800)
wire([(1185, 800), (1185, 850)], ADC, 3)
resistor(1185, 850, 22, 100, "100k", ADC, True)
dot(1185, 960, ADC)
resistor(1185, 965, 22, 100, "47k", ADC, True)
wire([(1185, 1065), (1185, 1080)], GND, 3)
tag(1185, 1094, "GND", GND, "down")
cap(1230, 960)
wire([(1185, 960), (1220, 960)], ADC, 2)
wire([(1230, 969), (1230, 1000), (1185, 1000)], ADC, 2)
wire([(1230, 1000), (1230, 1030)], GND, 2)
tag(1230, 1044, "GND", GND, "down")
stub_tag(1185, 960, "MID", ADC, 70)

# ---------------------------------------------------------------------------
# NTC channels
# ---------------------------------------------------------------------------
rr(1320, 800, 140, 280, 8, (250, 242, 255), (140, 100, 170), width=1)
txt(1335, 808, "NTC1 电池", F_BODY, NTC)
tag(1360, 782, "3V3", V33, "up")
pin_dot(1360, 800)
wire([(1360, 800), (1360, 850)], NTC, 3)
resistor(1360, 850, 22, 80, "10k", NTC, True)
dot(1360, 940, NTC)
wire([(1360, 940), (1360, 1010)], NTC, 3)
pin_dot(1310, 1010)
pin_dot(1410, 1010)
txt(1296, 1002, "A", F_TINY, INK, anchor="ra")
txt(1424, 1002, "B", F_TINY, INK)
tag(1310, 1030, "NTC1_A", NTC, "left")
tag(1410, 1030, "NTC1_B", NTC, "right")
stub_tag(1360, 940, "NTC1", NTC, 70)
wire([(1410, 1010), (1410, 1050)], GND, 3)
tag(1410, 1064, "GND", GND, "right")
cap(1390, 940)

rr(1500, 850, 140, 280, 8, (250, 242, 255), (140, 100, 170), width=1)
txt(1515, 858, "NTC2 PD/MOS", F_BODY, NTC)
tag(1540, 832, "3V3", V33, "up")
pin_dot(1540, 850)
wire([(1540, 850), (1540, 900)], NTC, 3)
resistor(1540, 900, 22, 80, "10k", NTC, True)
dot(1540, 990, NTC)
wire([(1540, 990), (1540, 1060)], NTC, 3)
pin_dot(1490, 1060)
pin_dot(1590, 1060)
txt(1476, 1052, "A", F_TINY, INK, anchor="ra")
txt(1604, 1052, "B", F_TINY, INK)
tag(1490, 1080, "NTC2_A", NTC, "left")
tag(1590, 1080, "NTC2_B", NTC, "right")
stub_tag(1540, 990, "NTC2", NTC, 70)
wire([(1590, 1060), (1590, 1100)], GND, 3)
tag(1590, 1114, "GND", GND, "right")
cap(1570, 990)

# NTC probes (wire to NTC1_A/B tags; no long dashed line to avoid crossing cards)
for px, py, name in ((1740, 940, "NTC1 探头"), (1740, 1080, "NTC2 探头")):
    d.ellipse([sc(px - 14), sc(py + 10), sc(px + 14), sc(py + 40)],
              fill=(242, 232, 250), outline=NTC, width=2)
    txt(px, py - 6, name, F_TINY, NTC, anchor="ma")
    txt(px, py + 48, "接 NTC_A / NTC_B", F_TINY, MUTED, anchor="ma")

# ---------------------------------------------------------------------------
# LED / KEY
# ---------------------------------------------------------------------------
rr(520, 1180, 230, 120, 8, (255, 252, 238), (190, 160, 70), width=1)
txt(535, 1188, "LED 指示", F_BODY, (150, 110, 15))
tag(500, 1220, "LED", LED_C, "left")
pin_dot(520, 1220)
resistor(545, 1209, 55, 22, "1k", LED_C, False)
wire([(600, 1220), (615, 1220)], LED_C, 3)
d.polygon([(sc(615), sc(1208)), (sc(615), sc(1232)), (sc(645), sc(1220))],
          fill=(255, 220, 80), outline=(150, 110, 15))
d.line([sc(645), sc(1208), sc(645), sc(1232)], fill=(150, 110, 15), width=2 * S)
wire([(645, 1220), (680, 1220)], GND, 3)
tag(700, 1220, "GND", GND, "right")
txt(535, 1260, "PA6 → 1k → LED → GND", F_TINY, MUTED)

rr(520, 1330, 230, 120, 8, (245, 248, 250), (120, 135, 150), width=1)
txt(535, 1338, "按键（可选）", F_BODY, (80, 95, 110))
tag(500, 1370, "KEY", CTRL, "left")
pin_dot(520, 1370)
wire([(520, 1370), (560, 1370)], CTRL, 3)
rr(560, 1350, 50, 40, 6, (230, 235, 240), (100, 110, 120), width=2)
txt(585, 1370, "KEY", F_TINY, (55, 65, 75), anchor="ma")
wire([(610, 1370), (650, 1370)], GND, 3)
tag(670, 1370, "GND", GND, "right")
txt(535, 1410, "PB0 → 按键 → GND，内部上拉", F_TINY, MUTED)

# ---------------------------------------------------------------------------
# UART / inhibit headers
# ---------------------------------------------------------------------------
rr(900, 1180, 180, 150, 8, (245, 248, 252), (120, 135, 150), width=2)
txt(915, 1190, "UART2（可选）", F_BODY, (80, 95, 110))
txt(915, 1216, "PA2 TX / PA3 RX", F_TINY, MUTED)
for x, name in ((930, "GND"), (970, "TX"), (1010, "RX"), (1050, "5V")):
    pin_dot(x, 1300)
    txt(x, 1306, name, F_TINY, MUTED, anchor="ma")
txt(915, 1348, "TX/RX 接 MCU 的 TX/RX 标签；", F_TINY, MUTED)
txt(915, 1368, "GND/5V 接电源轨。", F_TINY, MUTED)

rr(1110, 1180, 180, 150, 8, (252, 245, 245), (170, 90, 90), width=2)
txt(1125, 1190, "INHIBIT（可选）", F_BODY, (150, 60, 60))
txt(1125, 1216, "PA7 → 负载开关 EN", F_TINY, MUTED)
for x, name, net, col in ((1150, "PA7", "INH", OPT),
                          (1210, "GND", "GND", GND)):
    pin_dot(x, 1300)
    txt(x, 1306, name, F_TINY, MUTED, anchor="ma")
    tag(x, 1325, net, col, "down")
txt(1135, 1350, "默认不接；无硬件 EN 时留空", F_TINY, OPT)

# I2C pull-up inset
rr(740, 1050, 180, 110, 8, (250, 252, 255), (80, 120, 160), width=1)
txt(752, 1058, "I2C 上拉", F_BODY, (40, 80, 130))
tag(752, 1092, "SCL", SCL, "left")
resistor(785, 1082, 55, 20, "4.7k", V33, False)
wire([(752, 1092), (785, 1092)], SCL, 2)
wire([(840, 1092), (852, 1092)], V33, 2)
tag(852, 1092, "3V3", V33, "right")
tag(752, 1130, "SDA", SDA, "left")
resistor(785, 1120, 55, 20, "4.7k", V33, False)
wire([(752, 1130), (785, 1130)], SDA, 2)
wire([(840, 1130), (852, 1130)], V33, 2)
tag(852, 1130, "3V3", V33, "right")

# wiring order note in the free lower-left area
txt(100, 1685, "接线顺序：① 先接 3.3V/GND 轨  →  ② OLED/INA 的 I2C  →  ③ ADC 分压/NTC  →  ④ 最后接功率回路  →  ⑤ 8.0V/0.3A 限流上电",
    F_SMALL, (60, 75, 90))
txt(100, 1715, "MP1584 的 5V/GND 接面包板下排 5V/GND 轨；PACK_SENSE / MID_SENSE 接分压电阻上端。",
    F_SMALL, (60, 75, 90))
txt(100, 1745, "3V3 = 3.3V；所有 3V3 / GND / 5V 标签分别接 3.3V / GND / 5V 轨。",
    F_SMALL, (60, 75, 90))

# ============================================================================
# Right legend
# ============================================================================
rr(2100, 140, 640, 1810, 12, (255, 255, 255), (185, 194, 205), width=2)
txt(2120, 155, "接线表 / 检查", F_SECTION, INK)
txt(2120, 190, "同名标签 = 一根杜邦线；短线只做引脚引出。", F_SMALL, MUTED)

txt(2120, 225, "网络表", F_BODY, MUTED)
rows = [
    ("PACK", "PA0", "PACK 分压中点"),
    ("MID", "PA1", "MID 分压中点"),
    ("NTC1", "PA4", "NTC1 节点"),
    ("NTC2", "PA5", "NTC2 节点"),
    ("SCL", "PB6", "OLED/INA SCL"),
    ("SDA", "PB7", "OLED/INA SDA"),
    ("LED", "PA6", "1k→LED→GND"),
    ("KEY", "PB0", "按键→GND"),
    ("INH", "PA7", "负载开关 EN"),
    ("TX/RX", "PA2/PA3", "UART 排针"),
    ("3V3", "STM32", "3.3V 轨"),
    ("GND", "BMS P−", "公共地"),
    ("5V", "MP1584", "5V 轨/STM32 5V"),
    ("PACK_SENSE", "BMS P+", "PACK 100k 上端"),
    ("MID_SENSE", "BM", "MID 100k 上端"),
]
for i, (net, src, dst) in enumerate(rows):
    y = 258 + i * 31
    txt(2125, y, net, F_SMALL, (40, 80, 130))
    txt(2225, y, src, F_SMALL, INK)
    txt(2310, y, dst, F_SMALL, INK)

txt(2120, 760, "功率回路", F_BODY, MUTED)
power = [
    "B+ → F1 → BMS B+",
    "BM → BMS B1",
    "B− → BMS B−",
    "BMS P+ → INA IN−（电池侧）",
    "INA IN+ → PD BAT+（负载侧）",
    "BMS P− → PD BAT− / MP IN− / GND",
    "PD BAT+ → MP IN+",
    "MP OUT+ → 5V，OUT− → GND",
]
for i, s in enumerate(power):
    txt(2125, 793 + i * 27, s, F_SMALL, INK)

txt(2120, 1040, "I2C 地址", F_BODY, MUTED)
txt(2125, 1073, "OLED 0x3C  |  INA226 0x40", F_SMALL, INK)
txt(2125, 1100, "4.7k 上拉到 3.3V", F_SMALL, INK)
txt(2125, 1127, "模块自带则可不重复", F_SMALL, MUTED)

txt(2120, 1170, "极性 / 校准", F_BODY, MUTED)
txt(2125, 1203, "INA IN+ 接 PD 侧、IN− 接 BMS 侧", F_SMALL, INK)
txt(2125, 1230, "充电为正；若丝印相反改 SIGN=-1", F_SMALL, INK)
txt(2125, 1257, "分流电阻必须换 10mΩ", F_SMALL, INK)

txt(2120, 1305, "安全提示", F_BODY, (165, 55, 45))
warn = [
    "1. 4A 主回路严禁走面包板；",
    "   用 18AWG / XT30 / 螺钉端子。",
    "2. 面包板只走信号和 <100mA 电源。",
    "3. BMS 采样线顺序 B−/B1/B+，",
    "   接反会损坏 BMS。",
    "4. 首次上电用限流电源 8.0V/0.3A。",
    "5. 共地：BMS P− 单点星地。",
    "6. 交叉无圆点 = 不连接。",
]
for i, s in enumerate(warn):
    txt(2125, 1340 + i * 27, s, F_SMALL, (120, 45, 45))

txt(2120, 1575, "线色", F_BODY, MUTED)
color_rows = [
    ("3.3V / 红", V33), ("5V / 橙", V5), ("GND / 黑", GND),
    ("SCL / 蓝", SCL), ("SDA / 绿", SDA), ("ADC / 棕", ADC),
    ("NTC / 紫", NTC), ("LED / 黄", LED_C), ("可选 / 虚线", OPT),
]
for i, (s, col) in enumerate(color_rows):
    x = 2125 + (i % 3) * 160
    y = 1608 + (i // 3) * 28
    d.line([sc(x), sc(y + 8), sc(x + 22), sc(y + 8)], fill=col, width=4 * S)
    txt(x + 28, y, s, F_TINY, INK)

txt(2120, 1740, "注：开发板排针顺序为示意，", F_TINY, MUTED)
txt(2120, 1765, "实际以板子丝印为准。", F_TINY, MUTED)
txt(2120, 1790, "IP5389 无状态接口时不接 I2C。", F_TINY, MUTED)
txt(2120, 1835, "网络标签法：先插标签，再逐根连。", F_TINY, (40, 80, 130))

# bottom note
txt(60, 1972, "说明：功率主回路在板外；面包板只走信号/3.3V/5V。所有 GND 共地，逻辑 3.3V。",
    F_SMALL, MUTED)

img = img.resize((W, H), Image.LANCZOS)
img.save(r"docs\面包板接线图.png")
print("saved docs/面包板接线图.png")
