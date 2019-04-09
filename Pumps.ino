#define HOUR (60L*60)
#define DAY (HOUR*24)

/*static*/ bool Pumps::flooding;
/*static*/ uint8_t Pumps::currentCycle;
/*static*/ uint32_t Pumps::nextStateChangeTime;
/*static*/ uint32_t Pumps::firstCycleStart; // time after midnight in seconds
/*static*/ uint32_t Pumps::cycleDuration;

/*static*/ void Pumps::Init() {
  flooding = false;

  bool shouldStartAtSunrise = 24. / WATER_CYCLE_COUNT < (24 - LIGHT_HOURS);

  if (shouldStartAtSunrise) {
    // TODO lighting should begin AND end with flooding. Currently lighting ends right when an extra flood cycle would begin (I think).
    cycleDuration = HOUR * LIGHT_HOURS / WATER_CYCLE_COUNT;
    firstCycleStart = HOUR * SUNRISE;
  } else {
    cycleDuration = DAY / WATER_CYCLE_COUNT;
    firstCycleStart = cycleDuration / 2; // center an ebb time period on midnight
  }

  currentCycle = CalculateCycle();
  nextStateChangeTime = CalculateNextFlood();
}

/*static*/ int Pumps::CalculateCycle() {
  uint32_t daySeconds = g_now.unixtime() % DAY;
  if (daySeconds < firstCycleStart)
    return 0;
  else
    return (daySeconds - firstCycleStart) / cycleDuration;
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
