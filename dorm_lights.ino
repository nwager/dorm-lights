/*
 * Sources:
 * https://randomnerdtutorials.com/esp8266-nodemcu-web-server-slider-pwm/
 * https://randomnerdtutorials.com/esp8266-ota-updates-with-arduino-ide-over-the-air/
 * Google Color Picker (for the rainbow gradient)
 */

#define FASTLED_ESP8266_RAW_PIN_ORDER

// Hostname defaults to esp8266-[ChipID]
// ArduinoOTA.setHostname("myesp8266");

// No authentication by default
// ArduinoOTA.setPassword((const char *)"123");

#include "rawhtml.h"
#include "config.h"

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <FastLED.h>

const char* ssid = STASSID;
const char* password = STAPSK;

#define TIME_OFFSET ((-7)*3600)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

#define STRIP D3
#define CLOCK  D4
#define NUM_LEDS 100
#define NUM_CLOCK_LEDS 11
CRGB leds[NUM_LEDS];
CRGB clock_leds[NUM_CLOCK_LEDS];

uint8_t led_state = 0;

uint8_t g_hue = 0;
uint8_t g_sat = 255;
uint8_t g_val = 128;

uint8_t rainbow_speed = 50;
const uint16_t RB_MIN_MS = 2;
const uint16_t RB_MAX_MS = 100;

bool led_enable = true;
bool clock_enable = true;

#define GET_LEFT(c) (((c)&0xFF0000)>>16) // get left byte from hexcode
#define GET_MID(c) (((c)&0x00FF00)>>8)  // get middle byte from hexcode
#define GET_RIGHT(c) ((c)&0x0000FF)       // get right byte from hexcode

const int output = 2;

const char* PARAM_INPUT = "value";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Replaces placeholders
String processor(const String& var) {
  //Serial.println(var);
  String res;
  if (var == "STATE_VAL") {
    res = String(led_state);
  } else if (var == "HUE_VAL") {
    res = String(g_hue);
  } else if (var == "SAT_VAL") {
    res = String(g_sat);
  } else if (var == "VAL_VAL") {
    res = String(g_val);
  } else if (var == "LED_TOGGLE") {
    res = led_enable ? "1" : "0";
  } else if (var == "CLOCK_TOGGLE") {
    res = clock_enable ? "1" : "0";
  } else if (var == "RSPEED_VAL") {
    res = String(map(rainbow_speed, RB_MAX_MS, RB_MIN_MS, 0, 100));
  } else {
    res = "";
  }
  return res;
}

