bool lastSerialState = false;
bool Log::errorSeen = false;

/*static*/ void Log::init() {
  Serial.begin(9600);
}

/*static*/ void Log::poll() {
  static uint32_t nextLogTime = g_now.unixtime() + 5; // begin a few seconds after boot to have sane sensor data

  uint32_t unixTime = g_now.unixtime();
  if (unixTime >= nextLogTime) {
    nextLogTime = unixTime + LOG_INTERVAL_MINUTES * MINUTE;
    update();
  }
}

/*static*/ void Log::update() {
  logString(Log::info, "-");
}

/*static*/ void Log::logString(Log::Level level, String message) {
  Log::errorSeen |= (level >= error);
  
  static const char levelStrings[4][7] = {"INFO  ", "WARN  ", "ERROR ", "FATAL "};
  String date = String(g_now.year()) + "/" + String(g_now.month()) + "/" + String(g_now.day()) + " ";
  String time = String(g_now.hour()) + ":" + String(g_now.minute()) + ":" + String(g_now.second()) + " ";
  
  if (Serial) {
    Serial.print(levelStrings[level]);
    Serial.print(date);
    Serial.print(time);
    Serial.println(message);
  }
}
