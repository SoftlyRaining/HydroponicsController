class PumpSchedule {
public:
  PumpSchedule(const Deck& deck) : 
    deck(deck),
    cycleInterval(deck.lightDuration / (deck.floodCycles - 1)) // for 5 cycles, there are 4 intervals between, thus floodCycles-1
  {
    updateNextFlood();
  }

  void updateNextFlood() {
    currentCycle = calculateCurrentCycle();
    nextCycleTimestamp = calculateNextFlood();
  }

  uint8_t getCycle() { return currentCycle; }
  uint32_t getNextCycleTime() { return nextCycleTimestamp; }

  const Deck& deck;
  const uint32_t cycleInterval;

private:
    uint8_t calculateCurrentCycle() {
    uint32_t daySeconds = g_now.unixtime() % DAY;
    if (daySeconds < deck.sunriseTime)
      return 0;
      
     // for simplicity, I here assume cycle increments when flood begins, so at deck.sunriseTime the cycle is 1.
    int cycle = 1 + (daySeconds - deck.sunriseTime) / cycleInterval;
  
    if (cycle > deck.floodCycles)
      cycle = deck.floodCycles; // max cycle value
    return cycle;
  }
  
  uint32_t calculateNextFlood() {
    uint32_t nextFlood; // seconds after midnight
    if (currentCycle < deck.floodCycles)
      nextFlood = deck.sunriseTime + currentCycle * cycleInterval;
    else
      nextFlood = DAY + deck.sunriseTime;
    
    DateTime nowDay(g_now.year(), g_now.month(), g_now.day(), 0, 0, 0);
    return nowDay.unixtime() + nextFlood;
  }

  uint8_t currentCycle;
  uint32_t nextCycleTimestamp;
};

static PumpSchedule* PumpSchedules[deckCount];

/*static*/ void Pumps::init() {
  for (int i=0; i < deckCount; i++)
    PumpSchedules[i] = new PumpSchedule(g_deckList[i]); // lives until shutdown - we never deallocate.
}

void updateSchedulesAtMidnight() {
  // resets cycle count each day at midnight
  static uint8_t prevDay = g_now.day();
  if (g_now.day() != prevDay) {
    prevDay = g_now.day();
    for (PumpSchedule* schedule: PumpSchedules)
      schedule->updateNextFlood();
  }
}

PumpSchedule* findNextFloodEvent() {
  uint32_t nextFloodTime = -1; // max uint
  PumpSchedule* nextSchedule = PumpSchedules[0];
  for (PumpSchedule* schedule: PumpSchedules) {
    if (schedule->getNextCycleTime() < nextFloodTime) {
      nextFloodTime = schedule->getNextCycleTime();
      nextSchedule = schedule;
    }
  }
  return nextSchedule;
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
  static State currentState = wait;
  static PumpSchedule* currentSchedule = findNextFloodEvent();
  static uint32_t stateChangeTime = currentSchedule->getNextCycleTime();
  
  if (g_now.unixtime() < stateChangeTime)
    return;
  
  currentState = static_cast<State>((currentState + 1) % stateCount);
  
  switch (currentState) {
    case wait:
      Log::logString(Log::info, "Pump State: Wait");
      if (currentSchedule)
        currentSchedule->updateNextFlood(); // only the tray that just got flooded
      currentSchedule = findNextFloodEvent();
      stateChangeTime = currentSchedule->getNextCycleTime();
      break;
      
    case flood:
      Log::logString(Log::info, "Pump State: Flood #" + String(currentSchedule->deck.id));
      setOutlet(currentSchedule->deck.pumpOutlet, true);
      stateChangeTime = g_now.unixtime() + currentSchedule->deck.floodMinutes * MINUTE;
      break;
      
    case drain:
      Log::logString(Log::info, "Pump State: Drain #" + String(currentSchedule->deck.id));
      setOutlet(currentSchedule->deck.pumpOutlet, false);
      stateChangeTime = g_now.unixtime() + currentSchedule->deck.drainMinutes * MINUTE;
      break;
      
    default:
      fatalError("pump state");
  }
}

/*static*/ uint8_t Pumps::getCurrentCycle(uint8_t index) { return PumpSchedules[index]->getCycle(); }
/*static*/ uint32_t Pumps::getNextEvent(uint8_t index) { return PumpSchedules[index]->getNextCycleTime(); }
