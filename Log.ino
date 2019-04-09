#include <SPI.h> // used by SD.h
#include <SD.h>

bool lastSerialState = false;

/*static*/ void Log::Init() {
  Serial.begin(9600);

  if (!SD.begin(PIN_SD_CHIPSELECT))
    FatalError("SD card fail");
}

/*static*/ void Log::Poll() {
  static uint32_t nextLogTime = g_now.unixtime() + 5; // wait a few seconds after boot to have sane sensor data

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

  String taggedMessage = String(levelStrings[level]) + " ";
  taggedMessage += String(g_now.year()) + "/" + String(g_now.month()) + "/" + String(g_now.day()) + " ";
  taggedMessage += String(g_now.hour()) + ":" + String(g_now.minute()) + ":" + String(g_now.second()) + " ";
  taggedMessage += message;
  
  if (Serial)
    Serial.println(taggedMessage);
  
  File logFile = SD.open("log.txt", FILE_WRITE);
  if (!logFile) {
    Serial.println("error opening log on SD");
    Display::ShowError("SD log error"); // TODO this junks up the display when execution continues b/c normal display doesn't use clear. On the bright side, you can tell it happened even hours later...
  }
  if (logFile) {
    logFile.println(taggedMessage);
    logFile.close();
  }
}
