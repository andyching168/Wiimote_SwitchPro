# WiFi 熱點控制功能使用指南

## 功能說明
這個更新為您的 Wiimote 控制器新增了 WiFi 熱點功能，讓您可以透過網頁介面即時切換方向鍵的對應模式。

## 🆕 新增功能 - 強制門戶 (Captive Portal)
- **自動跳轉**：連接到 WiFi 熱點後，手機和電腦會自動彈出設定頁面
- **智慧重導向**：任何網址都會自動重導向到控制頁面
- **跨平台支援**：支援 Android、iOS、Windows、macOS 的自動檢測

## 使用方法

### 1. 啟動裝置
- 將程式碼上傳到 ESP32-S3 後，裝置會自動建立一個 WiFi 熱點

### 2. 連接 WiFi 熱點
- **熱點名稱:** `WiimoteController`
- **密碼:** `12345678`

### 3. 自動開啟設定頁面
- **手機 (Android/iOS)**：連接後會自動彈出「登入到網路」或類似的通知，點擊即可進入設定頁面
- **電腦 (Windows/macOS)**：連接後可能會自動開啟瀏覽器，或手動前往任何網址都會重導向到設定頁面
- **手動方式**：如果沒有自動跳轉，可以手動前往 `http://192.168.4.1`

### 4. 切換控制模式
頁面提供兩個按鈕：
- **設為方向鍵模式**: Wiimote 的方向鍵對應到 Switch 的數位方向鍵 (D-Pad)
- **設為類比搖桿模式**: Wiimote 的方向鍵對應到 Switch 的左類比搖桿

## 技術細節

### 強制門戶功能
程式現在支援以下強制門戶檢測端點：
- `/generate_204` - Android 裝置檢測
- `/fwlink` - Microsoft 裝置檢測  
- `/hotspot-detect.html` - Apple iOS 檢測
- `/connecttest.txt` - Windows 檢測
- `/ncsi.txt` - Windows 網路連線狀態指示器

### 網頁 API 端點
- `GET /` - 主設定頁面
- `GET /setMode?mode=dpad` - 切換到方向鍵模式
- `GET /setMode?mode=analog` - 切換到類比搖桿模式  
- `GET /status` - 取得當前狀態 (JSON 格式)
- 任何其他路徑都會重導向到主頁面

### 序列監視器輸出
裝置啟動時會在序列監視器顯示：
```
ESP32-S3 NS Controller Initializing...
Setting up WiFi Access Point...
AP IP address: 192.168.4.1
WiFi AP Name: WiimoteController
WiFi Password: 12345678
DNS server started for captive portal
HTTP server started
Connect to WiFi 'WiimoteController' and visit: http://192.168.4.1
Ready. Connect to Switch and waiting for button data...
```

### 自訂設定
如需更改熱點設定，請修改 `main.cpp` 中的這些變數：
```cpp
const char* ap_ssid = "WiimoteController";     // 熱點名稱
const char* ap_password = "12345678";          // 熱點密碼
```

## 注意事項
1. 強制門戶功能會讓連接體驗更順暢
2. 任何網址都會重導向到設定頁面，無需記住 IP 地址
3. 設定變更會立即生效，無需重新啟動
4. 支援多裝置同時連接和設定

## 疑難排解
- **如果沒有自動跳轉**：手動在瀏覽器輸入任何網址（如 `google.com`）會自動重導向
- **如果無法連接熱點**：請檢查裝置是否支援 2.4GHz WiFi
- **如果頁面無法載入**：請確認已連接到正確的熱點，然後嘗試重新連接
- **可透過序列監視器查看詳細的除錯資訊**

## 📱 各平台連接指南

### Android
1. 連接到熱點後，通知欄會顯示「登入到網路」
2. 點擊通知即可自動開啟設定頁面

### iOS  
1. 連接到熱點後會自動彈出強制門戶頁面
2. 如未自動彈出，開啟 Safari 輸入任何網址

### Windows
1. 連接後可能會自動開啟瀏覽器
2. 手動開啟瀏覽器輸入任何網址都會重導向

### macOS
1. 連接後系統會詢問是否開啟登入頁面
2. 選擇「是」即可自動開啟設定頁面
