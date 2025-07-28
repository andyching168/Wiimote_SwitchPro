// 檔案: ButtonData.h
// 作用: 定義 S1 和 S3 之間通訊用的共享資料結構 (僅按鈕)

#pragma once
#include <stdint.h>

// --- 按鈕位元定義 (從 ESP32Wiimote 函式庫複製) ---
#define BUTTON_A        0x0800//0x0008
#define BUTTON_B        0x0400//0x0004
#define BUTTON_C        0x4000  // Nunchuk
#define BUTTON_Z        0x0080  // Nunchuk
#define BUTTON_ONE      0x0200
#define BUTTON_TWO      0x0100
#define BUTTON_MINUS    0x1000
#define BUTTON_PLUS     0x0010 //0x1000
#define BUTTON_HOME     0x8000 //0x0080
#define BUTTON_LEFT     0x0001//ok
#define BUTTON_RIGHT    0x0002//ok
#define BUTTON_UP       0x0008
#define BUTTON_DOWN     0x0004

// 定義精簡版的通訊封包結構
// __attribute__((packed)) 確保編譯器不會增加額外的填充位元組
struct __attribute__((packed)) ButtonPacket {
    // 只包含一個成員：16位元的按鈕狀態
    // 來自 wiimote.getButtonState()
    uint16_t buttonState;
};