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

// --- 方向鍵模式設定 ---
bool directionalButtonMode = true;  // true: 方向鍵作為數位按鈕, false: 方向鍵作為左類比搖桿

// --- 按鈕映射配置表 ---
struct ButtonMapping {
    uint16_t wiimoteButton;
    uint8_t nsButton;
};

// 可編輯的按鈕映射表
const ButtonMapping buttonMappings[] = {
    // 主要按鈕映射
    {BUTTON_TWO,   NSButton_A},
    {BUTTON_ONE,   NSButton_B},
    {BUTTON_A,     NSButton_LeftTrigger},
    {BUTTON_B,     NSButton_RightTrigger},
    
    // 功能鍵映射
    {BUTTON_PLUS,  NSButton_Plus},
    {BUTTON_MINUS, NSButton_Minus},
    {BUTTON_HOME,  NSButton_Home},
    
    // 可選按鈕映射
    {BUTTON_Z,     NSButton_X},
    {BUTTON_C,     NSButton_Y}
};

// 方向鍵到數位按鈕的映射配置(沒用到)
const ButtonMapping directionalButtonMappings[] = {
    {BUTTON_UP,    NSButton_Y},     // 暫時映射到其他按鈕，或者移除這個陣列
    {BUTTON_DOWN,  NSButton_B},
    {BUTTON_LEFT,  NSButton_X},
    {BUTTON_RIGHT, NSButton_A}
};

// 方向鍵到搖桿的映射配置
struct DirectionalMapping {
    uint16_t wiimoteButton;
    uint8_t xAxisValue;
    uint8_t yAxisValue;
    const char* description;
};

// 可編輯的方向鍵映射表
const DirectionalMapping directionalMappings[] = {
    {BUTTON_UP,    0,   128, "Wiimote UP -> NS LEFT"},
    {BUTTON_DOWN,  255, 128, "Wiimote DOWN -> NS RIGHT"},
    {BUTTON_LEFT,  128, 255, "Wiimote LEFT -> NS DOWN"},
    {BUTTON_RIGHT, 128, 0,   "Wiimote RIGHT -> NS UP"}
};

// 取得映射表大小
const size_t buttonMappingsCount = sizeof(buttonMappings) / sizeof(buttonMappings[0]);
const size_t directionalMappingsCount = sizeof(directionalMappings) / sizeof(directionalMappings[0]);
const size_t directionalButtonMappingsCount = sizeof(directionalButtonMappings) / sizeof(directionalButtonMappings[0]);

/**
 * 處理方向鍵的 D-Pad 映射
 * @param buttons 按鈕狀態位元遮罩
 */
void mapDirectionalButtonsToDPad(uint16_t buttons) {
    // 根據按下的方向鍵組合設定 D-Pad
    bool up = buttons & BUTTON_RIGHT;
    bool down = buttons & BUTTON_LEFT;
    bool left = buttons & BUTTON_UP;
    bool right = buttons & BUTTON_DOWN;

    // 使用 D-Pad 函式處理方向
    Gamepad.dPad(up, down, left, right);
}

/**
 * 將 Wiimote 方向鍵映射到 NS 控制器的左搖桿
 * @param buttons 按鈕狀態位元遮罩
 */
void mapDirectionalButtonsToAnalogStick(uint16_t buttons) {
    // 預設搖桿在中心位置
    uint8_t finalXAxis = 128;
    uint8_t finalYAxis = 128;

    // 根據映射表處理各個方向
    for (size_t i = 0; i < directionalMappingsCount; i++) {
        if (buttons & directionalMappings[i].wiimoteButton) {
            finalXAxis = directionalMappings[i].xAxisValue;
            finalYAxis = directionalMappings[i].yAxisValue;
        }
    }
    
    // 處理對角線情況：如果同時按下相反方向，則維持中心點
    if ((buttons & BUTTON_UP) && (buttons & BUTTON_DOWN)) {
        finalXAxis = 128;
    }
    if ((buttons & BUTTON_LEFT) && (buttons & BUTTON_RIGHT)) {
        finalYAxis = 128;
    }

    // 設定搖桿位置
    Gamepad.leftXAxis(finalXAxis);
    Gamepad.leftYAxis(finalYAxis);
    Gamepad.rightXAxis(128); // 右搖桿保持中心
}

/**
 * 將 Wiimote 按鈕映射到 NS 控制器按鈕
 * @param buttons 按鈕狀態位元遮罩
 */
void mapWiimoteButtons(uint16_t buttons) {
    // 清除所有按鈕
    Gamepad.releaseAll();

    // 根據映射表處理按鈕
    for (size_t i = 0; i < buttonMappingsCount; i++) {
        if (buttons & buttonMappings[i].wiimoteButton) {
            Gamepad.press(buttonMappings[i].nsButton);
        }
    }

    // 注意：移除了方向鍵的重複映射，方向鍵現在只透過 D-Pad 或搖桿處理
}

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

        // 1. 處理方向鍵映射 - 根據模式選擇
        if (directionalButtonMode) {
            // 方向鍵模式：使用 D-Pad
            mapDirectionalButtonsToDPad(buttons);
            // 保持搖桿在中心位置
            Gamepad.leftXAxis(128);
            Gamepad.leftYAxis(128);
            Gamepad.rightXAxis(128);
        } else {
            // 類比搖桿模式：處理方向鍵到搖桿的映射
            if (buttons & (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT)) {
                mapDirectionalButtonsToAnalogStick(buttons);
            } else {
                // 如果沒有方向鍵被按下，重設搖桿到中心
                Gamepad.leftXAxis(128);
                Gamepad.leftYAxis(128);
                Gamepad.rightXAxis(128);
            }
            // 清除 D-Pad
            Gamepad.dPad(NSGAMEPAD_DPAD_CENTERED);
        }

        // 2. 處理按鈕映射
        mapWiimoteButtons(buttons);

        // 3. 將所有設定好的狀態透過 USB 發送給 Switch
        Gamepad.loop();
    }
}