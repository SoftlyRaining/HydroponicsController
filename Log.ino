bool lastSerialState = false;

/*static*/ void Log::Init() {
  Serial.begin(9600);
}

/*static*/ void Log::Poll() {
  static uint32_t nextLogTime = 0;

  uint32_t unixTime = g_now.unixtime();
  if (unixTime >= nextLogTime) {
    nextLogTime = unixTime + LOG_INTERVAL_MINUTES * 60;
    Update();
  }
}

/*static*/ void Log::Update() {
  String message = "Humidity: " + String(g_airHumidity, 0) + "% Temp:" + String(g_airTemp, 0) + "C";
  LogString(Log::INFO, message);
}

/*static*/ void Log::LogString(Log::Level level, String message) {
  static const char levelStrings[4][6] = {"FATAL", "ERROR", "WARN ", "INFO "};
  
  if (Serial) {
    // log level
    Serial.print(levelStrings[level]);
    Serial.print(' ');

    // timestamp
    Serial.print(g_now.year(), DEC);
    Serial.print('/');
    Serial.print(g_now.month(), DEC);
    Serial.print('/');
    Serial.print(g_now.day(), DEC);
    Serial.print(' ');
    Serial.print(g_now.hour(), DEC);
    Serial.print(':');
    Serial.print(g_now.minute(), DEC);
    Serial.print(':');
    Serial.print(g_now.second(), DEC);
    Serial.print(' ');
    
    Serial.println(message);
  }
  
  // TODO update log file on SD card (if present & working)
}
