#include <SPI.h> // used by SD.h
#include <SD.h>

bool lastSerialState = false;
bool Log::errorSeen = false;

/*static*/ void Log::Init() {
  Serial.begin(9600);

  if (!SD.begin(PIN_SD_CHIPSELECT))
    FatalError("SD card fail");
}

/*static*/ void Log::Poll() {
  static uint32_t nextLogTime = g_now.unixtime() + 5; // wait a few seconds after boot to have sane sensor data

  uint32_t unixTime = g_now.unixtime();
  if (unixTime >= nextLogTime) {
    nextLogTime = unixTime + LOG_INTERVAL_MINUTES * MINUTE;
    Update();
  }
}

/*static*/ void Log::Update() {
  String message = "Humidity: " + String(g_airHumidity, 0) + "% Temp:" + String(g_airTemp, 0) + "C";
  LogString(Log::INFO, message);
}

/*static*/ void Log::LogString(Log::Level level, String message) {
  Log::errorSeen |= (level >= ERROR);
  
  static const char levelStrings[4][7] = {"INFO  ", "WARN  ", "ERROR ", "FATAL "};
  String date = String(g_now.year()) + "/" + String(g_now.month()) + "/" + String(g_now.day()) + " ";
  String time = String(g_now.hour()) + ":" + String(g_now.minute()) + ":" + String(g_now.second()) + " ";
  
  if (Serial) {
    Serial.print(levelStrings[level]);
    Serial.print(date);
    Serial.print(time);
    Serial.println(message);
  }
  
  File logFile = SD.open("log.txt", FILE_WRITE);
  if (!logFile) {
    Serial.println("error opening log on SD");
    Display::ShowError("SD log error"); // TODO this junks up the display when execution continues b/c normal display doesn't use clear. On the bright side, you can tell it happened even hours later...
  } else {
    logFile.print(levelStrings[level]);
    logFile.print(date);
    logFile.print(time);
    logFile.println(message);
    logFile.close();
  }
}
