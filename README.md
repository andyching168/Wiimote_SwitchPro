# Wiimote to Nintendo Switch Pro Controller 🎮

將 Nintendo Wii 的 Wiimote 搖桿轉換為 Nintendo Switch Pro Controller 的完整解決方案。使用雙 ESP32 架構，透過 I2C 串列通訊實現高效能、低延遲的控制器轉換。

![Project Overview](https://img.shields.io/badge/Platform-ESP32-blue) ![Status](https://img.shields.io/badge/Status-Active-green) ![License](https://img.shields.io/badge/License-MIT-yellow)

## 🌟 主要特色

- **雙 ESP32 架構**: ESP32-S1 負責 Wiimote 接收，ESP32-S3 負責 Switch 控制器模擬
- **高效能串列通訊**: 50Hz 更新率確保流暢的遊戲體驗
- **WiFi 熱點控制**: 透過網頁介面即時切換控制模式
- **8方向類比搖桿**: 支援精確的對角線移動控制
- **強制門戶功能**: 跨平台自動設定頁面
- **可自訂按鈕映射**: 完全可配置的按鈕對應關係

## 🏗️ 系統架構

```
┌─────────────────┐    Serial2    ┌─────────────────┐    USB-C    ┌─────────────────┐
│   ESP32-S1      │   (50Hz)      │   ESP32-S3      │  (HID)      │ Nintendo Switch │
│                 │  ────────────>│                 │ ──────────> │                 │
│ • Wiimote 接收  │   I2C 通訊    │ • Switch 控制器 │   模擬      │ • 遊戲主機      │
│ • 按鈕狀態解析  │               │ • WiFi 熱點     │             │ • 控制器輸入    │
│ • 連續輪詢模式  │               │ • 網頁控制介面  │             │                 │
└─────────────────┘               └─────────────────┘             └─────────────────┘
```

## 📦 專案結構

```
Wiimote_SwitchPro/
├── README.md                    # 本檔案
├── SwitchPro_i2c/              # ESP32-S3 PlatformIO 專案 (主要版本)
│   ├── platformio.ini          # S3 專案配置
│   ├── src/main.cpp            # S3 主程式
│   ├── src/WiimoteData.h       # 共享資料結構
│   ├── lib/switch_ESP32/       # Switch 控制器函式庫
│   ├── WiFi_Control_Guide.md   # WiFi 控制功能說明
│   └── 8_Way_Analog_Guide.md   # 8方向搖桿說明
└── WiiMote_i2c/                # ESP32-S1 PlatformIO 專案
    ├── platformio.ini          # S1 專案配置
    ├── src/main.cpp            # S1 主程式
    ├── src/WiimoteData.h       # 共享資料結構
    └── lib/ESP32Wiimote/       # Wiimote 通訊函式庫
```

## 🛠️ 硬體需求

### ESP32-S1 (Wiimote 接收端)
- ESP32 開發板 (支援傳統藍芽)
- GPIO 17/18 用於 Serial2 通訊

### ESP32-S3 (Switch 控制器端)
- ESP32-S3 開發板 (支援 USB-OTG)
- USB-C 埠連接到 Switch 主機/底座
- GPIO 17/18 用於 Serial2 通訊
- 與 S1 共地連接

### 連接方式
```
ESP32-S1          ESP32-S3
GPIO 17 (TX2) --> GPIO 18 (RX2)
GPIO 18 (RX2) --> GPIO 17 (TX2)
GND          --> GND
```

## 🚀 快速開始

### 1. 環境準備
```bash
# 安裝 PlatformIO Core
pip install platformio

# 或使用 VS Code 的 PlatformIO 擴充功能
```

### 2. 編譯並上傳

#### ESP32-S1 (Wiimote 接收)
```bash
cd WiiMote_i2c
pio run --target upload --upload-port COM6
```

#### ESP32-S3 (Switch 控制器)
```bash
cd SwitchPro_i2c
pio run --target upload --upload-port COM13
```

### 3. 配對 Wiimote
1. 啟動 ESP32-S1
2. 同時按住 Wiimote 的 1 + 2 按鈕
3. 等待藍牙連接成功

### 4. 連接 Switch
1. 將 ESP32-S3 透過 USB-C 連接到 Switch
2. Switch 會自動識別為 Pro Controller

## 🎯 控制模式

### 方向鍵模式 (預設)
Wiimote 方向鍵直接對應 Switch 的數位方向鍵 (D-Pad)

### 8方向類比搖桿模式
Wiimote 方向鍵對應左類比搖桿，支援8個方向的精確控制：

| Wiimote 按鈕 | Switch 搖桿方向 | 座標 (X,Y) |
|-------------|----------------|-----------|
| UP          | 上方           | (128, 0)  |
| DOWN        | 下方           | (128, 255)|
| LEFT        | 左方           | (0, 128)  |
| RIGHT       | 右方           | (255, 128)|
| UP+LEFT     | 左上對角線      | (0, 0)    |
| UP+RIGHT    | 右上對角線      | (255, 0)  |
| DOWN+LEFT   | 左下對角線      | (0, 255)  |
| DOWN+RIGHT  | 右下對角線      | (255, 255)|

## 📱 WiFi 控制介面

### 連接設定
- **WiFi 熱點**: `WiimoteController`
- **密碼**: `12345678`
- **設定頁面**: 自動彈出或前往 `http://192.168.4.1`

### 功能特色
- **強制門戶**: 跨平台自動跳轉設定頁面
- **即時切換**: 線上切換控制模式
- **狀態監控**: 即時顯示當前配置

## 🎮 按鈕映射

### 預設按鈕配置
| Wiimote 按鈕 | Switch 按鈕 | 功能 |
|-------------|------------|------|
| 2           | A          | 確認 |
| 1           | B          | 取消 |
| A           | L (左肩鍵)  | 左觸發 |
| B           | R (右肩鍵)  | 右觸發 |
| +           | +          | 選單 |
| -           | -          | 選項 |
| Home        | Home       | 主頁 |
| Z (Nunchuk) | X          | 額外功能 |
| C (Nunchuk) | Y          | 額外功能 |

### 自訂映射
可透過修改 `SwitchPro_i2c/src/main.cpp` 中的 `buttonMappings` 陣列來自訂按鈕配置。

## 🔧 技術細節

### 通訊協定
- **頻率**: 50Hz (20ms 間隔)
- **資料結構**: 16-bit 按鈕狀態
- **通訊方式**: Serial2 UART

### 核心函式庫
- **ESP32Wiimote**: Wiimote 藍牙通訊
- **switch-ESP32**: Nintendo Switch HID 模擬

### 效能指標
- **延遲**: < 20ms
- **更新率**: 50Hz
- **藍牙範圍**: 標準 Wiimote 範圍 (~10m)

## 📋 故障排除

### 常見問題
1. **Wiimote 無法連接**
   - 檢查藍牙模組是否正常
   - 確認按鈕配對順序 (1+2 同時按)

2. **Switch 無法識別**
   - 確認 ESP32-S3 支援 USB-OTG
   - 檢查 USB-C 連接線品質

3. **控制延遲**
   - 檢查 Serial2 連接線長度
   - 確認共地連接良好

### 除錯模式
啟用序列監視器 (115200 baud) 查看詳細狀態訊息。

## 🤝 貢獻

歡迎提交 Issue 和 Pull Request！

### 開發指南
1. Fork 專案
2. 建立功能分支
3. 提交變更
4. 建立 Pull Request

## 📄 授權

本專案採用 MIT 授權條款。詳細內容請參閱各子專案的 LICENSE 檔案。

## 🙏 致謝

- [ESP32Wiimote](https://github.com/bigw00d/ESP32Wiimote) - Wiimote 通訊函式庫
- [switch-ESP32](https://github.com/rachman-muti/switch-ESP32) - Nintendo Switch 控制器模擬

## 📞 聯絡方式

如有任何問題或建議，請透過 GitHub Issues 聯絡。

---

⭐ 如果這個專案對您有幫助，請給我們一個星星！
