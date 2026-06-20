# 危极环境监测终端 (Weiji Environment Monitor)

基于 TI MSPM0G3507 + ESP8266 的多传感器环境监测终端，通过 MQTT 协议上报光照、温度、气压数据至华为云 IoTDA 平台。

## 硬件平台

| 模块 | 型号 | 接口 |
|---|---|---|
| MCU | MSPM0G3507 (Cortex-M0+, 80MHz) | — |
| WiFi | ESP8266 (AT 固件) | UART2 (PB17/PB18) |
| 光照传感器 | OPT3001 兼容 (I2C: 0x47) | I2C1 (PB2/PB3) |
| 环境传感器 | BME280 / BMP280 (I2C: 0x77) | I2C1 同总线 |
| OLED 显示屏 | 128×64 SSD1306 (SPI 软件模拟) | PB1/4/7/8 |
| 4 位数码管 | 共阴动态扫描 (NTP 网络时钟) | PA13/12/8/25 + PB9/24/19/12/15 + PA28/31/24 |

## 功能特性

- **三合一传感器采集**：光照 (lux)、温度 (°C)、气压 (hPa)
- **OLED 实时显示**：传感器数值即时刷新
- **NTP 网络时钟**：通过 ESP8266 SNTP 协议获取时间，4 位数码管动态显示 HH.MM
- **华为云 IoTDA MQTT 上报**：每秒将传感器数据打包为 JSON 上报至华为云平台
- **双串口透明桥接**：PC ↔ MCU ↔ ESP8266，方便调试

## 开发环境

- **IDE**: TI Code Composer Studio (CCS) Theia 或 CCS Eclipse
- **SDK**: MSPM0 SDK v2.05.01.00
- **编译器**: TI Arm Clang Compiler (ti-cgt-armllvm)
- **配置工具**: SysConfig 1.24.0+

## 导入与编译

### 1. 克隆仓库

```bash
git clone https://github.com/LLL46/weiji.git
```

### 2. 在 CCS 中导入项目

1. 打开 CCS Theia (或 CCS Eclipse)
2. `Project` → `Import CCS Projects...`
3. 选择克隆下来的 `weiji` 目录
4. 项目 `2026.8266.EXP` 会自动识别，勾选后点击 `Finish`

### 3. 编译

- 右键项目 → `Build Project`
- 或快捷键 `Ctrl+B`

首次编译时 SysConfig 会自动根据 `.syscfg` 文件生成 `ti_msp_dl_config.c` / `ti_msp_dl_config.h`。

### 4. 烧录与运行

- 通过 XDS110 调试器连接 LP-MSPM0G3507 LaunchPad
- 右键项目 → `Debug As` → `Code Composer Debug Session`

## 引脚分配

| 外设 | 信号 | 引脚 | LaunchPad 位置 |
|---|---|---|---|
| UART0 (上位机) | TX / RX | PA10 / PA11 | J4_34 / J4_33 |
| UART2 (ESP8266) | TX / RX | PB17 / PB18 | J2_18 / J2_17 |
| I2C1 | SCL / SDA | PB2 / PB3 | J2_12 / J2_11 |
| OLED SPI | SCK/SDA/RST/DC | PB8/PB7/PB4/PB1 | J2_8 / J2_7 / J1_4 / J2_9 |
| 数码管位选 | DIG1~4 | PA13/12/8/25 | — |
| 数码管段选 | A~G+DP | PB9/24/19/12/15 + PA28/31/24 | — |
| LED | USER_LED1 | PA0 | J27_9 |
| 调试 | SWCLK/SWDIO | PA20/PA19 | XDS110 |

## 项目结构

```
2026.8266.EXP/
├── .ccsproject              # CCS 项目配置
├── .cproject                # CDT 编译配置
├── .project                 # Eclipse 项目描述
├── .settings/               # 编辑器设置
├── targetConfigs/           # 调试器配置 (.ccxml)
├── uart_echo_interrupts_standby.c   # 主程序 (main + 中断)
├── uart_echo_interrupts_standby.syscfg  # SysConfig 引脚/外设配置
├── 8266cmd/
│   ├── command.c            # ESP8266 AT 指令常量
│   └── command.h
├── IICmodels/
│   ├── sensors.c            # BME280/BH1750 传感器驱动
│   └── sensors.h
└── LQ12864.c / LQ12864.h    # OLED 12864 驱动 (SPI 软件模拟)
```

## 数据上报格式

每秒上报一条 JSON 至华为云 IoTDA 设备影子：

```json
{
  "services": [{
    "service_id": "Environment",
    "properties": {
      "Light": 123.45,
      "temperature": 25.67,
      "pressure": 1013.25
    }
  }]
}
```

## 注意事项

- ESP8266 需刷入支持 MQTT AT 指令的固件（如安信可 AT v2.2.0+）
- WiFi SSID/密码 和 MQTT 密钥当前硬编码在 `8266cmd/command.c` 中，发布前请替换或使用配置文件
- 环境传感器地址为 0x77，上电后需约 2ms 初始化时间
- 数码管段 A~E 在 GPIOB，段 F/G/DP 在 GPIOA，硬件接线时注意跨端口
