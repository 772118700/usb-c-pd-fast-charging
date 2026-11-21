#!/usr/bin/env python3
"""Draw the software architecture diagram used by the design document."""
from PIL import Image, ImageDraw, ImageFont

W, H = 1600, 1060
FONT = r"C:\Windows\Fonts\msyh.ttc"
FONT_B = r"C:\Windows\Fonts\msyhbd.ttc"

img = Image.new("RGB", (W, H), (247, 249, 252))
d = ImageDraw.Draw(img)

f_title = ImageFont.truetype(FONT_B, 34)
f_sub = ImageFont.truetype(FONT, 18)
f_box = ImageFont.truetype(FONT_B, 19)
f_txt = ImageFont.truetype(FONT, 16)
f_small = ImageFont.truetype(FONT, 14)

BLUE = (39, 92, 160)
GREEN = (33, 122, 92)
ORANGE = (176, 96, 30)
PURPLE = (95, 73, 148)
GRAY = (90, 99, 112)


def box(x, y, w, h, title, lines, color, fill):
    d.rounded_rectangle([x, y, x + w, y + h], radius=12, fill=fill,
                        outline=color, width=2)
    d.text((x + 14, y + 9), title, font=f_box, fill=color)
    yy = y + 38
    for line in lines:
        d.text((x + 14, yy), line, font=f_txt, fill=(45, 52, 62))
        yy += 23


