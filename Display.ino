#include <LiquidCrystal.h>

// custom characters
byte customChar_waterDrop[] = {
  B00100,
  B00100,
  B01110,
  B01110,
  B11111,
  B11111,
  B11111,
  B01110
};
byte customChar_twoDrops[] = {
  B01000,
  B01000,
  B11100,
  B01000,
  B00010,
  B00010,
  B00111,
  B00010
};
byte customChar_drain[] = {
  B00000,
  B10001,
  B10001,
  B10001,
  B11111,
  B00100,
  B00000,
  B00100
};
byte customChar_fill[] = {
  B00100,
  B10001,
  B10101,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000
};

LiquidCrystal g_lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

/*static*/ void Display::Init() {
  g_lcd.begin(16, 2);
  g_lcd.createChar(0, customChar_waterDrop);
  g_lcd.createChar(1, customChar_drain);
  g_lcd.createChar(2, customChar_fill);

  g_lcd.setCursor(0,0);
  g_lcd.write(byte(0)); // water drop
  g_lcd.print(" --:--:-- ");
  g_lcd.setCursor(15,0);
  g_lcd.write(byte(0)); // water drop
}

/*static*/ void Display::Poll() {
  // blink heartbeat light
  static bool heartbeatLightState = false;
  heartbeatLightState = !heartbeatLightState;
  digitalWrite(PIN_LIGHT_HEARTBEAT, heartbeatLightState ? HIGH : LOW);
  
  // update LCD when clock seconds change
  static uint8_t lastSecond = 0;
  if (g_now.second() != lastSecond)
    Update();
  lastSecond = g_now.second();
}

/*static*/ void Display::ShowError(const char* message) {
  g_lcd.clear();
  g_lcd.print(message);
}

// actually writes to LCD
/*static*/ void Display::Update() {
  // 16x2 display area
  // up to 8 custom characters, 5x8

  /*
   |      |       |  (measures 16 characters)
   * HH:MM:SS W## *   duration, watering or draining, cycle index
     HH:MM ##% ##C   clock, humidity, temp
   */

   uint32_t secondsToNextPumpEvent = Pumps::GetNextPumpEventTimestamp() - g_now.unixtime();
   uint8_t seconds = secondsToNextPumpEvent % 60;
   secondsToNextPumpEvent /= 60;
   uint8_t minutes = secondsToNextPumpEvent % 60;
   secondsToNextPumpEvent /= 60;
   uint8_t hours = secondsToNextPumpEvent % 100; // truncate to 2 digits

  g_lcd.setCursor(2,0);
  if (hours < 10) g_lcd.print('0');
  g_lcd.print(hours, DEC);
  
  g_lcd.setCursor(5,0);
  if (minutes < 10) g_lcd.print('0');
  g_lcd.print(minutes, DEC);
  
  g_lcd.setCursor(8,0);
  if (seconds < 10) g_lcd.print('0');
  g_lcd.print(seconds, DEC);

  g_lcd.setCursor(11,0);
  g_lcd.write(byte(Pumps::IsFloodingNow() ? 2 : 1)); // fill / drain characters
  if (Pumps::GetFloodCycle() < 10) g_lcd.print('0');
  g_lcd.print(Pumps::GetFloodCycle(), DEC);
  
  g_lcd.setCursor(2,1);
  if (g_now.hour() < 10) g_lcd.print('0');
  g_lcd.print(g_now.hour(), DEC);
  g_lcd.print(':');
  if (g_now.minute() < 10) g_lcd.print('0');
  g_lcd.print(g_now.minute(), DEC);
  g_lcd.print(' ');
  g_lcd.print((int) g_airHumidity, DEC);
  g_lcd.print("% ");
  g_lcd.print((int) g_airTemp, DEC);
  g_lcd.print("C  ");
}
