#define HOUR UINT32_C(3600)
#define DAY UINT32_C(86400)

/*static*/ bool Pumps::flooding;

class PumpSchedule {
public:
  PumpSchedule(uint8_t cycleCount, uint8_t pumpOutlet) :
    pumpOutlet(pumpOutlet),
    firstCycleStart(HOUR * SUNRISE), // might not be evenly spaced for long days and low # of cycles
    cycleInterval((HOUR * LIGHT_HOURS - PUMP_ON_MINUTES) / (cycleCount - 1)),
    cycleCount(cycleCount)
  {
    Update();
  }

  void Update() {
    currentCycle = CalculateCurrentCycle;
    nextCycleTimestamp = CalculateNextFlood();
  }

  uint8_t GetCycle() { return currentCycle; }
  uint32_t GetNextCycleTime { return nextCycleTimestamp; }

  const uint8_t pumpOutlet; // 1 based, as the hardware is labeled
  const uint32_t firstCycleStart; // seconds after midnight
  const uint32_t cycleInterval; // seconds
  const uint8_t cycleCount;

private:
    uint8_t CalculateCurrentCycle() {
    uint32_t daySeconds = g_now.unixtime() % DAY;
    if (daySeconds < firstCycleStart)
      return 0;
      
     // for simplicity, I here assume cycle increments when flood begins, so at time firstCycleStart the cycle is 1.
    int cycle = 1 + (daySeconds - firstCycleStart) / cycleInterval;
  
    if (cycle > WATER_CYCLE_COUNT)
      cycle = WATER_CYCLE_COUNT; // max cycle value
    return cycle;
  }
  
  uint32_t CalculateNextFlood() {
    uint32_t nextFlood; // seconds after midnight
    if (currentCycle < WATER_CYCLE_COUNT)
      nextFlood = firstCycleStart + currentCycle * cycleInterval;
    else
      nextFlood = DAY + firstCycleStart;
    
    DateTime nowDay(g_now.year(), g_now.month(), g_now.day(), 0, 0, 0);
    return nowDay.unixtime() + nextFlood;
  }

  uint8_t currentCycle;
  uint32_t nextCycleTimestamp;
};

/*static*/ void Pumps::Init() {
  flooding = false;
  uint8_t deck
}

/*static*/ void Pumps::Poll() {
  // reset cycle count each day at midnight
  static uint8_t prevDay = g_now.day();
  if (g_now.day() != prevDay) {
    currentCycle = 0;
    prevDay = g_now.day();
  }

  // timed state changes
  uint32_t unixTime = g_now.unixtime();
  if (unixTime >= nextStateChangeTime) {
    if (flooding)
      EndFlood();
    else
      StartFlood();
  }
  SetOutlet(PUMP_OUTLET, flooding);
}

/*static*/ bool Pumps::IsFloodingNow() {
  return flooding;
}

/*static*/ void Pumps::StartFlood() {
  // change state
  flooding = true;
  Log::LogString(Log::INFO, "Starting Flood Pump");

  // set timestamp to turn off pump
  nextStateChangeTime = g_now.unixtime() + PUMP_ON_MINUTES * 60L;
}

/*static*/ void Pumps::EndFlood() {
  // change state
  flooding = false;
  Log::LogString(Log::INFO, "Stopping Flood Pump");

  // set timestamp for next flood
  currentCycle++;
  nextStateChangeTime = CalculateNextFlood();
}
