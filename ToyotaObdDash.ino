#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "logo.h"
#include "pin_config.h"
#include "Utils.h"

const char *ssid = "Toyota_OBD1web";

WebSocketsClient webSocket;

Arduino_DataBus *bus = new Arduino_ESP32LCD8(7, 6, 8, 9, 39, 40, 41, 42, 45, 46, 47, 48);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 5 /* RST */, 0 /* rotation */, true /* IPS */, 170 /* width */, 320 /* height */, 35 /* col offset 1 */, 0 /* row offset 1 */, 35 /* col offset 2 */, 0 /* row offset 2 */);

DynamicJsonDocument dataStorage(1024);

int lineSize = 20;
bool connected = false;
const int conColor = 0xfaa0; // #fe4b07
const int disColor = 0x8b8d; // #8C7369

void renderData(String label, String text, int row, bool right = false){
  int dX = right ? 75 : 0;
  gfx->setTextSize(1);
  gfx->setCursor(5 + dX, (lineSize * row)+10);
  gfx->println(label);
  gfx->setTextSize(2);
  gfx->setCursor(5 + dX, lineSize * (row+1));
  gfx->println(text);
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  String sPayload = String((char *)payload);
  deserializeJson(dataStorage, sPayload);

  if (type == WStype_TEXT && dataStorage.containsKey("id") && dataStorage["id"] == 13) {
    sanitizeAndStoreData();
  }

  switch (type) {
    case WStype_DISCONNECTED:
      connected = false;
      break;
    case WStype_CONNECTED:
      connected = true;
      gfx->fillScreen(BLACK);
      webSocket.sendTXT("sactive:4");
      break;
  }
}

void sanitizeAndStoreData() {
  String value = dataStorage["value"];

  while (value.indexOf("...") != -1) {
    value.replace("...", "..");
  }

  value.replace(".."," | ");

  String substrings[10];
  int substringCount = 0;
  splitString(value, "<br>", substrings, substringCount);

  if (substringCount >= 10) {
    for (int i = 0; i < substringCount; i++) {
      int indexOfSeparator = substrings[i].indexOf(" | ");
      if (indexOfSeparator != -1) {
        substrings[i].remove(0, indexOfSeparator + 3);
      }

      // Остальной код для удаления лишних данных из substrings[i]
      substrings[i].trim();
    }

    substrings[1].remove(substrings[1].length() - 3, 3); // pulse
    substrings[2].remove(substrings[2].length() - 3, 3); // timing
    substrings[6].remove(substrings[6].length() - 4, 4); // temperature
    substrings[4].remove(substrings[4].length() - 4, 4); // rpm
    substrings[5].remove(substrings[5].length() - 4, 4); // pressure
    substrings[7].remove(substrings[7].length() - 3, 3); // throttle
    substrings[8].remove(substrings[8].length() - 5, 5); // speed
    substrings[9].remove(substrings[9].length() - 2, 2); // afr

    dataStorage["pulse"] = substrings[1];
    dataStorage["timing"] = substrings[2];
    dataStorage["rpm"] = substrings[4];
    dataStorage["pressure"] = substrings[5];
    dataStorage["temperature"] = substrings[6];
    dataStorage["throttle"] = substrings[7];
    dataStorage["speed"] = substrings[8];
    dataStorage["afr"] = substrings[9];
  }
}

void updateDisplay() {
  gfx->flush();
  gfx->setTextSize(2);

  if(connected){
    gfx->setTextColor(conColor, BLACK);
  } else {
    gfx->setTextColor(disColor);
  }

  gfx->drawRoundRect(0, 0, 170, 320, 0, connected ? conColor : disColor);
  gfx->drawRoundRect(1, 1, 168, 318, 0, connected ? conColor : disColor);

  // pulse
  renderData(String("pulse ms"), String((const char*)dataStorage["pulse"]), 4);
  // timing
  renderData(fillWidth(String("timing"), 14, ' '), fillWidth(fillWidth(String((const char*)dataStorage["timing"]), 2), 7, ' '), 4,true);
  //pressure
  renderData(String("pressure kPa"), String((const char*)dataStorage["pressure"]), 6);
  // temperature
  renderData(fillWidth(String("tmp C"), 14, ' '), fillWidth(fillWidth(String((const char*)dataStorage["temperature"]), 3, '0'), 7, ' '), 6, true);
  // throttle
  renderData(String("throttle"), fillWidth(String((const char*)dataStorage["throttle"]), 2, '0'), 8);
  // afr
  renderData(fillWidth(String("afr"), 14, ' '), fillWidth(String((const char*)dataStorage["afr"]), 7, ' '), 8, true);

  //speed
  gfx->setTextSize(1);
  gfx->setCursor(125, (lineSize * 1)+17);
  gfx->println(F("km/h"));
  gfx->setTextSize(5);
  gfx->setCursor(35, (lineSize * 0)+7);
  gfx->println(fillWidth(String((const char*)dataStorage["speed"]), 3, '0'));

  // rpm
  gfx->setTextSize(1);
  gfx->setCursor(7, (lineSize * 13)+7);
  gfx->println(F("rpm"));
  gfx->setTextSize(5);
  gfx->setCursor(7, (lineSize * 14)-3);
  gfx->println(fillWidth(String((const char*)dataStorage["rpm"]), 4, '0'));
}

void displayConnectionLost() {
  const int centerX = 84;       // X-координата центра спиннера
  const int centerY = 159;      // Y-координата центра спиннера

  renderData(String("Connecting..."), String(""), 2);

  gfx->drawArc(centerX, centerY, 64, 65, millis(), millis() + (360/3), conColor);
  gfx->drawArc(centerX, centerY, 64, 65, millis() + (360/3), millis() + (360/3)*2, disColor);
  gfx->drawArc(centerX, centerY, 64, 65, millis() + (360/3)*2, millis(), BLACK);
}

void setup() {
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);

  ledcSetup(0, 2000, 8);
  ledcAttachPin(PIN_LCD_BL, 0);
  ledcWrite(0, 255);

  pinMode(0, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.println(F("Hello T-Display-S3"));

  gfx->begin();
  gfx->fillScreen(BLACK);
  gfx->setRotation(0);
  gfx->setTextColor(0xfaa0, BLACK);
  gfx->draw16bitRGBBitmap(0, 1, (uint16_t *)gImage_logo, 170, 320);

  delay(1500);

  WiFi.begin(ssid, "");
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }
  
  webSocket.begin("192.168.4.1", 80, "/ws");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(750);

  gfx->fillScreen(BLACK);

  dataStorage["pulse"] = dataStorage["temperature"] = dataStorage["speed"] = "---";
  dataStorage["timing"] = dataStorage["throttle"] = "--";
  dataStorage["rpm"] = dataStorage["pressure"] = dataStorage["afr"] = "----";

  updateDisplay();
}

void loop() {
  webSocket.loop();
  updateDisplay();
  if(!connected){
    displayConnectionLost();
  }
}
