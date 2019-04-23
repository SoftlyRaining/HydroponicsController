/*static*/ void Lights::init() {
}

/*static*/ void Lights::poll() {
  static bool lightOn[sizeof(g_deckList)] {};

  uint32_t daySeconds = g_now.unixtime() % DAY;
  for (int i=0; i<sizeof(g_deckList); i++) {
    const bool afterSunrise = (daySeconds >= g_deckList[i].sunriseTime);
    const bool beforeSunset = (daySeconds < g_deckList[i].sunriseTime + g_deckList[i].lightDuration);
    const bool shouldBeOn = afterSunrise && beforeSunset;
    
    if (lightOn[i] != shouldBeOn) {
      lightOn[i] = shouldBeOn;
      Log::logString(Log::info, shouldBeOn ? "Sunrise" : "Sunset");
      setOutlet(g_deckList[i].lightOutlet, shouldBeOn);
    }
  }
}
