#define MINUTE UINT32_C(60)
#define HOUR UINT32_C(3600)
#define DAY UINT32_C(86400)

// configuration
//#define SCHEDULE_TEST // if defined, the clock will be greatly sped up
//#define SET_TIME_OVERRIDE // if defined, it will set the clock and stop.

struct Deck {
  const int id;
  
  const int lightOutlet;
  const int pumpOutlet;
  
  const uint32_t lightDuration; // seconds
  const uint32_t sunriseTime; // seconds after midnight
  const uint8_t floodCycles;

  const uint8_t floodMinutes; // pump-on time
  const uint8_t drainMinutes; // wait for water to drain back to reservoir before any other pump action (0 if not necessary)
};

static const Deck g_deckList[] = {
  {1 /*id*/, 1 /*lightOutlet*/, 2 /*pumpOutlet*/, 12 * HOUR /*lightDuration*/, 9 * HOUR /*sunriseTime*/, 4 /*floodCycles*/, 10 /*floodMinutes*/, 0 /*drainMinutes*/},
};
static const uint8_t deckCount = sizeof(g_deckList) / sizeof(Deck);
static_assert(deckCount > 0, "Must have at least one deck");

#define LOG_INTERVAL_MINUTES 5

// pinout definitions
#define PIN_LIGHT_HEARTBEAT   LED_BUILTIN

#define PIN_OUTLET_1          2
#define PIN_OUTLET_2          3

// includes
#include <avr/wdt.h>
#include <Wire.h> // used by RTClb (I2C?)
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

// global state
DateTime g_now;

void setOutlet(int number, bool on) {
  Log::logString(Log::info, "Setting outlet " + String(number) + " = " + String(on));
  if (number < 1 || number > 8)
    fatalError("bad outlet #");
  static const int pinMap[8] = {
    PIN_OUTLET_1,
    PIN_OUTLET_2,
  };
  digitalWrite(pinMap[number-1], on ? LOW : HIGH);
}

void setup() {  
  // enable watchdog timer
  HEARTBEAT;
  wdt_enable(WDTO_2S); // 2 seconds

  // configure pins
  HEARTBEAT;
  pinMode(PIN_LIGHT_HEARTBEAT, OUTPUT);
  pinMode(PIN_OUTLET_1, OUTPUT);
  pinMode(PIN_OUTLET_2, OUTPUT);

  digitalWrite(PIN_OUTLET_1, HIGH); // high means relay off
  digitalWrite(PIN_OUTLET_2, HIGH);

  // initialize hardware, modules
  HEARTBEAT;
  Display::init();
  Log::init();
  
  if (!g_rtc.begin())
    fatalError("begin RTC error");
#ifdef SET_TIME_OVERRIDE
  g_rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
#endif
  delay(500); // come to our senses and get a sane time
  
  HEARTBEAT;
  g_now = g_rtc.now();
  Lights::init();
  Pumps::init();
  
  Log::logString(Log::warn, "RESET");
#ifdef SCHEDULE_TEST
  Log::logString(Log::warn, "Schedule test enabled.");
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
  
  Display::poll();
  Log::poll();
  Lights::poll();
  Pumps::poll();

  HEARTBEAT;
  delay(500); // ms
}

void fatalError(const char* message) {
  Log::logString(Log::fatal, message);
  Display::showError(message); // display error on LCD

  // wait for watchdog to reset us (no heartbeat!)
  while (true) {
    delay(250);
  }
}
