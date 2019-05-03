#define MINUTE UINT32_C(60)
#define HOUR UINT32_C(3600)
#define DAY UINT32_C(86400)

// configuration
//#define SCHEDULE_TEST // if defined, the clock will be greatly sped up

struct Deck {
  const int id;
  
  const int lightOutlet;
  const int pumpOutlet;
  
  const uint32_t lightDuration; // seconds
  const uint32_t sunriseTime; // seconds after midnight
  const uint8_t floodCycles;

  const uint8_t floodMinutes;
  const uint8_t drainMinutes;
};

// actual timing:
// deck 1   5 minute flood    5 minute drain
// deck 2   5 minute flood    10 minute drain
// deck 1 drains back fast enough for deck 2 to start immediately, thus "0" drainminutes for deck 1
static const Deck g_deckList[] = {
  {1 /*id*/, 1 /*lightOutlet*/, 2 /*pumpOutlet*/, 14 * HOUR /*lightDuration*/, 8 * HOUR /*sunriseTime*/, 2 /*floodCycles*/, 5 /*floodMinutes*/, 0 /*drainMinutes*/},
  {2 /*id*/, 3 /*lightOutlet*/, 4 /*pumpOutlet*/, 14 * HOUR /*lightDuration*/, 8 * HOUR /*sunriseTime*/, 2 /*floodCycles*/, 5 /*floodMinutes*/, 10 /*drainMinutes*/},
};
static const uint8_t deckCount = sizeof(g_deckList) / sizeof(Deck);
static_assert(deckCount > 0, "Must have at least one deck");

#define LOG_INTERVAL_MINUTES 5

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
  enum Level { info, warn, error, fatal };

  static void init();
  static void poll();
  static void logString(Level level, String message);
  static bool haveSeenError() { return errorSeen; }
  
private:
  static void update();

  static bool errorSeen;
};

class PumpSchedule; // for some reason this needs to be here for stuff in Pumps.ino to work??
class Pumps {
public:
  static void init();
  static void poll();

  static uint8_t getCurrentCycle(uint8_t index);
  static uint32_t getNextEvent(uint8_t index);
};

class Lights {
public:
  static void init();
  static void poll();
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

void setOutlet(int number, bool on) {
  Log::logString(Log::info, "Setting outlet " + String(number) + " = " + String(on));
  if (number < 1 || number > 8)
    fatalError("bad outlet #");
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
  //wdt_enable(WDTO_2S); // 2 seconds

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
  Lights::init();
  Pumps::init();

  //lights on
  setOutlet(1, true);
  setOutlet(3, true);
}

void loop() {
  auto setPumps = [](bool deck1, bool deck2) {
     setOutlet(2, deck1);
     setOutlet(4, deck2);
  };

  setPumps(true, false);
  //Display::showError("Hello World 1");
  delay(5000);
  
  setPumps(false, false);
  //Display::showError("Hello World 2");
  delay(5000);
  
  setPumps(false, true);
  //Display::showError("Hello World 2");
  delay(5000);
  
  setPumps(false, false);
  //Display::showError("Hello World 2");
  delay(5000);
}

void fatalError(const char* message) {
  // activate red light & alarm buzzer
  digitalWrite(PIN_BUZZER, HIGH);
  digitalWrite(PIN_LIGHT_ALARM, HIGH);
  
  Log::logString(Log::fatal, message);
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
