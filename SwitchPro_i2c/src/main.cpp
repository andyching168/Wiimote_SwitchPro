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
#include "WiimoteData.h"   // 我們的共享資料結構
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

// --- Serial2 設定 ---
#define TX2_PIN 17
#define RX2_PIN 18

// --- WiFi 熱點設定 ---
const char* ap_ssid = "WiimoteController";
const char* ap_password = "12345678";

// --- 建立 Switch Gamepad 物件 ---
NSGamepad Gamepad;

// --- 建立網頁伺服器物件 ---
WebServer server(80);

// --- 建立 DNS 伺服器物件 (用於強制門戶) ---
DNSServer dnsServer;
const byte DNS_PORT = 53;

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

// 方向鍵到數位按鈕的映射配置(已廢棄，保留供參考)
const ButtonMapping directionalButtonMappings[] = {
    {BUTTON_UP,    NSButton_Y},     
    {BUTTON_DOWN,  NSButton_B},
    {BUTTON_LEFT,  NSButton_X},
    {BUTTON_RIGHT, NSButton_A}
};

// 注意：方向鍵到搖桿的映射現在使用8方向邏輯，不再需要映射表
// 舊的 DirectionalMapping 結構已被新的8方向邏輯取代

// 取得映射表大小
const size_t buttonMappingsCount = sizeof(buttonMappings) / sizeof(buttonMappings[0]);
const size_t directionalButtonMappingsCount = sizeof(directionalButtonMappings) / sizeof(directionalButtonMappings[0]);
// 注意：directionalMappingsCount 已移除，現在使用8方向邏輯

// 函式宣告
bool captivePortal();
bool isIp(String str);
void handleRoot();
void handleSetMode();
void handleStatus();
void handleNotFound();
void handleCaptivePortal();

/**
 * 處理根路徑請求 - 顯示設定頁面
 */
void handleRoot() {
    // 檢查是否需要強制門戶重導向
    if (captivePortal()) {
        return;
    }
    
    String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">";
    html += "<title>Wiimote 控制器設定</title>";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }";
    html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }";
    html += ".status { padding: 10px; margin: 10px 0; border-radius: 5px; }";
    html += ".dpad-mode { background-color: #d4edda; color: #155724; }";
    html += ".analog-mode { background-color: #cce7ff; color: #004085; }";
    html += ".button { background-color: #007bff; color: white; border: none; padding: 15px 30px; ";
    html += "border-radius: 5px; cursor: pointer; margin: 10px; font-size: 16px; }";
    html += ".button:hover { background-color: #0056b3; }";
    html += ".info { background-color: #fff3cd; padding: 15px; border-radius: 5px; margin: 15px 0; }";
    html += "</style></head><body>";
    html += "<div class=\"container\">";
    html += "<h1>🎮 Wiimote 控制器設定</h1>";
    
    html += "<div class=\"status " + String(directionalButtonMode ? "dpad-mode" : "analog-mode") + "\">";
    html += "<h2>目前模式: " + String(directionalButtonMode ? "方向鍵 (D-Pad)" : "左類比搖桿") + "</h2>";
    html += "<p>" + String(directionalButtonMode ? 
        "Wiimote 的方向鍵會對應到 Switch 的數位方向鍵" : 
        "Wiimote 的方向鍵會對應到 Switch 的左類比搖桿") + "</p>";
    html += "</div>";
    
    html += "<h3>變更控制模式:</h3>";
    html += "<button class=\"button\" onclick=\"setMode('dpad')\">設為方向鍵模式</button>";
    html += "<button class=\"button\" onclick=\"setMode('analog')\">設為類比搖桿模式</button>";
    
    html += "<div class=\"info\">";
    html += "<h3>📖 模式說明:</h3>";
    html += "<p><strong>方向鍵模式:</strong> Wiimote 的上下左右按鈕會對應到 Switch 的數位方向鍵 (D-Pad)</p>";
    html += "<p><strong>類比搖桿模式:</strong> Wiimote 的上下左右按鈕會對應到 Switch 的左類比搖桿</p>";
    html += "<div style=\"margin-top: 10px; padding: 10px; background-color: #e3f2fd; border-radius: 5px;\">";
    html += "<h4>🎯 8方向類比搖桿支援:</h4>";
    html += "<p style=\"margin: 5px 0;\">• 單一方向: 上、下、左、右</p>";
    html += "<p style=\"margin: 5px 0;\">• 對角線方向: 左上、右上、左下、右下</p>";
    html += "<p style=\"margin: 5px 0;\">• 同時按下相鄰按鈕可實現精確的對角線控制</p>";
    html += "</div>";
    html += "</div>";
    
    html += "<p><small>💡 連線資訊: " + WiFi.softAPIP().toString() + "</small></p>";
    html += "</div>";
    
    html += "<script>";
    html += "function setMode(mode) {";
    html += "fetch('/setMode?mode=' + mode)";
    html += ".then(response => response.text())";
    html += ".then(data => { alert(data); location.reload(); })";
    html += ".catch(error => { alert('設定失敗: ' + error); });";
    html += "}";
    html += "</script>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

