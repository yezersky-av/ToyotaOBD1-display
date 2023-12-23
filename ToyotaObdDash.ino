#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include "Vector2D.h"
#include "Utils.h"
#include "logo.h"
#include "pin_config.h"

// Указываем размеры экрана
#define SCREEN_WIDTH  170
#define SCREEN_HEIGHT 320

const char *ssid = "Toyota_OBD1web";

WebSocketsClient webSocket;

Arduino_DataBus *bus = new Arduino_ESP32LCD8(7, 6, 8, 9, 39, 40, 41, 42, 45, 46, 47, 48);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 5 /* RST */, 0 /* rotation */, true /* IPS */, 170 /* width */, 320 /* height */, 35 /* col offset 1 */, 0 /* row offset 1 */, 35 /* col offset 2 */, 0 /* row offset 2 */);

DynamicJsonDocument dataStorage(1024);

int lineSize = 20;
bool connected = false;
const int conColor = 0xfaa0; // #fe4b07
const int disColor = 0x8b8d; // #8C7369

const int interval = 1000 / 30;  // Интервал в миллисекундах (1000 мс / 60 Гц)

unsigned long previousMillis = 0;  // Переменная для хранения времени последнего выполнения кода


Shape displayShape;

Vector2D points[4] = {Vector2D(0,0), Vector2D(169,0),Vector2D(169,319),Vector2D(0,319)};

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

  gfx->drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, connected ? conColor : disColor);
  gfx->drawRoundRect(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, 0, connected ? conColor : disColor);
  gfx->drawRoundRect(1, 1, SCREEN_WIDTH-2, SCREEN_HEIGHT-2, 0, connected ? conColor : disColor);

  // pulse
  renderData(String("pulse ms"), fillWidth(String((const char*)dataStorage["pulse"]), 4, ' '), 4);
  // timing
  renderData(fillWidth(String("timing"), 14, ' '), fillWidth(fillWidth(String((const char*)dataStorage["timing"]), 2), 7, ' '), 4,true);
  //pressure
  renderData(String("pressure kPa"), fillWidth(String((const char*)dataStorage["pressure"]), 4, ' '), 6);
  // temperature
  renderData(fillWidth(String("tmp C"), 14, ' '), fillWidth(fillWidth(String((const char*)dataStorage["temperature"]), 3, '0'), 7, ' '), 6, true);
  // throttle
  renderData(String("throttle"), fillWidth(fillWidth(String((const char*)dataStorage["throttle"]), 2, '0'), 4, ' '), 8);
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
  renderData(String("WS Connecting..."), String(""), 2);

  // Очищаем экран
  // gfx->fillScreen(0);

  Vector2D center = Vector2D(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
  Vector2D direction = Vector2D(SCREEN_HEIGHT/2, SCREEN_HEIGHT/2);
  Vector2D directionBefore = direction;
  Vector2D directionAfter = direction;

  float angle = millis()*0.002;
  
  direction.rotate(angle);
  Vector2D nd = direction.normalize();
  float afterAngle = angle+((PI/4) * (0.8 + abs(nd.x)));

  directionAfter.rotate(afterAngle);

  Vector2D intersection       = displayShape.findIntersection(center, direction);
  Vector2D intersectionAfter  = displayShape.findIntersection(center, directionAfter);

/*
  gfx->drawLine(center.x, center.y, intersection.x, intersection.y, 0xFFFF);
  gfx->drawLine(center.x, center.y, intersectionAfter.x, intersectionAfter.y, conColor);
*/

  if(intersection.x == intersectionAfter.x){
    if(intersection.x == 0){
      gfx->drawLine(0, intersection.y, 0, intersectionAfter.y, conColor);
      gfx->drawLine(1, intersection.y, 1, intersectionAfter.y, conColor);
    } else {
      gfx->drawLine(169, intersection.y, 169, intersectionAfter.y, conColor);
      gfx->drawLine(168, intersection.y, 168, intersectionAfter.y, conColor);
    }
  } else if (intersection.y == intersectionAfter.y) {
    if(intersection.y == 0){
      gfx->drawLine(intersection.x, 0, intersectionAfter.x, 0, conColor);
      gfx->drawLine(intersection.x, 1, intersectionAfter.x, 1, conColor);
    } else {
      gfx->drawLine(intersection.x, 319, intersectionAfter.x, 319, conColor);
      gfx->drawLine(intersection.x, 318, intersectionAfter.x, 318, conColor);
    }
  } else if(intersection.x == 0 && intersectionAfter.y == 0) {
      gfx->drawLine(0, 0, 0, intersection.y, conColor);
      gfx->drawLine(1, 0, 1, intersection.y, conColor);

      gfx->drawLine(0, 0, intersectionAfter.x, 0, conColor);
      gfx->drawLine(0, 1, intersectionAfter.x, 1, conColor);

  } else if(intersection.y == 0 && intersectionAfter.x == 169) {
      gfx->drawLine(intersection.x, 0, SCREEN_WIDTH, 0, conColor);
      gfx->drawLine(intersection.x, 1, SCREEN_WIDTH, 1, conColor);

      gfx->drawLine(168, 0, 168, intersectionAfter.y, conColor);
      gfx->drawLine(169, 0, 169, intersectionAfter.y, conColor);
  } else if(intersection.x == 169 && intersectionAfter.y == 319) {
      gfx->drawLine(168, intersection.y, 168, SCREEN_HEIGHT, conColor);
      gfx->drawLine(169, intersection.y, 169, SCREEN_HEIGHT, conColor);

      gfx->drawLine(intersectionAfter.x, 318, SCREEN_WIDTH, 318, conColor);
      gfx->drawLine(intersectionAfter.x, 319, SCREEN_WIDTH, 319, conColor);
  } else if(intersection.y == 319 && intersectionAfter.x == 0) {
      gfx->drawLine(0, 318, intersection.x, 318, conColor);
      gfx->drawLine(0, 319, intersection.x, 319, conColor);

      gfx->drawLine(0, intersectionAfter.y, 0, SCREEN_HEIGHT, conColor);
      gfx->drawLine(1, intersectionAfter.y, 1, SCREEN_HEIGHT, conColor);
  }
}


void setup() {
  for(int i = 0; i < 4; i++){
    displayShape.addVector(points[i]);
  }
  displayShape.createSegments();

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
  gfx->setTextColor(conColor, BLACK);
  gfx->draw16bitRGBBitmap(0, 1, (uint16_t *)gImage_logo, SCREEN_WIDTH, SCREEN_HEIGHT);

  delay(1500);

  WiFi.begin(ssid, "");
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }
  
  webSocket.begin("192.168.4.1", 80, "/ws");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(750);

  gfx->fillScreen(BLACK); // REMOVE AFTER DEBUG

  dataStorage["pulse"] = dataStorage["temperature"] = dataStorage["speed"] = "---";
  dataStorage["rpm"] = dataStorage["pressure"] = dataStorage["afr"] = "----";
  dataStorage["timing"] = dataStorage["throttle"] = "--";

  // updateDisplay();
}

void loop() {
  unsigned long currentMillis = millis();

  if(connected){
    gfx->setTextColor(conColor, BLACK);
  } else {
    gfx->setTextColor(disColor);
  }

  webSocket.loop();
  if (currentMillis - previousMillis >= interval) {
    
    updateDisplay();
    if(!connected){
      displayConnectionLost();
    }

    previousMillis = currentMillis;
  }

}
