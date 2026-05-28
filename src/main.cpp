/*
 * CubeRobot – ESP8266, RGB LED Controller, TFT Eye Display, DHT11 sensor.
 * Version 6.0 - Emotions: Neutral, Sleep, Angry, Surprise, Hearts, Shy, Happy, Sad, Cyclops.
 * Copyright (c) 2025 @wwxriyy (Instagram / Telegram)
 * MIT License
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <TFT_eSPI.h>
#include <DHT.h>

// Pin Definitions
#define RED_PIN   0
#define GREEN_PIN 2
#define BLUE_PIN  16
#define LDR_PIN   A0
#define DHT_PIN   1
#define DHT_TYPE  DHT11

// Display
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite eyes = TFT_eSprite(&tft); 
TFT_eSprite region = TFT_eSprite(&tft);
DHT dht(DHT_PIN, DHT_TYPE);

// WiFi
const char* wifi_ssid = "Ravlyk";
const char* wifi_password = "9G2bUVdTDWXPj3Vf6x";
ESP8266WebServer server(80);

// LED State
bool ledPower = true;
String mode = "manual";
int redValue = 0, greenValue = 0, blueValue = 0;
int stepValue = 0;

// Eye Geometry
const int eyeWidth = 40;
const int eyeHeight = 40;
const int eyeY = 44;
const int leftEyeX = 30;
const int rightEyeX = 90;
const int minBlinkHeight = 8;

// Light Sensor
int ldrThreshold = 300;
bool ldrEnabled = true;
int forceEmotion = 0;          // 0 = auto, 1 = forced sleep, 2 = forced awake

// DHT11 Sensor on TX (GPIO1)
bool dhtEnabled = true;
float dhtTemperature = NAN;
float dhtHumidity = NAN;
bool dhtReadOk = false;
unsigned long lastDhtRead = 0;
const unsigned long dhtReadInterval = 2000;

// Blink Parameters
int blinkMin = 2000;
int blinkMax = 6000;
int currentEyeHeight = eyeHeight;
bool isBlinking = false;
unsigned long blinkStartTime = 0;
int blinkStep = 0;
const int blinkSteps = 8;
unsigned long nextBlinkTime = 0;
bool sleeping = false;

// Eye Animations
bool levitationEnabled = true;
int  levitationSpeed = 2;
int  levitationAmplitude = 3;
bool lookAroundEnabled = true;
int  lookAroundSpeed = 3;
int  lookAroundAmplitude = 5;
float levitationPhase = 0.0;
float lookPhase = 0.0;
int levitationOffset = 0;
int lookOffset = 0;

// Atomic redraw
int lastSpriteX = leftEyeX;
int lastSpriteY = eyeY;
int lastEyeHeight = -1;
const int REGION_W = 130;
const int REGION_H = 60;

// FPS
float currentFps = 0;
unsigned long lastFpsCalc = 0;
unsigned long frameCount = 0;

// Emotions
enum Emotion { EM_NEUTRAL, EM_SLEEP, EM_ANGRY, EM_SURPRISE, EM_HEARTS, EM_SHY, EM_HAPPY, EM_SAD, EM_CYCLOPS };
Emotion currentEmotion = EM_NEUTRAL;
Emotion lastRenderedEmotion = EM_NEUTRAL;
unsigned long nextEmotionChange = 0;
int moodMin = 5;
int moodMax = 15;
bool autoMoodEnabled = true;
float emotionPhase = 0.0;

#define BGCOLOR TFT_BLACK

void renderEyesAtomic();

int bootLogY = 0;
const int bootLogTop = 58;
const int bootLineHeight = 9;

void drawCenteredText(const String& text, int y, int size, uint16_t color) {
  tft.setTextSize(size);
  tft.setTextColor(color, BGCOLOR);
  int x = (tft.width() - tft.textWidth(text)) / 2;
  if (x < 0) x = 0;
  tft.setCursor(x, y);
  tft.print(text);
}

void showBootScreen() {
  tft.fillScreen(BGCOLOR);
  drawCenteredText("CubeRobot", 16, 2, TFT_WHITE);
  drawCenteredText("by @wwxriyy", 38, 1, TFT_CYAN);
  tft.drawFastHLine(8, 54, tft.width() - 16, TFT_DARKGREY);
  bootLogY = bootLogTop;
}

void bootLog(const String& message) {
  if (bootLogY > tft.height() - bootLineHeight) {
    tft.fillRect(0, bootLogTop, tft.width(), tft.height() - bootLogTop, BGCOLOR);
    bootLogY = bootLogTop;
  }

  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN, BGCOLOR);
  tft.fillRect(0, bootLogY, tft.width(), bootLineHeight, BGCOLOR);
  tft.setCursor(4, bootLogY);
  tft.print("> ");
  tft.print(message);
  bootLogY += bootLineHeight;
  delay(180);
  yield();
}

void drawHeart(int x, int y, int size, uint16_t color) {
  int r = size / 3;
  int cx = x + size / 2;
  eyes.fillCircle(x + r, y + r, r, color);
  eyes.fillCircle(x + size - r, y + r, r, color);
  eyes.fillTriangle(x, y + r, x + size, y + r, cx, y + size, color);
  eyes.fillTriangle(x + 4, y + size / 3, x + size - 4, y + size / 3, cx, y + size, color);
}

void drawHappyEyelid(int x, int y, int w, int h, uint16_t color) {
  eyes.fillRoundRect(x, y, w, h, 8, color);
  eyes.fillRoundRect(x - 1, y + h / 2, w + 2, h, 8, BGCOLOR);
}

void drawShyEye(int x, int y, int w, int h) {
  int eyeH = h / 2;
  if (eyeH < 10) eyeH = 10;
  int eyeY = y + h / 3;
  eyes.fillRoundRect(x, eyeY, w, eyeH, 8, TFT_PINK);
  eyes.fillRoundRect(x - 1, eyeY + eyeH / 2, w + 2, eyeH, 8, BGCOLOR);
}

void drawBlush(int x, int y, uint16_t color) {
  eyes.drawLine(x, y + 8, x + 8, y, color);
  eyes.drawLine(x + 8, y + 10, x + 16, y + 2, color);
  eyes.drawLine(x + 16, y + 12, x + 24, y + 4, color);
}

String formatFloatOrNull(float value, int digits) {
  if (isnan(value)) return "null";
  return String(value, digits);
}

void drawDhtOverlay() {
  int y = tft.height() - 16;
  tft.fillRect(0, y, tft.width(), 16, BGCOLOR);
  tft.setTextColor(TFT_WHITE, BGCOLOR);
  tft.setTextSize(1);

  String tempText = dhtEnabled && !isnan(dhtTemperature)
    ? "T:" + String(dhtTemperature, 1) + "C"
    : (dhtEnabled ? "T:ERR" : "T:--C");
  String humText = dhtEnabled && !isnan(dhtHumidity)
    ? "H:" + String(dhtHumidity, 0) + "%"
    : (dhtEnabled ? "H:ERR" : "H:--%");

  tft.setCursor(2, y + 4);
  tft.print(tempText);
  tft.setCursor(tft.width() - 42, y + 4);
  tft.print(humText);
}

void updateDhtSensor(bool force = false) {
  if (!force && millis() - lastDhtRead < dhtReadInterval) return;
  lastDhtRead = millis();

  if (!dhtEnabled) {
    dhtTemperature = NAN;
    dhtHumidity = NAN;
    dhtReadOk = false;
    drawDhtOverlay();
    return;
  }

  float newHumidity = dht.readHumidity();
  float newTemperature = dht.readTemperature();
  if (!isnan(newHumidity) && !isnan(newTemperature)) {
    dhtHumidity = newHumidity;
    dhtTemperature = newTemperature;
    dhtReadOk = true;
  } else {
    dhtReadOk = false;
  }
  drawDhtOverlay();
}

void drawEmotionOnSprite() {
  int yOffset = (eyeHeight - currentEyeHeight) / 2;
  eyes.fillSprite(BGCOLOR);
  bool eyesClosed = sleeping || currentEmotion == EM_SLEEP;
  int pulse = sin(emotionPhase) * 2;

  if (eyesClosed) {
    int closedHeight = 15;
    int closedY = yOffset + (currentEyeHeight - closedHeight) / 2;
    eyes.fillRoundRect(0, closedY, eyeWidth, closedHeight, 8, TFT_WHITE);
    eyes.fillRoundRect(60, closedY, eyeWidth, closedHeight, 8, TFT_WHITE);

  } else if (currentEmotion == EM_ANGRY) {

    eyes.fillRoundRect(0, yOffset, eyeWidth, currentEyeHeight, 8, TFT_RED);
    eyes.fillRoundRect(60, yOffset, eyeWidth, currentEyeHeight, 8, TFT_RED);
    int yMid = yOffset + currentEyeHeight / 2;
    eyes.fillTriangle(0, yOffset, eyeWidth, yOffset, eyeWidth, yMid, BGCOLOR);
    eyes.fillTriangle(60, yOffset, 100, yOffset, 60, yMid, BGCOLOR);

  } else if (currentEmotion == EM_SURPRISE) {

    eyes.fillRoundRect(0, yOffset, eyeWidth, currentEyeHeight, 6, TFT_WHITE);
    eyes.fillRoundRect(60, yOffset, eyeWidth, currentEyeHeight, 6, TFT_WHITE);
    int innerW = eyeWidth / 3 + pulse;
    int innerH = currentEyeHeight / 2 + pulse;
    eyes.fillRoundRect((eyeWidth - innerW) / 2, yOffset + (currentEyeHeight - innerH) / 2, innerW, innerH, 3, BGCOLOR);
    eyes.fillRoundRect(60 + (eyeWidth - innerW) / 2, yOffset + (currentEyeHeight - innerH) / 2, innerW, innerH, 3, BGCOLOR);

  } else if (currentEmotion == EM_HEARTS) {

    int size = currentEyeHeight - 3 + pulse;
    if (size > eyeWidth) size = eyeWidth;
    if (size < 12) size = 12;
    drawHeart((eyeWidth - size) / 2, yOffset, size, TFT_RED);
    drawHeart(60 + (eyeWidth - size) / 2, yOffset, size, TFT_RED);

  } else if (currentEmotion == EM_SHY) {

    int shyOffset = sin(emotionPhase * 0.8) * 1;
    drawShyEye(0, yOffset + shyOffset, eyeWidth, currentEyeHeight);
    drawShyEye(60, yOffset + shyOffset, eyeWidth, currentEyeHeight);
    drawBlush(7, yOffset + 26 + shyOffset, TFT_MAGENTA);
    drawBlush(69, yOffset + 26 + shyOffset, TFT_MAGENTA);

  } else if (currentEmotion == EM_HAPPY) {

    drawHappyEyelid(0, yOffset, eyeWidth, currentEyeHeight, TFT_WHITE);
    drawHappyEyelid(60, yOffset, eyeWidth, currentEyeHeight, TFT_WHITE);

  } else if (currentEmotion == EM_SAD) {

    int sadOffset = 1 + sin(emotionPhase * 0.6) * 1;
    eyes.fillRoundRect(0, yOffset, eyeWidth, currentEyeHeight, 8, TFT_BLUE);
    eyes.fillRoundRect(60, yOffset, eyeWidth, currentEyeHeight, 8, TFT_BLUE);

    int yMid = yOffset + currentEyeHeight / 2 + sadOffset;
    eyes.fillTriangle(0, yOffset, eyeWidth, yOffset, 0, yMid, BGCOLOR);
    eyes.fillTriangle(60, yOffset, 100, yOffset, 100, yMid, BGCOLOR);

  } else if (currentEmotion == EM_CYCLOPS) {

    int cyclopsW = 64 + pulse * 2;
    int cyclopsX = (100 - cyclopsW) / 2;
    eyes.fillRoundRect(cyclopsX, yOffset, cyclopsW, currentEyeHeight, 10, TFT_CYAN);
    int innerW = cyclopsW / 4;
    int innerH = currentEyeHeight / 2;
    eyes.fillRoundRect(cyclopsX + (cyclopsW - innerW) / 2, yOffset + (currentEyeHeight - innerH) / 2, innerW, innerH, 4, BGCOLOR);

  } else {

    // Neutral eyes
    eyes.fillRoundRect(0, yOffset, eyeWidth, currentEyeHeight, 8, TFT_WHITE);
    eyes.fillRoundRect(60, yOffset, eyeWidth, currentEyeHeight, 8, TFT_WHITE);
  }
}

void setEmotionSmooth(Emotion nextEmotion) {
  if (currentEmotion == nextEmotion && currentEyeHeight == eyeHeight) {
    renderEyesAtomic();
    return;
  }

  isBlinking = false;
  for (int h = currentEyeHeight; h >= minBlinkHeight; h -= 6) {
    currentEyeHeight = h;
    renderEyesAtomic();
    delay(18);
    yield();
  }

  currentEmotion = nextEmotion;
  emotionPhase = 0.0;

  for (int h = minBlinkHeight; h <= eyeHeight; h += 6) {
    currentEyeHeight = h;
    renderEyesAtomic();
    delay(18);
    yield();
  }
  currentEyeHeight = eyeHeight;
  renderEyesAtomic();
}

void renderEyesAtomic() {
  int newX = leftEyeX + lookOffset;
  int newY = eyeY + levitationOffset;
  if (newX == lastSpriteX && newY == lastSpriteY && currentEyeHeight == lastEyeHeight && currentEmotion == lastRenderedEmotion) return;

  int minX = min(lastSpriteX, newX);
  int minY = min(lastSpriteY, newY);
  int maxX = max(lastSpriteX + 100, newX + 100);
  int maxY = max(lastSpriteY + 40, newY + 40);
  int unionW = maxX - minX;
  int unionH = maxY - minY;
  if (unionW > REGION_W) unionW = REGION_W;
  if (unionH > REGION_H) unionH = REGION_H;

  region.fillSprite(BGCOLOR);
  int eyeInRegionX = newX - minX;
  int eyeInRegionY = newY - minY;

  drawEmotionOnSprite();
  region.fillRect(eyeInRegionX, eyeInRegionY, 100, 40, BGCOLOR);
  region.pushImage(eyeInRegionX, eyeInRegionY, 100, 40, (uint16_t*)eyes.getPointer());
  region.pushSprite(minX, minY);

  lastSpriteX = newX;
  lastSpriteY = newY;
  lastEyeHeight = currentEyeHeight;
  lastRenderedEmotion = currentEmotion;
  frameCount++;
}

void startBlink() {
  if (sleeping || currentEmotion != EM_NEUTRAL) return;
  isBlinking = true;
  blinkStartTime = millis();
  blinkStep = 0;
}

void updateBlink() {
  if (!isBlinking) return;
  unsigned long now = millis();
  int newStep = (now - blinkStartTime) / 30;
  if (newStep > blinkSteps * 2) {
    isBlinking = false;
    currentEyeHeight = eyeHeight;
    renderEyesAtomic();
    nextBlinkTime = now + random(blinkMin, blinkMax);
    return;
  }
  if (newStep != blinkStep) {
    blinkStep = newStep;
    if (blinkStep < blinkSteps)
      currentEyeHeight = map(blinkStep, 0, blinkSteps, eyeHeight, minBlinkHeight);
    else
      currentEyeHeight = map(blinkStep - blinkSteps, 0, blinkSteps, minBlinkHeight, eyeHeight);
    renderEyesAtomic();
  }
}

// -------------------------------------------------------------------
// HTTP Handlers
// -------------------------------------------------------------------
void handleRed() {
  if (mode != "manual") return;
  redValue = server.arg("value").toInt();
  if (ledPower) analogWrite(RED_PIN, redValue);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
  yield();
}
void handleGreen() {
  if (mode != "manual") return;
  greenValue = server.arg("value").toInt();
  if (ledPower) analogWrite(GREEN_PIN, greenValue);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
  yield();
}
void handleBlue() {
  if (mode != "manual") return;
  blueValue = server.arg("value").toInt();
  if (ledPower) analogWrite(BLUE_PIN, blueValue);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
  yield();
}
void handlePower() {
  ledPower = server.arg("state").toInt();
  if (ledPower && mode == "manual") {
    analogWrite(RED_PIN, redValue);
    analogWrite(GREEN_PIN, greenValue);
    analogWrite(BLUE_PIN, blueValue);
  } else {
    analogWrite(RED_PIN, 0);
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 0);
  }
  // No changes to animations!
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
  yield();
}
void handleMode() {
  String val = server.arg("value");
  if (val == "ping") {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "OK");
    return;
  }
  mode = val;
  stepValue = 0;
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
  yield();
}
void handleStatus() {
  String json = "{";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"power\":" + String(ledPower ? "true" : "false") + ",";
  json += "\"mode\":\"" + mode + "\",";
  json += "\"red\":" + String(redValue) + ",";
  json += "\"green\":" + String(greenValue) + ",";
  json += "\"blue\":" + String(blueValue) + ",";
  json += "\"ldrEnabled\":" + String(ldrEnabled ? "true" : "false") + ",";
  json += "\"ldrThreshold\":" + String(ldrThreshold) + ",";
  json += "\"dhtEnabled\":" + String(dhtEnabled ? "true" : "false") + ",";
  json += "\"dhtOk\":" + String(dhtReadOk ? "true" : "false") + ",";
  json += "\"temperature\":" + formatFloatOrNull(dhtTemperature, 1) + ",";
  json += "\"humidity\":" + formatFloatOrNull(dhtHumidity, 1) + ",";
  json += "\"forceEmotion\":" + String(forceEmotion) + ",";
  json += "\"sleeping\":" + String(sleeping ? "true" : "false") + ",";
  json += "\"blinkMin\":" + String(blinkMin) + ",";
  json += "\"blinkMax\":" + String(blinkMax) + ",";
  json += "\"levitationEnabled\":" + String(levitationEnabled ? "true" : "false") + ",";
  json += "\"levitationSpeed\":" + String(levitationSpeed) + ",";
  json += "\"levitationAmp\":" + String(levitationAmplitude) + ",";
  json += "\"lookEnabled\":" + String(lookAroundEnabled ? "true" : "false") + ",";
  json += "\"lookSpeed\":" + String(lookAroundSpeed) + ",";
  json += "\"lookAmp\":" + String(lookAroundAmplitude) + ",";
  json += "\"moodMin\":" + String(moodMin) + ",";
  json += "\"moodMax\":" + String(moodMax) + ",";
  json += "\"autoMoodEnabled\":" + String(autoMoodEnabled ? "true" : "false") + ",";
  json += "\"currentEmotion\":";
  switch (currentEmotion) {
    case EM_NEUTRAL: json += "\"neutral\""; break;
    case EM_SLEEP:   json += "\"sleep\""; break;
    case EM_ANGRY:   json += "\"angry\""; break;
    case EM_SURPRISE: json += "\"surprise\""; break;
    case EM_HEARTS:  json += "\"hearts\""; break;
    case EM_SHY:     json += "\"shy\""; break;
    case EM_HAPPY:   json += "\"happy\""; break;
    case EM_SAD:     json += "\"sad\""; break;
    case EM_CYCLOPS: json += "\"cyclops\""; break;
    default:         json += "\"neutral\"";
  }
  json += "}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
  yield();
}
void handleLight() {
  int lightLevel = analogRead(LDR_PIN);
  String json = "{\"light\":" + String(lightLevel) + "}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
  yield();
}
void handleLdr() {
  if (server.hasArg("enable")) ldrEnabled = server.arg("enable").toInt();
  if (server.hasArg("threshold")) ldrThreshold = server.arg("threshold").toInt();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
  yield();
}
void handleDht() {
  if (server.hasArg("enable")) {
    dhtEnabled = server.arg("enable").toInt();
    if (!dhtEnabled) {
      dhtTemperature = NAN;
      dhtHumidity = NAN;
      dhtReadOk = false;
    }
    updateDhtSensor(true);
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
  yield();
}
void handleClimate() {
  String json = "{";
  json += "\"enabled\":" + String(dhtEnabled ? "true" : "false") + ",";
  json += "\"ok\":" + String(dhtReadOk ? "true" : "false") + ",";
  json += "\"temperature\":" + formatFloatOrNull(dhtTemperature, 1) + ",";
  json += "\"humidity\":" + formatFloatOrNull(dhtHumidity, 1);
  json += "}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
  yield();
}
void handleEmotion() {
  String val = server.arg("mode");
  if (val == "auto") forceEmotion = 0;
  else if (val == "sleep") forceEmotion = 1;
  else if (val == "awake") forceEmotion = 2;
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
  yield();
}
void handleBlink() {
  if (server.hasArg("min")) {
    blinkMin = server.arg("min").toInt();
    if (blinkMin < 500) blinkMin = 500;
    if (blinkMin > 10000) blinkMin = 10000;
  }
  if (server.hasArg("max")) {
    blinkMax = server.arg("max").toInt();
    if (blinkMax < blinkMin) blinkMax = blinkMin + 500;
    if (blinkMax > 15000) blinkMax = 15000;
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
  yield();
}
void handleLevitation() {
  if (server.hasArg("enable")) levitationEnabled = server.arg("enable").toInt();
  if (server.hasArg("speed")) {
    levitationSpeed = server.arg("speed").toInt();
    if (levitationSpeed < 1) levitationSpeed = 1;
    if (levitationSpeed > 10) levitationSpeed = 10;
  }
  if (server.hasArg("amp")) {
    levitationAmplitude = server.arg("amp").toInt();
    if (levitationAmplitude < 0) levitationAmplitude = 0;
    if (levitationAmplitude > 10) levitationAmplitude = 10;
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
  yield();
}
void handleLookAround() {
  if (server.hasArg("enable")) lookAroundEnabled = server.arg("enable").toInt();
  if (server.hasArg("speed")) {
    lookAroundSpeed = server.arg("speed").toInt();
    if (lookAroundSpeed < 1) lookAroundSpeed = 1;
    if (lookAroundSpeed > 10) lookAroundSpeed = 10;
  }
  if (server.hasArg("amp")) {
    lookAroundAmplitude = server.arg("amp").toInt();
    if (lookAroundAmplitude < 0) lookAroundAmplitude = 0;
    if (lookAroundAmplitude > 15) lookAroundAmplitude = 15;
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
  yield();
}
void handleFps() {
  String json = "{\"fps\":" + String(currentFps) + "}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
  yield();
}
void handleMood() {
  if (server.hasArg("min")) {
    moodMin = server.arg("min").toInt();
    if (moodMin < 1) moodMin = 1;
    if (moodMin > 60) moodMin = 60;
  }
  if (server.hasArg("max")) {
    moodMax = server.arg("max").toInt();
    if (moodMax < moodMin) moodMax = moodMin + 1;
    if (moodMax > 120) moodMax = 120;
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
  yield();
}
void handleManualEmotion() {
  String val = server.arg("emotion");
  Emotion nextEmotion = currentEmotion;
  if (val == "neutral") nextEmotion = EM_NEUTRAL;
  else if (val == "sleep") nextEmotion = EM_SLEEP;
  else if (val == "angry") nextEmotion = EM_ANGRY;
  else if (val == "surprise") nextEmotion = EM_SURPRISE;
  else if (val == "hearts") nextEmotion = EM_HEARTS;
  else if (val == "shy") nextEmotion = EM_SHY;
  else if (val == "happy") nextEmotion = EM_HAPPY;
  else if (val == "sad") nextEmotion = EM_SAD;
  else if (val == "cyclops") nextEmotion = EM_CYCLOPS;
  setEmotionSmooth(nextEmotion);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
  yield();
}
void handleEmotionsList() {
  String json = "{\"emotions\":[\"neutral\",\"sleep\",\"angry\",\"surprise\",\"hearts\",\"shy\",\"happy\",\"sad\",\"cyclops\"]}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
  yield();
}
void handleAutoMood() {
  if (server.hasArg("enable")) autoMoodEnabled = server.arg("enable").toInt();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
  yield();
}

// -------------------------------------------------------------------
// LED Effects
// -------------------------------------------------------------------
void rainbow() {
  int r = abs((stepValue % 512) - 255);
  int g = abs(((stepValue + 170) % 512) - 255);
  int b = abs(((stepValue + 340) % 512) - 255);
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
  stepValue += 5;
  redValue = r; greenValue = g; blueValue = b;
}
void purplePink() {
  int r = 150 + sin(stepValue * 0.05) * 100;
  int g = 0;
  int b = 150 + cos(stepValue * 0.05) * 100;
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
  stepValue++;
  redValue = r; greenValue = g; blueValue = b;
}
void blueCyan() {
  int r = 0;
  int g = 100 + sin(stepValue * 0.05) * 100;
  int b = 200;
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
  stepValue++;
  redValue = r; greenValue = g; blueValue = b;
}
void rainbowCycle() {
  int hue = stepValue % 360;
  float h = hue / 60.0;
  int i = (int)h;
  float f = h - i;
  int q = 255 * (1 - f);
  int t = 255 * f;
  switch (i) {
    case 0: redValue = 255; greenValue = t; blueValue = 0; break;
    case 1: redValue = q; greenValue = 255; blueValue = 0; break;
    case 2: redValue = 0; greenValue = 255; blueValue = t; break;
    case 3: redValue = 0; greenValue = q; blueValue = 255; break;
    case 4: redValue = t; greenValue = 0; blueValue = 255; break;
    default: redValue = 255; greenValue = 0; blueValue = q; break;
  }
  analogWrite(RED_PIN, redValue);
  analogWrite(GREEN_PIN, greenValue);
  analogWrite(BLUE_PIN, blueValue);
  stepValue += 2;
}
void fadeRed() {
  int val = (sin(stepValue * 0.02) * 127 + 128);
  analogWrite(RED_PIN, val);
  analogWrite(GREEN_PIN, 0);
  analogWrite(BLUE_PIN, 0);
  redValue = val; greenValue = 0; blueValue = 0;
  stepValue++;
}
void fadeGreen() {
  int val = (sin(stepValue * 0.02) * 127 + 128);
  analogWrite(RED_PIN, 0);
  analogWrite(GREEN_PIN, val);
  analogWrite(BLUE_PIN, 0);
  redValue = 0; greenValue = val; blueValue = 0;
  stepValue++;
}
void fadeBlue() {
  int val = (sin(stepValue * 0.02) * 127 + 128);
  analogWrite(RED_PIN, 0);
  analogWrite(GREEN_PIN, 0);
  analogWrite(BLUE_PIN, val);
  redValue = 0; greenValue = 0; blueValue = val;
  stepValue++;
}
void police() {
  int period = 20;
  if ((stepValue / period) % 2 == 0) {
    analogWrite(RED_PIN, 255);
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 0);
    redValue = 255; greenValue = 0; blueValue = 0;
  } else {
    analogWrite(RED_PIN, 0);
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 255);
    redValue = 0; greenValue = 0; blueValue = 255;
  }
  stepValue++;
}
void randomColors() {
  if (stepValue % 50 == 0) {
    redValue = random(256);
    greenValue = random(256);
    blueValue = random(256);
    analogWrite(RED_PIN, redValue);
    analogWrite(GREEN_PIN, greenValue);
    analogWrite(BLUE_PIN, blueValue);
  }
  stepValue++;
}
void theater() {
  int period = 8;
  int cycle = (stepValue / period) % 3;
  if (cycle == 0) {
    analogWrite(RED_PIN, 255); analogWrite(GREEN_PIN, 0); analogWrite(BLUE_PIN, 0);
    redValue = 255; greenValue = 0; blueValue = 0;
  } else if (cycle == 1) {
    analogWrite(RED_PIN, 0); analogWrite(GREEN_PIN, 255); analogWrite(BLUE_PIN, 0);
    redValue = 0; greenValue = 255; blueValue = 0;
  } else {
    analogWrite(RED_PIN, 0); analogWrite(GREEN_PIN, 0); analogWrite(BLUE_PIN, 255);
    redValue = 0; greenValue = 0; blueValue = 255;
  }
  stepValue++;
}
void strobe() {
  int period = 10;
  if ((stepValue % 30) < period) {
    analogWrite(RED_PIN, 255);
    analogWrite(GREEN_PIN, 255);
    analogWrite(BLUE_PIN, 255);
    redValue = greenValue = blueValue = 255;
  } else {
    analogWrite(RED_PIN, 0);
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 0);
    redValue = greenValue = blueValue = 0;
  }
  stepValue++;
}
void fire() {
  redValue = random(200, 256);
  greenValue = random(0, 100);
  blueValue = 0;
  analogWrite(RED_PIN, redValue);
  analogWrite(GREEN_PIN, greenValue);
  analogWrite(BLUE_PIN, 0);
  delay(50);
}

void setup() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  analogWrite(RED_PIN, 0);
  analogWrite(GREEN_PIN, 0);
  analogWrite(BLUE_PIN, 0);

  dht.begin();
  delay(1000);

  tft.init();
  tft.setRotation(1);
  eyes.createSprite(100, 40);
  region.createSprite(REGION_W, REGION_H);

  showBootScreen();
  bootLog("Booting CubeRobot v6.0");
  bootLog("Author: @wwxriyy.");
  bootLog("Pins initialized.");
  bootLog("TFT display ready.");
  bootLog("DHT11 on TX ready.");
  bootLog("RGB channels ready.");
  bootLog("LDR sensor on A0 ready.");
  bootLog("Connecting WiFi..."); 
  WiFi.begin(wifi_ssid, wifi_password);
  int dots = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    dots++;
    if (dots % 2 == 0) bootLog("WiFi wait " + String(dots / 2) + "s");
    yield();
  }
  bootLog("WiFi connected");
  bootLog("IP: " + WiFi.localIP().toString());
  delay(1000);

  bootLog("Starting web server...");
  server.on("/red", handleRed);
  bootLog("server.on /red");
  server.on("/green", handleGreen);
  bootLog("server.on /green");
  server.on("/blue", handleBlue);
  bootLog("server.on /blue");
  server.on("/power", handlePower);
  bootLog("server.on /power");
  server.on("/mode", handleMode);
  bootLog("server.on /mode");
  server.on("/status", handleStatus);
  bootLog("server.on /status");
  server.on("/ldr", handleLdr);
  bootLog("server.on /ldr");
  server.on("/dht", handleDht);
  bootLog("server.on /dht");
  server.on("/climate", handleClimate);
  bootLog("server.on /climate");
  server.on("/emotion", handleEmotion);
  bootLog("server.on /emotion");
  server.on("/light", handleLight);
  bootLog("server.on /light");
  server.on("/blink", handleBlink);
  bootLog("server.on /blink");
  server.on("/levitation", handleLevitation);
  bootLog("server.on /levitation");
  server.on("/lookaround", handleLookAround);
  bootLog("server.on /lookaround");
  server.on("/fps", handleFps);
  bootLog("server.on /fps");
  server.on("/mood", handleMood);
  bootLog("server.on /mood");
  server.on("/manual_emotion", handleManualEmotion);
  bootLog("server.on /manual_emotion");
  server.on("/emotions_list", handleEmotionsList);
  bootLog("server.on /emotions_list");
  server.on("/autoswitch", handleAutoMood);
  bootLog("server.on /autoswitch");
  server.begin();
  bootLog("Web server started.");
  bootLog("API: /status /light /fps");
  bootLog("API: /climate /dht");
  bootLog("Site data: RGB, LDR, DHT");
  bootLog("Site data: emotion, FPS");
  bootLog("Loading eyes...");
  delay(500);

  tft.fillScreen(TFT_BLACK);
  currentEyeHeight = eyeHeight;
  renderEyesAtomic();
  updateDhtSensor(true);
  drawDhtOverlay();

  nextBlinkTime = millis() + random(blinkMin, blinkMax);
  lastFpsCalc = millis();
  nextEmotionChange = millis() + random(moodMin * 1000, moodMax * 1000);
}

void loop() {
  server.handleClient();
  yield();
  updateDhtSensor();

  // Sleep state management (auto-sleep by light sensor or forced)
  bool newSleeping = sleeping;
  if (forceEmotion == 1) newSleeping = true;
  else if (forceEmotion == 2) newSleeping = false;
  else if (ldrEnabled) {
    int lightLevel = analogRead(LDR_PIN);
    if (!sleeping && lightLevel < ldrThreshold) newSleeping = true;
    else if (sleeping && lightLevel > ldrThreshold + 100) newSleeping = false;
  }
  if (newSleeping != sleeping) {
    sleeping = newSleeping;
    if (sleeping) {
      isBlinking = false;
      renderEyesAtomic();
    } else {
      renderEyesAtomic();
      nextBlinkTime = millis() + random(blinkMin, blinkMax);
    }
  }

  // Blink
  unsigned long now = millis();
  if (!sleeping && !isBlinking && now >= nextBlinkTime) startBlink();
  if (isBlinking) updateBlink();

  // Eye animations – these are ALWAYS active, regardless of ledPower
  bool allowLook = !sleeping && currentEmotion != EM_SLEEP && currentEmotion != EM_ANGRY;
  bool needRedraw = false;
  if (currentEmotion != EM_NEUTRAL && currentEmotion != EM_SLEEP && currentEmotion != EM_ANGRY) {
    emotionPhase += 0.08;
    lastEyeHeight = -1;
    needRedraw = true;
  }
  if (levitationEnabled) {
    levitationPhase += levitationSpeed * 0.01;
    int newLevOffset = sin(levitationPhase) * levitationAmplitude;
    if (newLevOffset != levitationOffset) {
      levitationOffset = newLevOffset;
      needRedraw = true;
    }
  } else if (levitationOffset != 0) {
    levitationOffset = 0;
    needRedraw = true;
  }
  if (allowLook && lookAroundEnabled) {
    lookPhase += lookAroundSpeed * 0.01;
    int newLookOffset = sin(lookPhase) * lookAroundAmplitude;
    if (newLookOffset != lookOffset) {
      lookOffset = newLookOffset;
      needRedraw = true;
    }
  } else if (lookOffset != 0) {
    lookOffset = 0;
    needRedraw = true;
  }
  if (needRedraw) renderEyesAtomic();

  // Auto emotion change
  if (autoMoodEnabled && forceEmotion == 0 && !sleeping) {
    if (millis() >= nextEmotionChange) {
      isBlinking = false;
      currentEyeHeight = eyeHeight;
      Emotion nextEmotion = EM_NEUTRAL;
      if (currentEmotion == EM_NEUTRAL) nextEmotion = EM_ANGRY;
      else if (currentEmotion == EM_ANGRY) nextEmotion = EM_SURPRISE;
      else if (currentEmotion == EM_SURPRISE) nextEmotion = EM_HEARTS;
      else if (currentEmotion == EM_HEARTS) nextEmotion = EM_SHY;
      else if (currentEmotion == EM_SHY) nextEmotion = EM_HAPPY;
      else if (currentEmotion == EM_HAPPY) nextEmotion = EM_SAD;
      else if (currentEmotion == EM_SAD) nextEmotion = EM_CYCLOPS;
      else if (currentEmotion == EM_CYCLOPS) nextEmotion = EM_SLEEP;
      setEmotionSmooth(nextEmotion);
      nextEmotionChange = millis() + random(moodMin * 1000, moodMax * 1000);
    }
  }

  // FPS calculation
  if (millis() - lastFpsCalc >= 1000) {
    currentFps = frameCount;
    frameCount = 0;
    lastFpsCalc = millis();
  }

  // LED effects – only if power is on
  if (!ledPower) return;
  if (mode == "rainbow") rainbow();
  else if (mode == "purplepink") purplePink();
  else if (mode == "bluecyan") blueCyan();
  else if (mode == "rainbow_cycle") rainbowCycle();
  else if (mode == "fade_red") fadeRed();
  else if (mode == "fade_green") fadeGreen();
  else if (mode == "fade_blue") fadeBlue();
  else if (mode == "police") police();
  else if (mode == "random") randomColors();
  else if (mode == "theater") theater();
  else if (mode == "strobe") strobe();
  else if (mode == "fire") fire();

  delay(20);
}
