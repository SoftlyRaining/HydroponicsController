#define HOUR UINT32_C(3600)
#define DAY UINT32_C(86400)

/*static*/ bool Pumps::flooding;
/*static*/ uint8_t Pumps::currentCycle;
/*static*/ uint32_t Pumps::nextStateChangeTime;
/*static*/ uint32_t Pumps::firstCycleStart; // time after midnight in seconds
/*static*/ uint32_t Pumps::cycleDuration;

/*static*/ void Pumps::Init() {
  flooding = false;

  bool shouldStartAtSunrise = 24. / WATER_CYCLE_COUNT < (24 - LIGHT_HOURS);

  if (shouldStartAtSunrise) {
    // for the last flood if we want the pump to turn off at sunset, subtract PUMP_ON_MINUTES
    uint32_t floodDayLength = HOUR * LIGHT_HOURS - PUMP_ON_MINUTES;
    // for N floods during a day, there are N-1 intervals between floods
    uint8_t daytimeWaitIntervals = WATER_CYCLE_COUNT - 1;
    
    cycleDuration = floodDayLength / daytimeWaitIntervals;
    firstCycleStart = HOUR * SUNRISE;
  } else {
    cycleDuration = DAY / WATER_CYCLE_COUNT;
    firstCycleStart = cycleDuration / 2; // center an ebb time period on midnight
  }

  currentCycle = CalculateCurrentCycle();
  nextStateChangeTime = CalculateNextFlood();
}

/*static*/ int Pumps::CalculateCurrentCycle() {
  uint32_t daySeconds = g_now.unixtime() % DAY;
  if (daySeconds < firstCycleStart)
    return 0;
    
   // for simplicity, I here assume cycle increments when flood begins, so at time firstCycleStart the cycle is 1.
  int cycle = 1 + (daySeconds - firstCycleStart) / cycleDuration;

  if (cycle > WATER_CYCLE_COUNT)
    cycle = WATER_CYCLE_COUNT; // max cycle value
  return cycle;
}


/*static*/ uint32_t Pumps::CalculateNextFlood() {
  uint32_t nextFlood; // seconds since midnight
  if (currentCycle < WATER_CYCLE_COUNT)
    nextFlood = firstCycleStart + currentCycle * cycleDuration;
  else
    nextFlood = DAY + firstCycleStart;
  
  DateTime floodTime(g_now.year(), g_now.month(), g_now.day(), 0, 0, 0);
  return floodTime.unixtime() + nextFlood;
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

/*static*/ uint32_t Pumps::GetNextPumpEventTimestamp() {
  return nextStateChangeTime;
}

/*static*/ int Pumps::GetFloodCycle() {
  return currentCycle;
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
