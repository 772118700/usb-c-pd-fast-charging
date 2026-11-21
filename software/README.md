# USB-C PD 智能快充系统 · 软件工程

面向 **STM32G071CBT6 + FreeRTOS + 2S 18650 + IP5389 30 W 双向 PD** 的软件实现。
应用层与硬件解耦，可在 PC 上编译并运行 221 项主机测试；目标板移植层基于
STM32G0 HAL，配合 CubeMX 生成的工程使用。

> **安全边界**：BMS 是电芯的主保护，IP5389 是 PD 协商与功率级的主控。MCU 只做
> 采集、SOC、温度/压差监测、故障诊断、显示和可选的 inhibit 请求，不替代 BMS，
> 也不写 PD 模块。详见 [软件设计说明](../软件设计说明.md)。

## 1. 目录结构

```
software/
├── include/                 # 公共头文件、FreeRTOSConfig.h
├── src/
│   ├── services/            # 纯 C 算法: SOC/NTC/热保护/均衡/故障/PD/显示模型
│   ├── drivers/             # INA226、SSD1306 + 5x7 字库
│   ├── app/                 # 三任务、状态快照、应用入口
│   └── ...
├── platform/stm32g0/        # STM32G071 HAL 适配: ADC/I2C/GPIO
├── tests/                   # 主机测试、I2C mock、FreeRTOS 语法桩
├── Makefile
└── build/                   # 测试输出 (make clean 可删)
```

## 2. 主机测试

要求：GCC + GNU Make（Windows 下 MinGW 可用）。

```bash
cd software
make test       # 编译并运行 221 项检查
make syntax     # 用 FreeRTOS 桩对 app_tasks/app_main 做语法检查
make syntax-platform  # 用 HAL 桩检查 STM32G0 适配层
make clean
```

最近一次结果：

```
checks: 221, failures: 0
RESULT: PASS
```

测试覆盖：分压换算、NTC Beta、SOC 安时积分/OCV 修正、INA226 校准与正反向电流、
温度回差、压差去抖、PD 状态帧/校验/超时、故障锁存与复位、OLED 页面格式、
30 W 充放电场景、过压/过温联动。

## 3. 目标板集成

1. 用 STM32CubeMX 建立 STM32G071CBT6 工程，配置 ADC1、I2C1、USART2、GPIO 和
   FreeRTOS；具体引脚与参数见 [软件集成与接线](../软件集成与接线.md)。
2. 把 `src/`、`platform/stm32g0/` 加入工程，`include/` 加入头文件路径。
3. 用本工程的 `include/FreeRTOSConfig.h` 覆盖 CubeMX 生成的同名文件（或逐项比对）。
4. 在 CubeMX 的 `MX_*_Init()` 之后调用 `app_main()`。
5. 按文档完成分流电阻、分压电阻、NTC 和电流方向的校准。

> 本仓库不附带 STM32 HAL/FreeRTOS 内核源码（它们由 CubeMX 生成）。应用层与主机
> 测试不依赖 HAL/FreeRTOS；目标板的 `platform/stm32g0` 需要 HAL 头文件与工程。

## 4. 关键参数

| 项目 | 默认值 | 位置 |
|------|--------|------|
| 电池 | 2S1P 2500 mAh，30 W | `app_config.h` |
| 分压 | 100 kΩ / 47 kΩ | `app_config.h` |
| INA226 | 0x40，10 mΩ，±8.192 A，LSB 250 µA | `app_config.h` |
| OLED | 0x3C，SSD1306/SSD1315 | `app_config.h` |
| 采样/显示 | 100 ms / 500 ms | `app_config.h` |
| 故障锁存 | OV/UV/OC 锁存；TEMP 自动恢复 | `fault_diag.c` |
| PD | 默认 NONE；可选 I2C 只读状态块 | `pd_status.c` |

## 5. 已知限制

- IP5389 模块通常不开放 PD 协商状态；默认显示 `PD N/A (BLACK BOX)`，不控制 PDO/PPS。
- SOC 不做断电持久化，每次上电由 OCV 播种；带载上电会有误差。
- 压差只监测，不主动均衡；均衡由 2S BMS 完成。
- 目标板 HIL（真实 ADC/I2C/OLED/PD）尚未执行，测试清单见
  [测试报告](../测试报告.md)。