/**
 * 處理模式設定請求
 */
void handleSetMode() {
    if (server.hasArg("mode")) {
        String mode = server.arg("mode");
        if (mode == "dpad") {
            directionalButtonMode = true;
            server.send(200, "text/plain", "已切換至方向鍵模式！");
            Serial.println("模式已切換: 方向鍵 (D-Pad)");
        } else if (mode == "analog") {
            directionalButtonMode = false;
            server.send(200, "text/plain", "已切換至類比搖桿模式！");
            Serial.println("模式已切換: 左類比搖桿");
        } else {
            server.send(400, "text/plain", "無效的模式參數");
        }
    } else {
        server.send(400, "text/plain", "缺少模式參數");
    }
}

/**
 * 處理狀態查詢請求
 */
void handleStatus() {
    String json = "{";
    json += "\"directionalButtonMode\":" + String(directionalButtonMode ? "true" : "false") + ",";
    json += "\"mode\":\"" + String(directionalButtonMode ? "dpad" : "analog") + "\",";
    json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\"";
    json += "}";
    server.send(200, "application/json", json);
}

/**
 * 處理強制門戶 - 任何未知請求都重導向到主頁
 */
void handleNotFound() {
    // 對於所有未找到的路徑，重導向到主頁
    String redirectHtml = "<!DOCTYPE html><html><head>";
    redirectHtml += "<meta charset=\"UTF-8\">";
    redirectHtml += "<title>重導向中...</title>";
    redirectHtml += "<meta http-equiv=\"refresh\" content=\"0; url=http://";
    redirectHtml += WiFi.softAPIP().toString();
    redirectHtml += "/\">";
    redirectHtml += "</head><body>";
    redirectHtml += "<p>正在重導向到設定頁面...</p>";
    redirectHtml += "<script>window.location.href='http://";
    redirectHtml += WiFi.softAPIP().toString();
    redirectHtml += "/';</script>";
    redirectHtml += "</body></html>";
    
    server.send(200, "text/html", redirectHtml);
}

/**
 * 處理強制門戶檢測請求
 */
void handleCaptivePortal() {
    // 常見的強制門戶檢測端點
    handleRoot();
}

/**
 * 檢查是否為強制門戶請求
 */
bool isIp(String str) {
    for (size_t i = 0; i < str.length(); i++) {
        int c = str.charAt(i);
        if (c != '.' && (c < '0' || c > '9')) {
            return false;
        }
    }
    return true;
}

/**
 * 處理所有請求的通用重導向
 */