def arrow(x1, y1, x2, y2, color=GRAY, dash=False):
    if not dash:
        d.line([x1, y1, x2, y2], fill=color, width=3)
    else:
        # simple dashed line
        total = max(abs(x2 - x1), abs(y2 - y1))
        steps = max(total // 12, 1)
        for i in range(steps):
            if i % 2 == 0:
                xa = x1 + (x2 - x1) * i / steps
                ya = y1 + (y2 - y1) * i / steps
                xb = x1 + (x2 - x1) * (i + 1) / steps
                yb = y1 + (y2 - y1) * (i + 1) / steps
                d.line([xa, ya, xb, yb], fill=color, width=2)
    # arrow head
    if x2 == x1:
        d.polygon([(x2, y2), (x2 - 6, y2 - 12), (x2 + 6, y2 - 12)], fill=color)
    else:
        d.polygon([(x2, y2), (x2 - 12, y2 - 6), (x2 - 12, y2 + 6)], fill=color)


d.text((50, 30), "USB-C PD 智能快充系统 · 软件架构", font=f_title, fill=(28, 37, 49))
d.text((52, 78), "STM32G071CBT6 + FreeRTOS  |  采集 / 故障 / 显示 三任务  |  PD 只读监视",
       font=f_sub, fill=GRAY)
d.line([50, 112, 1550, 112], fill=(210, 216, 224), width=2)

# Input layer
d.text((50, 135), "感知层", font=f_box, fill=BLUE)
box(50, 168, 330, 120, "2S 电池组 + 分压网络",
    ["PACK 总压 / MID 中点", "100k / 47k, ADC1_IN0/1", "单体电压、压差"], BLUE, (235, 243, 253))
box(400, 168, 330, 120, "NTC 10K B3950 ×2",
    ["电池 NTC: ADC1_IN4", "PD/MOS NTC: ADC1_IN5", "开路/短路诊断"], BLUE, (235, 243, 253))
box(750, 168, 330, 120, "INA226 + 10 mΩ",
    ["I2C 0x40, 16 bit", "双向电流 / 母线电压", "功率 = V × I"], BLUE, (235, 243, 253))
box(1100, 168, 450, 120, "IP5389 PD PCBA (黑盒)",
    ["PD3.0 / PPS / QC 协商由 PCBA 完成", "可选 I2C 状态块: 0x2E", "默认不读取, 绝不下发控制"], BLUE, (235, 243, 253))

# Driver layer
d.text((50, 315), "驱动层", font=f_box, fill=GREEN)
box(50, 348, 460, 130, "bsp_adc / bsp_i2c",
    ["ADC1: 8 次采样, 去极值平均, 160.5 cycles", "I2C1 400 kHz, 互斥量保护", "超时 20 ms, 失败置 invalid"], GREEN, (234, 246, 240))
box(540, 348, 460, 130, "ina226 / ssd1306",
    ["INA226 配置 0x4527, 校准 2048", "SSD1306 128×64 帧缓存 + 5×7 字库", "逐页刷新, 1 KB / 次"], GREEN, (234, 246, 240))
box(1030, 348, 520, 130, "board_port",
    ["HAL_GetTick / GPIO / 临界区", "LED: PA6, inhibit: PA7 (可选), KEY: PB0", "FreeRTOS 适配层"], GREEN, (234, 246, 240))

# Service layer
d.text((50, 505), "服务层", font=f_box, fill=ORANGE)
box(50, 538, 280, 150, "SOC 算法",
    ["OCV 查表播种", "mA·ms 安时积分", "静置 30 min 慢修正", "容量 2S1P/2S2P 可配"], ORANGE, (253, 245, 232))
box(350, 538, 280, 150, "温度保护",
    ["充/放电温度窗口", "回差 3 degC", "PD NTC 85 degC 跳闸", "告警 / 闭锁"], ORANGE, (253, 245, 232))
box(650, 538, 280, 150, "均衡监测",
    ["两节压差", "50 mV 告警", "100 mV 严重", "3 s 去抖, 不控制 BMS"], ORANGE, (253, 245, 232))
box(950, 538, 280, 150, "故障诊断",
    ["OV/UV/OC 锁存", "TEMP 自动恢复", "12 个故障位", "inhibit 请求"], ORANGE, (253, 245, 232))
box(1250, 538, 300, 150, "PD 状态管理",
    ["只读状态块 v1", "magic/checksum 校验", "2 s 超时", "无接口则显示 N/A"], ORANGE, (253, 245, 232))

# App / RTOS layer
d.text((50, 715), "应用 / RTOS 层", font=f_box, fill=PURPLE)
box(50, 748, 360, 120, "采集任务  acq  (prio 4)",
    ["100 ms: ADC / INA226 / NTC", "SOC / 热保护 / 压差 / 故障", "500 ms: 可选 PD 轮询"], PURPLE, (242, 238, 250))
box(430, 748, 360, 120, "故障任务  fault  (prio 5)",
    ["事件通知 + 200 ms 兜底", "LED 闪烁 / 可选 inhibit", "长按 2 s 清锁存"], PURPLE, (242, 238, 250))
box(810, 748, 360, 120, "显示任务  disp  (prio 2)",
    ["500 ms 刷新, 3 页轮显", "主数据 / PD / 故障", "OLED 失败 10 s 重试"], PURPLE, (242, 238, 250))
box(1190, 748, 360, 120, "系统快照 app_state",
    ["临界区整结构拷贝", "避免 32 位字段撕裂", "故障/显示只读快照"], PURPLE, (242, 238, 250))

# Outputs
d.text((50, 895), "执行 / 人机", font=f_box, fill=GRAY)
box(50, 925, 360, 65, "OLED 128×64", ["电压/电流/SOC/温度/PD/故障"], GRAY, (241, 243, 246))
box(430, 925, 360, 65, "故障 LED", ["常亮=inhibit, 闪烁=告警"], GRAY, (241, 243, 246))
box(810, 925, 360, 65, "PD inhibit (可选)", ["需外加负载开关/使能脚"], GRAY, (241, 243, 246))
box(1190, 925, 360, 65, "UART2 (预留)", ["PA2/PA3, 当前未接日志"], GRAY, (241, 243, 246))

# arrows: input -> driver
for x in (215, 565, 915, 1325):
    arrow(x, 288, x, 348)
# driver -> service
for x in (280, 780, 1280):
    arrow(x, 478, x, 538)
# service -> app
for x in (190, 490, 790, 1090, 1390):
    arrow(x, 688, x, 748)
# app state -> display (bidirectional-ish)
arrow(1190, 808, 1170, 808, PURPLE)
# tasks -> outputs
arrow(230, 868, 230, 925)
arrow(610, 868, 610, 925)
arrow(990, 868, 990, 925)
arrow(1370, 868, 1370, 925)

# notes
d.rounded_rectangle([50, 1040, 1550, 1046], radius=4, fill=(230, 234, 240))
d.text((52, 1008), "安全边界: BMS 是锂电主保护; MCU 负责监督、显示与可选 inhibit。PD 协商由 IP5389 完成。",
       font=f_small, fill=(150, 60, 50))

img.save(r"docs\软件架构.png")
print("saved docs/软件架构.png")



