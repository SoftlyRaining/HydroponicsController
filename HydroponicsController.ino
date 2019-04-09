// configuration
#define LIGHT_HOURS 14
#define SOLAR_NOON 13 // 1300 / 1pm (center of 9-5 working day)
#define SUNRISE (SOLAR_NOON - LIGHT_HOURS/2) // ignoring underflow risk
#define SUNSET (SUNRISE + LIGHT_HOURS)

#define WATER_CYCLE_COUNT 5 // number of floods per day. These are spaced out during the "daytime" period
#define PUMP_ON_MINUTES 15 // number of minutes to turn on pump for each flood

#define LOG_INTERVAL_MINUTES 5

#define LIGHT_OUTLET  1
#define PUMP_OUTLET   2

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

#define PIN_OUTLET_1          23
#define PIN_OUTLET_2          25
#define PIN_OUTLET_3          27
#define PIN_OUTLET_4          29
#define PIN_OUTLET_5          31
#define PIN_OUTLET_6          33
#define PIN_OUTLET_7          35
#define PIN_OUTLET_8          37

// includes
#include <avr/wdt.h>
#include <Wire.h> // used by RTClb (I2C?)

#include "DHT.h"
#include "RTClib.h"

#define HEARTBEAT wdt_reset()

class Display {
public:
  static void Init();
  static void Poll();
  static void ShowError(const char* message);
  
private:
  static void Update();
};

class Log {
public:
  enum Level { FATAL, ERROR, WARN, INFO };

  static void Init();
  static void Poll();
  static void LogString(Level level, String message);
  
private:
  static void Update();
};

class Pumps {
public:
  static void Init();
  static void Poll();

  static bool IsFloodingNow();
  static int GetFloodCycle();
  static uint32_t GetNextPumpEventTimestamp();

private:
  static void StartFlood();
  static void EndFlood();

  static int CalculateCurrentCycle();
  static uint32_t CalculateNextFlood();

  static bool flooding;
  static uint8_t currentCycle;
  static uint32_t nextStateChangeTime;
  
  static uint32_t firstCycleStart; // time after midnight in seconds
  static uint32_t cycleDuration;
};

RTC_DS3231 g_rtc;
DHT g_dht(PIN_DHT, DHT22);

// global state
DateTime g_now;
float g_airTemp;
float g_airHumidity;

void PollSensorState() {
  // TODO update sensors: water temp, water EC, water pH, water level (x4), soil humidity (x3)

  // TODO each DHT call takes ~250ms - too long for every tick?
  g_airTemp = g_dht.readTemperature();
  g_airHumidity = g_dht.readHumidity();
}

void PollLights() {
  static bool lightsOn = false;

  uint8_t hour = g_now.hour();
  if (lightsOn) {
    if (hour < SUNRISE || hour >= SUNSET) {
      lightsOn = false;
      Log::LogString(Log::INFO, "Sunset");
    }
  } else {
    if (hour >= SUNRISE && hour < SUNSET) {
      lightsOn = true;
      Log::LogString(Log::INFO, "Sunrise");
    }
  }
  
  SetOutlet(LIGHT_OUTLET, lightsOn);
}

void SetOutlet(int number, bool on) {
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
  digitalWrite(pinMap[number-1], on ? HIGH : LOW);
}

void setup() {
  // enable watchdog timer
  HEARTBEAT;
  wdt_enable(WDTO_2S); // 2 seconds

  // TODO configure pins
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

  // initialize hardware, modules
  HEARTBEAT;
  Display::Init();
  Log::Init();
  Log::LogString(Log::INFO, "BOOTING");
  if (!g_rtc.begin())
    FatalError("begin RTC error");
  delay(500); // come to our senses and get a sane time
  HEARTBEAT;
  g_now = g_rtc.now();
  Pumps::Init();
}

void loop() {
  HEARTBEAT;

  // update clock time
  g_now = g_rtc.now();
  
  PollSensorState();
  
  Display::Poll();
  Log::Poll();
  Pumps::Poll();
  PollLights();

  HEARTBEAT;
  delay(500); // ms
}

void FatalError(const char* message) {
  // activate red light & alarm buzzer
  digitalWrite(PIN_BUZZER, HIGH);
  digitalWrite(PIN_LIGHT_ALARM, HIGH);
  
  Log::LogString(Log::FATAL, message);
  Display::ShowError(message); // display error on LCD

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