void setup() {
  /********************************************/
  /* DON'T TOUCH*/ setupOTA(); /* DON'T TOUCH */
  /********************************************/

  FastLED.addLeds<WS2811, STRIP, BRG>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.addLeds<WS2812B, CLOCK, GRB>(clock_leds, NUM_CLOCK_LEDS).setCorrection(TypicalSMD5050);
  FastLED.clear();
  FastLED.show();

  // called every time page is loaded
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
  server.on("/hex", HTTP_GET, [] (AsyncWebServerRequest * request) {
    // GET input1 value on <ESP_IP>/hex?value=<inputMessage>
    String inputMessage;
    if (request->hasParam(PARAM_INPUT)) {
      uint32_t hex = (inputMessage = request->getParam(PARAM_INPUT)->value()).toInt();
      g_hue = GET_LEFT(hex);
      g_sat = GET_MID(hex);
      g_val = GET_RIGHT(hex);
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  server.on("/state", HTTP_GET, [] (AsyncWebServerRequest * request) {
    if (request->hasParam(PARAM_INPUT)) {
      String msg = request->getParam(PARAM_INPUT)->value();
      led_state = msg.toInt();
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/speed", HTTP_GET, [] (AsyncWebServerRequest * request) {
    if (request->hasParam(PARAM_INPUT)) {
      String msg = request->getParam(PARAM_INPUT)->value();
      uint8_t percent = msg.toInt();
      rainbow_speed = map(percent, 0, 100, RB_MAX_MS, RB_MIN_MS);
    }
    request->send(200, "text/plain", "OK");
  });

  // clock toggle
  server.on("/toggleclock", HTTP_GET, [] (AsyncWebServerRequest * request) {
    if (request->hasParam(PARAM_INPUT)) {
      String msg = request->getParam(PARAM_INPUT)->value();
      clock_enable = msg == "1";
    }
    request->send(200, "text/plain", "OK");
  });

  // led toggle
  server.on("/toggleled", HTTP_GET, [] (AsyncWebServerRequest * request) {
    if (request->hasParam(PARAM_INPUT)) {
      String msg = request->getParam(PARAM_INPUT)->value();
      led_enable = msg == "1";
    }
    request->send(200, "text/plain", "OK");
  });

  // Start server
  server.begin();

  timeClient.begin();
  timeClient.setTimeOffset(TIME_OFFSET);

}

void loop() {
  /*****************************************************/
  /* DON'T TOUCH*/ ArduinoOTA.handle(); /* DON'T TOUCH */
  /*****************************************************/

  if (led_enable) {
    updateLED();
  } else {
    turn_off(leds, NUM_LEDS);
  }
  
  updateClock();

  delayOTA(5);
}

// animations for led strips depending on state
void updateLED() {
  switch (led_state) {
    case 0: // normal color-picker
      fill_solid(leds, NUM_LEDS, CHSV(g_hue, g_sat, g_val));
      break;
    case 1: // rainbow cycle
      rainbow_cycle();
      break;
    case 2: // noise
      noise();
    default:
      break;
  }
  FastLED.show();
}

void rainbow_cycle() {
  static uint8_t hue = 0;
  static long prev = 0;
  
  long curr = millis();
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue+i, g_sat, g_val);
  }
  // if rainbow_speed ms have elapsed
  if (curr - prev >= rainbow_speed) {
    hue++;
    prev = curr;
  }
}

void noise() {
  static long offset = 0;
  int scale = 100;
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t noise_val = inoise8((i*scale), offset);
    leds[i] = CHSV(noise_val+triwave8(offset/3), 255, g_val);
  }
  offset++;
}


void updateClock() {
  if (!clock_enable) {
    turn_off(clock_leds, NUM_CLOCK_LEDS);
    return;
  }
  static uint8_t hue_start = 0;

  timeClient.update();

  uint8_t h = (uint8_t)timeClient.getHours() % 12;
  uint8_t m = (uint8_t)timeClient.getMinutes();
  uint8_t s = (uint8_t)timeClient.getSeconds();

  // 10 9 8 7  6  5 4 3 2 1 0
  //  h h h h  :  m m m m m m
  uint8_t hue_step = 5;
  uint8_t on_array[NUM_CLOCK_LEDS];
  for (int i = 0; i < NUM_CLOCK_LEDS; i++) {
    uint8_t is_on = 0;
    if (i < 6) {
      is_on = bitRead(m, i);
    } else if (i == 6) {
      is_on = s & 1; // alternates every second
    } else {
      is_on = bitRead(h, i - 7);
    }
    on_array[i] = is_on;
  }

  for (int m = 0; m < 6; m++) {
    clock_leds[m] = CHSV(hue_start + (m * hue_step), 255, 64 * on_array[m]);
  }
  for (int h = 7; h < 11; h++) {
    clock_leds[h] = CHSV(hue_start + (h * hue_step), 255, 64 * on_array[h]);
  }
  clock_leds[6] = CHSV(hue_start + (6 * hue_step) + 128, 255, 64 * on_array[6]);

  FastLED.show();
  EVERY_N_MILLISECONDS(50) {
    hue_start++;
  }
}

void turn_off(CRGB* l, int n) {
  fill_solid(l, n, CRGB(0,0,0));
  FastLED.show();
}




// delay approx. ms, call handle() every 1ms
void delayOTA(int ms) {
  for (int i = 0; i < ms; i++) {
    ArduinoOTA.handle();
    delay(1);
  }
}

// DON'T TOUCH!!!!!!!!!!!!
// I'M SERIOUS!!!!!!!!!!!!
// GET AWAY FROM HERE!!!!!
void setupOTA() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
  //    Serial.println("Connection Failed! Rebooting...");
  //    delay(5000);
  //    ESP.restart();
  //  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}
