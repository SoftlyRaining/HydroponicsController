class PumpSchedule {
public:
  PumpSchedule(uint8_t id, uint8_t cycleCount, uint8_t floodMinutes, uint8_t drainMinutes, uint8_t pumpOutlet) :
    id(id),
    cycleCount(cycleCount),
    floodMinutes(floodMinutes),
    drainMinutes(drainMinutes),
    pumpOutlet(pumpOutlet),
    firstCycleStart(HOUR * SUNRISE), // might not be evenly spaced for long days and low # of cycles
    cycleInterval((HOUR * LIGHT_HOURS) / (cycleCount - 1)) // for 5 cycles, there are 4 intervals between, thus cycleCount-1
  {
  }

  void update() {
    currentCycle = calculateCurrentCycle();
    nextCycleTimestamp = calculateNextFlood();
  }

  uint8_t getCycle() { return currentCycle; }
  uint32_t getNextCycleTime() { return nextCycleTimestamp; }

  const uint8_t id;
  const uint8_t cycleCount;
  const uint8_t floodMinutes;
  const uint8_t drainMinutes;
  const uint8_t pumpOutlet; // 1 based, as the hardware is labeled
  const uint32_t firstCycleStart; // seconds after midnight
  const uint32_t cycleInterval; // seconds

private:
    uint8_t calculateCurrentCycle() {
    uint32_t daySeconds = g_now.unixtime() % DAY;
    if (daySeconds < firstCycleStart)
      return 0;
      
     // for simplicity, I here assume cycle increments when flood begins, so at time firstCycleStart the cycle is 1.
    int cycle = 1 + (daySeconds - firstCycleStart) / cycleInterval;
  
    if (cycle > cycleCount)
      cycle = cycleCount; // max cycle value
    return cycle;
  }
  
  uint32_t calculateNextFlood() {
    uint32_t nextFlood; // seconds after midnight
    if (currentCycle < cycleCount)
      nextFlood = firstCycleStart + currentCycle * cycleInterval;
    else
      nextFlood = DAY + firstCycleStart;
    
    DateTime nowDay(g_now.year(), g_now.month(), g_now.day(), 0, 0, 0);
    return nowDay.unixtime() + nextFlood;
  }

  uint8_t currentCycle;
  uint32_t nextCycleTimestamp;
};

static PumpSchedule PumpSchedules[] = {
  {1 /*id*/, 5 /*cycleCount*/, 5 /*floodMinutes*/, 5 /*drainMinutes*/, 2 /*pumpOutlet*/},
  {2 /*id*/, 4 /*cycleCount*/, 5 /*floodMinutes*/, 5 /*drainMinutes*/, 4 /*pumpOutlet*/},
};

/*static*/ void Pumps::init() {
  for (PumpSchedule& schedule: PumpSchedules)
    schedule.update();
}

void updateSchedulesAtMidnight() {
  // resets cycle count each day at midnight
  static uint8_t prevDay = g_now.day();
  if (g_now.day() != prevDay) {
    prevDay = g_now.day();
    for (PumpSchedule& schedule: PumpSchedules)
      schedule.update();
  }
}

enum State {
  wait,
  flood,
  drain,
  stateCount
};
  
/*static*/ void Pumps::poll() {
  updateSchedulesAtMidnight();
  
  // timed state changes
  static uint32_t stateChangeTime = 0;
  static State currentState = wait - 1; // state changes immediately - will be incremented back to "wait" as first effective state // TODO refactor: this is crappy and un-obvious
  static PumpSchedule* currentSchedule = NULL;
  
  if (g_now.unixtime() < stateChangeTime)
    return;
  
  currentState = (currentState + 1) % stateCount;
  
  switch (currentState) {
    case wait:
      Log::logString(Log::info, "Pump State: Wait");
      if (currentSchedule)
        currentSchedule->update(); // only update the tray that just got flooded
      stateChangeTime = -1; // max uint value
      for (PumpSchedule& ps: PumpSchedules) {
        if (ps.getNextCycleTime() < stateChangeTime) {
          stateChangeTime = ps.getNextCycleTime();
          currentSchedule = &ps;
        }
      }
      break;
      
    case flood:
      Log::logString(Log::info, "Pump State: Flood #" + String(currentSchedule->id));
      setOutlet(currentSchedule->pumpOutlet, true);
      stateChangeTime = g_now.unixtime() + currentSchedule->floodMinutes * MINUTE;
      break;
      
    case drain:
      Log::logString(Log::info, "Pump State: Drain #" + String(currentSchedule->id));
      setOutlet(currentSchedule->pumpOutlet, false);
      stateChangeTime = g_now.unixtime() + currentSchedule->drainMinutes * MINUTE;
      break;
      
    default:
      fatalError("pump state");
  }
}

/*static*/ uint8_t Pumps::getCurrentCycle(uint8_t index) { return PumpSchedules[index].getCycle(); }
/*static*/ uint32_t Pumps::getNextEvent(uint8_t index) { return PumpSchedules[index].getNextCycleTime(); }
