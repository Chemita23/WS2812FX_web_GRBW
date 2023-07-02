#include <WS2812FX.h>
#include "rain.h"
#include "Popcorn.h"
#include "Heartbeat.h"
#include "TriFade.h"

#ifdef ARDUINO_ARCH_ESP32
  #include <WiFi.h>
  #include <WebServer.h>
  #define WEB_SERVER WebServer
  #define ESP_RESET ESP.restart()
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #define WEB_SERVER ESP8266WebServer
  #define ESP_RESET ESP.reset()
#endif

extern const char index_html[];
extern const char main_js[];
int led = 2;
#define WIFI_SSID "MOVISTAR_643A"
#define WIFI_PASSWORD "Mateos23"

#define STATIC_IP                       // uncomment for static IP, set IP below
#ifdef STATIC_IP
  IPAddress ip(192,168,1,252);
  IPAddress gateway(192,168,1,1);
  IPAddress subnet(255,255,255,0);
#endif

// QUICKFIX...See https://github.com/esp8266/Arduino/issues/263
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define LED_PIN 23                       // ESP8266 = 14, 7 = GPIO13, 2=GPIO2
#define LED_COUNT 117

#define WIFI_TIMEOUT 40000              // checks WiFi every ...ms. Reset after this time, if WiFi cannot reconnect.
#define HTTP_PORT 80

#define DEFAULT_COLOR 0xffcc5500
#define DEFAULT_BRIGHTNESS 150
#define DEFAULT_SPEED 2905
#define DEFAULT_MODE FX_MODE_TWINKLE_FADE_RANDOM

unsigned long auto_last_change = 0;
unsigned long last_wifi_check_time = 0;
String modes = "";
uint8_t myModes[] = {}; // *** optionally create a custom list of effect/mode numbers
boolean auto_cycle = false;

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);
WebServer server(HTTP_PORT); // ESP8266WebServer server(HTTP_PORT);

void setup(){
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\nStarting...");


  Serial.println("WS2812FX setup");
  ws2812fx.init();
  ws2812fx.setMode(FX_MODE_TRIFADE);
  //ws2812fx.setMode(FX_MODE_MULTI_STROBE); 
  ws2812fx.setCustomMode(F("Rain"),rain);
  ws2812fx.setCustomMode(F("Popcorn"),popcorn);
  ws2812fx.setCustomMode(F("Heartbeat"),heartbeat);
  ws2812fx.setCustomMode(F("TriFade"),triFade);
  ws2812fx.setColor(DEFAULT_COLOR);
  ws2812fx.setSpeed(DEFAULT_SPEED);
  ws2812fx.setBrightness(200);
  ws2812fx.start();
  ws2812fx.service();
  delay(1500);
  Serial.println("Wifi setup");
  modes.reserve(5000);
  modes_setup();  
  wifi_setup();
 
  Serial.println("HTTP server setup");
  server.on("/", srv_handle_index_html);
  server.on("/main.js", srv_handle_main_js);
  server.on("/modes", srv_handle_modes);
  server.on("/set", srv_handle_set);
  server.onNotFound(srv_handle_not_found);
  server.begin();
  Serial.println("HTTP server started.");
  Serial.println("ready!");
}


void loop() {
  unsigned long now = millis();
  server.handleClient();
  ws2812fx.service();

  if(now - last_wifi_check_time > WIFI_TIMEOUT) {
    Serial.print("Checking WiFi... ");
    if(WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection lost. Reconnecting...");
      ws2812fx.setColor(DEFAULT_COLOR);
      ws2812fx.setSpeed(616);      
      ws2812fx.setMode(FX_MODE_MULTI_STROBE);
      wifi_setup();
    } else {
      Serial.println("OK");
    }
    last_wifi_check_time = now;
  }

  if(auto_cycle && (now - auto_last_change > 10000)) { // cycle effect mode every 10 seconds
    uint8_t next_mode = (ws2812fx.getMode() + 1) % ws2812fx.getModeCount();
    if(sizeof(myModes) > 0) { // if custom list of modes exists
      for(uint8_t i=0; i < sizeof(myModes); i++) {
        if(myModes[i] == ws2812fx.getMode()) {
          next_mode = ((i + 1) < sizeof(myModes)) ? myModes[i + 1] : myModes[0];
          break;
        }
      }
    }
    ws2812fx.setMode(next_mode);
    Serial.print("mode is "); Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
    auto_last_change = now;
  }

}

