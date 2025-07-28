/*
 * ESP32-S3: Wiimote to Nintendo Switch Controller
 * 
 * 1. 透過 Serial2 從 ESP32-S1 接收按鈕資料。
 * 2. 將 Wiimote 按鈕狀態映射為 NS Gamepad 的輸入。
 * 3. 透過 USB 將自己模擬成一個 Switch 控制器。
 * 
 * 硬體需求:
 * - ESP32-S3 開發板
 * - 使用支援 USB-OTG 的 USB-C 埠連接到 Switch 主機/底座
 * - Serial2 (GPIO 18/17) 連接到 S1 的 Serial2
 * - GND 與 S1 共地
 * 
 * 函式庫需求:
 * - switch-esp32 by rachman-muti
 */

#include <Arduino.h>
#include "switch_ESP32.h"  // Switch 控制器函式庫
#include "WiimoteData.h"      // 我們的共享資料結構

// --- Serial2 設定 ---
#define TX2_PIN 17
#define RX2_PIN 18

// --- 建立 Switch Gamepad 物件 ---
NSGamepad Gamepad;

void setup() {
    // 開啟 USB 功能，這是模擬控制器的關鍵
    USB.begin(); 
    
    // 初始化 Gamepad
    Gamepad.begin();
    
    // 初始化 Serial，用於除錯輸出 (可選，但建議保留)
    Serial.begin(115200);
    Serial.println("ESP32-S3 NS Controller Initializing...");

    // 初始化 Serial2，用於接收來自 S1 的資料
    Serial2.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);

    Serial.println("Ready. Connect to Switch and waiting for button data...");
}

void loop() {
    // 檢查是否有來自 S1 的完整封包
    if (Serial2.available() >= sizeof(ButtonPacket)) {
        ButtonPacket received_packet;
        Serial2.readBytes((char*)&received_packet, sizeof(received_packet));
        uint16_t buttons = received_packet.buttonState;

        // --- 開始映射 ---

        // 1. **預設搖桿在中心位置**
        // 我們先宣告兩個變數來儲存最終要設定的搖桿值
        uint8_t finalXAxis = 128;
        uint8_t finalYAxis = 128;

        // 2. 根據按下的方向鍵，計算最終的搖桿位置
        // Wiimote UP    -> NS LEFT
        if (buttons & BUTTON_UP)    finalXAxis = 0;
        // Wiimote DOWN  -> NS RIGHT
        if (buttons & BUTTON_DOWN)  finalXAxis = 255;
        // Wiimote LEFT  -> NS DOWN
        if (buttons & BUTTON_LEFT)  finalYAxis = 255;
        // Wiimote RIGHT -> NS UP
        if (buttons & BUTTON_RIGHT) finalYAxis = 0;
        
        // 處理對角線情況：如果同時按下，則維持中心點
        // (這可以防止搖桿在兩個相反方向間快速抖動)
        if ((buttons & BUTTON_UP) && (buttons & BUTTON_DOWN)) {
            finalXAxis = 128;
        }
        if ((buttons & BUTTON_LEFT) && (buttons & BUTTON_RIGHT)) {
            finalYAxis = 128;
        }

        // 3. **一次性設定最終的搖桿位置**
        Gamepad.leftXAxis(finalXAxis);
        Gamepad.leftYAxis(finalYAxis);
        Gamepad.rightXAxis(128); // 右搖桿保持中心

        // 4. 清除所有按鈕，然後根據收到的狀態重新按下
        Gamepad.releaseAll();

        // 主要按鈕映射
        if (buttons & BUTTON_TWO)   Gamepad.press(NSButton_A);
        if (buttons & BUTTON_ONE)   Gamepad.press(NSButton_B);
        if (buttons & BUTTON_A)     Gamepad.press(NSButton_LeftTrigger);
        if (buttons & BUTTON_B)     Gamepad.press(NSButton_RightTrigger);

        // 功能鍵映射
        if (buttons & BUTTON_PLUS)  Gamepad.press(NSButton_Plus);
        if (buttons & BUTTON_MINUS) Gamepad.press(NSButton_Minus);
        if (buttons & BUTTON_HOME)  Gamepad.press(NSButton_Home);

        // 可選按鈕映射
        if (buttons & BUTTON_Z)     Gamepad.press(NSButton_X);
        if (buttons & BUTTON_C)     Gamepad.press(NSButton_Y);

        // 5. 將所有設定好的狀態透過 USB 發送給 Switch
        Gamepad.loop();
    }
}