bool captivePortal() {
    String hostHeader = server.hostHeader();
    if (!isIp(hostHeader) && hostHeader.indexOf(WiFi.softAPIP().toString()) < 0) {
        // 如果主機名不是 IP 地址且不包含我們的 IP，則重導向
        String redirectUrl = "http://" + WiFi.softAPIP().toString() + "/";
        server.sendHeader("Location", redirectUrl, true);
        server.send(302, "text/plain", "");
        return true;
    }
    return false;
}

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

    // 檢查各個方向按鈕的狀態
    bool up = buttons & BUTTON_RIGHT;
    bool down = buttons & BUTTON_LEFT;
    bool left = buttons & BUTTON_UP;
    bool right = buttons & BUTTON_DOWN;

    // 處理 8 方向映射
    if (up && right) {
        // 右上對角線 (Wiimote UP+RIGHT -> NS UP+RIGHT)
        finalXAxis = 255;  // 右
        finalYAxis = 0;    // 上
    } else if (up && left) {
        // 左上對角線 (Wiimote UP+LEFT -> NS UP+LEFT)
        finalXAxis = 0;    // 左
        finalYAxis = 0;    // 上
    } else if (down && right) {
        // 右下對角線 (Wiimote DOWN+RIGHT -> NS DOWN+RIGHT)
        finalXAxis = 255;  // 右
        finalYAxis = 255;  // 下
    } else if (down && left) {
        // 左下對角線 (Wiimote DOWN+LEFT -> NS DOWN+LEFT)
        finalXAxis = 0;    // 左
        finalYAxis = 255;  // 下
    } else if (up) {
        // 純上方向 (Wiimote UP -> NS UP)
        finalXAxis = 128;  // 中心
        finalYAxis = 0;    // 上
    } else if (down) {
        // 純下方向 (Wiimote DOWN -> NS DOWN)
        finalXAxis = 128;  // 中心
        finalYAxis = 255;  // 下
    } else if (left) {
        // 純左方向 (Wiimote LEFT -> NS LEFT)
        finalXAxis = 0;    // 左
        finalYAxis = 128;  // 中心
    } else if (right) {
        // 純右方向 (Wiimote RIGHT -> NS RIGHT)
        finalXAxis = 255;  // 右
        finalYAxis = 128;  // 中心
    }
    
    // 處理相反方向同時按下的情況：維持中心點
    if (up && down) {
        finalYAxis = 128;  // Y軸回到中心
    }
    if (left && right) {
        finalXAxis = 128;  // X軸回到中心
    }

    // 設定搖桿位置
    Gamepad.leftXAxis(finalXAxis);
    Gamepad.leftYAxis(finalYAxis);
    Gamepad.rightXAxis(128); // 右搖桿保持中心
    
    // 除錯輸出（可選，用於測試）
    if (buttons & (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT)) {
        Serial.printf("8-Way Analog: U:%d D:%d L:%d R:%d -> X:%d Y:%d\n", 
                     up, down, left, right, finalXAxis, finalYAxis);
    }
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

    // 設定 WiFi 熱點
    Serial.println("Setting up WiFi Access Point...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_password);
    
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    Serial.println("WiFi AP Name: " + String(ap_ssid));
    Serial.println("WiFi Password: " + String(ap_password));

    // 啟動 DNS 伺服器（強制門戶功能）
    dnsServer.start(DNS_PORT, "*", myIP);
    Serial.println("DNS server started for captive portal");

    // 設定網頁伺服器路由
    server.on("/", handleRoot);
    server.on("/setMode", handleSetMode);
    server.on("/status", handleStatus);
    
    // 常見的強制門戶檢測端點
    server.on("/generate_204", handleCaptivePortal);         // Android
    server.on("/fwlink", handleCaptivePortal);               // Microsoft
    server.on("/hotspot-detect.html", handleCaptivePortal); // Apple iOS
    server.on("/connecttest.txt", handleCaptivePortal);     // Windows
    server.on("/ncsi.txt", handleCaptivePortal);            // Windows
    
    // 處理所有未匹配的請求
    server.onNotFound(handleNotFound);
    
    // 啟動網頁伺服器
    server.begin();
    Serial.println("HTTP server started");
    Serial.println("Connect to WiFi '" + String(ap_ssid) + "' and visit: http://" + myIP.toString());

    Serial.println("Ready. Connect to Switch and waiting for button data...");
}

void loop() {
    // 處理 DNS 請求（強制門戶功能）
    dnsServer.processNextRequest();
    
    // 處理網頁伺服器請求
    server.handleClient();
    
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