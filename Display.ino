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

/*static*/ void Display::init() {
  // 16x2 display area
  g_lcd.begin(16, 2);
  
  // up to 8 custom characters, 5x8
  g_lcd.createChar(0, customChar_waterDrop);
  g_lcd.createChar(1, customChar_drain);
  g_lcd.createChar(2, customChar_fill);
}

/*static*/ void Display::poll() {
  // blink heartbeat light
  static bool heartbeatLightState = false;
  heartbeatLightState = !heartbeatLightState;
  digitalWrite(PIN_LIGHT_HEARTBEAT, heartbeatLightState ? HIGH : LOW);

  // update check engine light
  digitalWrite(PIN_LIGHT_ALARM, Log::haveSeenError() ? HIGH : LOW);
  
  // update LCD always
  update();
}

/*static*/ void Display::showError(const char* message) {
  g_lcd.clear();
  g_lcd.print(message);
}

// actually writes to LCD
/*static*/ void Display::update() {
  /*
   |      |       |  (measures 16 characters)
   HH:MM*#  HH:MM*#  tray1 next flood, tray 1 flood index, tray2 next flood, tray2 flood index
     HH:MM ##% ##C   clock, humidity, temp
   */

  auto displayTrayInfo = [](uint8_t trayIndex) {
    DateTime nextEvent = DateTime(Pumps::getNextEvent(trayIndex));
    if (nextEvent.hour() < 10) g_lcd.print('0');
    g_lcd.print(nextEvent.hour(), DEC);
    g_lcd.print(':');
    if (nextEvent.minute() < 10) g_lcd.print('0');
    g_lcd.print(nextEvent.minute(), DEC);
    g_lcd.write(byte(0)); // water drop
    g_lcd.print(Pumps::getCurrentCycle(trayIndex));
  };

  g_lcd.setCursor(0,0);
  displayTrayInfo(0);
  g_lcd.print("  ");
  displayTrayInfo(1);
  
  g_lcd.setCursor(0,1);
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
