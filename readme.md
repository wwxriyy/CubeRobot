# CubeRobot – Smart Eyes with RGB & Web Control

Animate expressive robot eyes on a TFT display, control RGB lighting, read temperature/humidity, and react to ambient light — all from a clean web interface.

---

## ✨ Features

- **Animated Eyes** — 1.8″ TFT 128×160 (ST7735)  
  Emotions: Neutral, Sleep, Angry, Surprise, Hearts, Shy, Happy, Sad, Cyclops.  
  Includes blinking, levitation, and side-looking animations (fully adjustable).

- **RGB LED Effects**  
  Manual control or 14 automatic modes:
  rainbow, police, fire, fade, breathing, and more.

- **Sensors**
  - **DHT11** — temperature & humidity
  - **Photoresistor (LDR)** — automatic eye-closing in darkness

- **Web Control Panel**
  - Real-time control
  - FPS counter
  - Mood timer
  - RGB settings
  - Eye animation tuning

- **Wi-Fi OTA** *(optional)*  
  Upload firmware wirelessly.

---

## 🔌 Pinout

| Component | ESP8266 (NodeMCU) |
|---|---|
| **TFT 1.8″ ST7735** | |
| BL | 3.3V |
| CS | D6 |
| DC | D1 |
| RES | D2 |
| SDA (MOSI) | D7 |
| SCK | D5 |
| VCC | 3.3V |
| GND | GND |
| **RGB LED** | |
| R | D3 |
| G | D4 |
| B | D0 |
| **DHT11** | |
| VCC | 3.3V |
| GND | GND |
| DATA | D8 (4.7kΩ pull-up) |
| **Photoresistor** | |
| Signal | A0 (10kΩ divider) |

> **Important:** Disconnect the DHT11 sensor from pin D8 before uploading firmware.

---

## 🚀 Quick Start

### 1. Install required libraries

Install these libraries in Arduino IDE or PlatformIO:

- `TFT_eSPI`
- `DHT sensor library`
- `Adafruit Unified Sensor`

---

### 2. Configure TFT display

Edit `User_Setup.h` for your ST7735 display configuration  
(example setup is included in the repository).

---

### 3. Set Wi‑Fi credentials

Open `src/main.cpp` and change:

    const char* wifi_ssid = "WIFI NAME";
    const char* wifi_password = "WIFI PASSWORD";

---

### 4. Upload firmware

Upload the sketch to your ESP8266 or ESP32 board.

---

### 5. Open the web panel

Open the index.html file after connecting the robot to wifi.

You can now control:

- RGB lighting
- Eye emotions
- Animations
- Sensors
- Timers

---

## 🌐 Important Web Panel Setup

Before opening the web panel, edit the ESP IP address in:

    data/js/script.js

Find the IP variable in `script.js`:

    const ip = "192.168.0.120";

Replace it with your ESP device IP address.

---

## 📸 Project Preview

![CubeRobot Front](https://raw.githubusercontent.com/wwxriyy/CubeRobot/main/img/github/robot.jpg)

![CubeRobot Web Panel](https://raw.githubusercontent.com/wwxriyy/CubeRobot/main/img/github/web.png)

---

## 📁 Repository Structure

    CubeRobot/
    ├── css/
    │   └── style.css
    ├── img/
    │   ├── Instagram_logo.png
    │   └── telegram_logo.png
    ├── include/
    ├── js/
    │   └── script.js
    ├── lib/
    ├── src/
    │   └── main.cpp
    ├── .gitignore
    ├── index.html
    ├── LICENSE
    ├── platformio.ini
    └── README.md

---

## 🤝 Credits

Designed and built by **@wwxriyy**

- Telegram: https://t.me/wwxriyy
- Instagram: https://instagram.com/wwxriyy
