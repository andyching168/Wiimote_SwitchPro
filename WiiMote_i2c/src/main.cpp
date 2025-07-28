/*
 * ESP32-S1: Wiimote Button Sender (Continuous Polling Mode)
 * 
 * 持續讀取 Wiimote 狀態，並以固定頻率發送，以確保控制器響應流暢。
 */
#include <Arduino.h>
#include "ESP32Wiimote.h"
#include "WiimoteData.h" // 確保你使用的是精簡版的 WiimoteData.h

// 定義 Serial2 使用的 GPIO
#define TX2_PIN 17
#define RX2_PIN 18

// 定義發送頻率
// 50Hz (20ms) 是一個很好的遊戲控制器更新率
#define SEND_INTERVAL_MS 20

ESP32Wiimote wiimote;
unsigned long lastSendTime = 0;

// 用一個變數來儲存最新的按鈕狀態
uint16_t currentButtonState = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32-S1 Continuous Sender Initializing...");
    Serial2.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
    wiimote.init();
    wiimote.addFilter(ACTION_IGNORE, FILTER_ACCEL);
    Serial.println("Sender Ready. Waiting for Wiimote connection...");
}

void loop() {
    // 總是要檢查 Wiimote 的任務
    wiimote.task();

    // 如果 Wiimote 狀態有更新，就更新我們儲存的狀態變數
    if (wiimote.available() > 0) {
        currentButtonState = wiimote.getButtonState();
    }

    // 使用 millis() 來控制發送頻率，這比 delay() 更好
    if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
        lastSendTime = millis();

        // 建立封包，內容為我們儲存的最新狀態
        ButtonPacket packet_to_send;
        packet_to_send.buttonState = currentButtonState;

        // 不管狀態有沒有變，都發送一次
        Serial2.write((uint8_t*)&packet_to_send, sizeof(packet_to_send));
        
        // (可選) 在本地監控視窗除錯，確認它在連續發送
        // Serial.printf("Sent state: 0x%04X\n", currentButtonState);
    }
}