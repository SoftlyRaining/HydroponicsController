#define MINUTE UINT32_C(60)
#define HOUR UINT32_C(3600)
#define DAY UINT32_C(86400)

// configuration
//#define SCHEDULE_TEST // if defined, the clock will be greatly sped up

#define LIGHT_HOURS 14
#define SOLAR_NOON 13 // 1300 / 1pm (center of 9-5 working day)
#define SUNRISE (SOLAR_NOON - LIGHT_HOURS/2) // ignoring underflow risk
#define SUNSET (SUNRISE + LIGHT_HOURS)

#define LOG_INTERVAL_MINUTES 5

#define LIGHT_OUTLET  1

// pinout definitions
#define PIN_BUZZER            2
#define PIN_LIGHT_HEARTBEAT   LED_BUILTIN
#define PIN_LIGHT_ALARM       4

#define PIN_LCD_RS            22
#define PIN_LCD_EN            24
#define PIN_LCD_D4            26
#define PIN_LCD_D5            28
#define PIN_LCD_D6            30
#define PIN_LCD_D7            32

#define PIN_DHT               34
#define PIN_SD_CHIPSELECT     53

#define PIN_OUTLET_1          37
#define PIN_OUTLET_2          35
#define PIN_OUTLET_3          33
#define PIN_OUTLET_4          31
#define PIN_OUTLET_5          29
#define PIN_OUTLET_6          27
#define PIN_OUTLET_7          25
#define PIN_OUTLET_8          23

// includes
#include <avr/wdt.h>
#include <Wire.h> // used by RTClb (I2C?)
#include "DHT.h"
#include "RTClib.h"

#define HEARTBEAT wdt_reset()

class Display {
public:
  static void init();
  static void poll();
  static void showError(const char* message);
  
private:
  static void update();
};

class Log {
public:
  enum Level { INFO, WARN, ERROR, FATAL };

  static void init();
  static void poll();
  static void logString(Level level, String message);
  static bool haveSeenError() { return errorSeen; }
  
private:
  static void update();

  static bool errorSeen;
};

class Pumps {
public:
  static void init();
  static void poll();

  static uint8_t getCurrentCycle(uint8_t index);
  static uint32_t getNextEvent(uint8_t index);
};

RTC_DS3231 g_rtc;
DHT g_dht(PIN_DHT, DHT22);

// global state
DateTime g_now;
float g_airTemp;
float g_airHumidity;

void pollSensorState() {
  // TODO update sensors: water temp, water EC, water pH, water level (x4), soil humidity (x3)

  // DHT updates about once every 2 seconds, and update takes 250ms. (otherwise it does nothing, seemingly)
  g_airTemp = g_dht.readTemperature();
  g_airHumidity = g_dht.readHumidity();
}

void pollLights() {
  static bool lightsOn = false;

  uint8_t hour = g_now.hour();
  if (lightsOn) {
    if (hour < SUNRISE || hour >= SUNSET) {
      lightsOn = false;
      Log::logString(Log::INFO, "Sunset");
      setOutlet(LIGHT_OUTLET, lightsOn);
    }
  } else {
    if (hour >= SUNRISE && hour < SUNSET) {
      lightsOn = true;
      Log::logString(Log::INFO, "Sunrise");
      setOutlet(LIGHT_OUTLET, lightsOn);
    }
  }
}

void setOutlet(int number, bool on) {
  Log::logString(Log::INFO, "Setting outlet " + String(number) + " = " + String(on));
  static const int pinMap[8] = {
    PIN_OUTLET_1,
    PIN_OUTLET_2,
    PIN_OUTLET_3,
    PIN_OUTLET_4,
    PIN_OUTLET_5,
    PIN_OUTLET_6,
    PIN_OUTLET_7,
    PIN_OUTLET_8,
  };
  digitalWrite(pinMap[number-1], on ? LOW : HIGH);
}

void setup() {
  // enable watchdog timer
  HEARTBEAT;
  wdt_enable(WDTO_2S); // 2 seconds

  // configure pins
  HEARTBEAT;
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LIGHT_HEARTBEAT, OUTPUT);
  pinMode(PIN_LIGHT_ALARM, OUTPUT);
  pinMode(PIN_OUTLET_1, OUTPUT);
  pinMode(PIN_OUTLET_2, OUTPUT);
  pinMode(PIN_OUTLET_3, OUTPUT);
  pinMode(PIN_OUTLET_4, OUTPUT);
  pinMode(PIN_OUTLET_5, OUTPUT);
  pinMode(PIN_OUTLET_6, OUTPUT);
  pinMode(PIN_OUTLET_7, OUTPUT);
  pinMode(PIN_OUTLET_8, OUTPUT);

  digitalWrite(PIN_OUTLET_1, HIGH); // high means relay off
  digitalWrite(PIN_OUTLET_2, HIGH);
  digitalWrite(PIN_OUTLET_3, HIGH);
  digitalWrite(PIN_OUTLET_4, HIGH);
  digitalWrite(PIN_OUTLET_5, HIGH);
  digitalWrite(PIN_OUTLET_6, HIGH);
  digitalWrite(PIN_OUTLET_7, HIGH);
  digitalWrite(PIN_OUTLET_8, HIGH);

  // lamp test (briefly illuminate all to ensure they're connected and functioning)
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LIGHT_HEARTBEAT, LOW);
  digitalWrite(PIN_LIGHT_ALARM, HIGH);
  delay(250);
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LIGHT_HEARTBEAT, LOW);
  digitalWrite(PIN_LIGHT_ALARM, LOW);

  // initialize hardware, modules
  HEARTBEAT;
  Display::init();
  Log::init();
  if (!g_rtc.begin())
    fatalError("begin RTC error");
  delay(500); // come to our senses and get a sane time
  HEARTBEAT;
  g_now = g_rtc.now();
  Pumps::init();
  
  Log::logString(Log::WARN, "RESET");
#ifdef SCHEDULE_TEST
  Log::logString(Log::WARN, "Schedule test enabled.");
#endif
}

void loop() {
  HEARTBEAT;

  // update clock time
#ifdef SCHEDULE_TEST
  g_now = g_now + TimeSpan(MINUTE); // fast forward - one minute per tick
#else  
  g_now = g_rtc.now();
#endif
  
  pollSensorState();
  
  Display::poll();
  Log::poll();
  Pumps::poll();
  pollLights();

  HEARTBEAT;
  delay(500); // ms
}

void fatalError(const char* message) {
  // activate red light & alarm buzzer
  digitalWrite(PIN_BUZZER, HIGH);
  digitalWrite(PIN_LIGHT_ALARM, HIGH);
  
  Log::logString(Log::FATAL, message);
  Display::showError(message); // display error on LCD

  // wait for watchdog to reset us (no heartbeat!) while drawing attention
  while (true) {
    delay(250);
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LIGHT_ALARM, LOW);
    delay(250);
    digitalWrite(PIN_BUZZER, HIGH);
    digitalWrite(PIN_LIGHT_ALARM, HIGH);
  }
}