/*
 * Connect to WiFi. If no connection is made within WIFI_TIMEOUT, ESP gets resettet.
 */
void wifi_setup() {
  //digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.mode(WIFI_STA);
  #ifdef STATIC_IP  
    WiFi.config(ip, gateway, subnet);
  #endif

  unsigned long connect_start = millis();
  while(WiFi.status() != WL_CONNECTED) {
    //delay(70);
    //Serial.print(".");
    ws2812fx.service();
    //digitalWrite(led, LOW);
    //delay(20);
    //digitalWrite(led, HIGH); 

    if(millis() - connect_start > WIFI_TIMEOUT) {
      Serial.println();
      Serial.print("Tried ");
      Serial.print(WIFI_TIMEOUT);
      Serial.print("ms. Resetting ESP now.");
      ESP.restart();  // ESP.reset(); //ESP8266
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  ws2812fx.setMode(DEFAULT_MODE);
  ws2812fx.setColor(DEFAULT_COLOR);
  ws2812fx.setSpeed(100);
  ws2812fx.setBrightness(DEFAULT_BRIGHTNESS);
  ws2812fx.start();
  ws2812fx.service();
}


/*
 * Build <li> string for all modes.
 */
void modes_setup() {
  modes = "";
  uint8_t num_modes = sizeof(myModes) > 0 ? sizeof(myModes) : ws2812fx.getModeCount();
  for(uint8_t i=0; i < num_modes; i++) {
    uint8_t m = sizeof(myModes) > 0 ? myModes[i] : i;
    modes += "<li><a href='#'>";
    modes += ws2812fx.getModeName(m);
    modes += "</a></li>";
  }
}

/* #####################################################
#  Webserver Functions
##################################################### */

void srv_handle_not_found() {
  server.send(404, "text/plain", "File Not Found");
}

void srv_handle_index_html() {
  server.send_P(200,"text/html", index_html);
}

void srv_handle_main_js() {
  server.send_P(200,"application/javascript", main_js);
}

void srv_handle_modes() {
  server.send(200,"text/plain", modes);
}

void srv_handle_set() {
  for (uint8_t i=0; i < server.args(); i++){
    if(server.argName(i) == "c") {
      long tmp = (long) strtol(server.arg(i).c_str(), NULL, 10);
      if(tmp >= 0x00000000 && tmp <= 0xFFFFFFFF) {
        Serial.print("Color  ");Serial.println(tmp);
        ws2812fx.setColor(tmp);
      }
      Serial.print("i: ");Serial.println(i);
    }

    if(server.argName(i) == "m") {
      uint8_t tmp = (uint8_t) strtol(server.arg(i).c_str(), NULL, 10);
      ws2812fx.setMode(tmp % ws2812fx.getModeCount());
      Serial.print("mode is "); Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
      Serial.print("mode is "); Serial.println(tmp % ws2812fx.getModeCount());
    }

    if(server.argName(i) == "b") {
        uint8_t tmp = (uint8_t) strtol(server.arg(i).c_str(), NULL, 10);
        ws2812fx.setBrightness(tmp);

      Serial.print("brightness is "); Serial.println(ws2812fx.getBrightness());
    }

  

        if(server.argName(i) == "x") {
        uint16_t tmp = (uint16_t) strtol(server.arg(i).c_str(), NULL, 10);
        ws2812fx.setSpeed(tmp);
        Serial.print("speedx tmp is "); Serial.println(tmp);
        Serial.print("speedx is "); Serial.println(ws2812fx.getSpeed());
    }

    if(server.argName(i) == "a") {
      if(server.arg(i)[0] == '-') {
        auto_cycle = false;
      } else {
        auto_cycle = true;
        auto_last_change = 0;
      }
    }
  }
  server.send(200, "text/plain", "OK");
}
