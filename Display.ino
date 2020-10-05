
/*static*/ void Display::init() {
}

/*static*/ void Display::poll() {
  // blink heartbeat light
  static bool heartbeatLightState = false;
  heartbeatLightState = !heartbeatLightState;
  digitalWrite(PIN_LIGHT_HEARTBEAT, heartbeatLightState ? HIGH : LOW);
}

/*static*/ void Display::showError(const char* message) {
}
