#include "light.h"

void Light::on() {
    brightness = savedBrightness;
    powerState = true;
}

void Light::off() {
    savedBrightness = brightness;
    brightness = 0;
    powerState = false;
    pauseTime = transitionPauseTime;
}


bool Light::isOn() {
    return powerState;
}

void Light::setColor(uint8_t r, uint8_t g, uint8_t b) {
  savedLedState[0] = r;
  savedLedState[1] = g;
  savedLedState[2] = b;
}

uint32_t Light::getColor() {
  uint32_t c = ((uint32_t)ledState[0] << 16) | ((uint32_t)ledState[1] <<  8) | (uint32_t)ledState[2];
  return c;
}

void Light::setMode(MODES newMode) {
  if (newMode == RAINBOW) {
    requestedLedState[0] = 255;
    requestedLedState[1] = 0;
    requestedLedState[2] = 0;
    stage = 0;
    mode = RAINBOW;
    pauseTime = fadePauseTime;
    // if (mqttClient.isConnected())
    //   mqttClient.publish("home/light/playroom/skylight/effect", "rainbow", true);
  } else if (newMode == STATIC) {
    for (int i = 0; i < 3; i++)
      requestedLedState[i] = savedLedState[i];
    mode = STATIC;
    pauseTime = transitionPauseTime;
    // if (mqttClient.isConnected()) {
    //   mqttClient.publish("home/light/playroom/skylight/effect", "static", true);
    // }
  }
}

Light::MODES Light::getMode() {
    return mode;
}

bool Light::updateLeds() {
  bool changesMade = false;
   
  for (int i = 0; i < 3; i++) {
    uint8_t adjustedState = (requestedLedState[i] * brightness) / 255;
    if (ledState[i] != adjustedState) {
      changesMade = true;
      if (adjustedState > ledState[i])
        ledState[i]++;
      else
        ledState[i]--;

      analogWrite(ledPins[i], ledState[i]);
    }
  }

  publishColor = changesMade ? true : publishColor;
  return changesMade;
}

void Light::loadSettings() {
  SaveData saveData;
  EEPROM.get(0, saveData);
  if (saveData.mode > 0) {
    mode = saveData.mode;
    savedBrightness = saveData.brightness;
    savedLedState[0] = saveData.red;
    savedLedState[1] = saveData.green;
    savedLedState[2] = saveData.blue;
    
    brightness = savedBrightness;
    for (int i = 0; i < 3; i++)
      requestedLedState[i] = savedLedState[i];
  }
}

void Light::saveSettings() {
    SaveData saveData;
    saveData.mode = mode;
    saveData.brightness = savedBrightness;
    saveData.red = savedLedState[0];
    saveData.green = savedLedState[1];
    saveData.blue = savedLedState[2];
    EEPROM.put(0, saveData);
}

bool Light::getColorPublished() {
    if (publishColor) {
        publishColor = false;
        return true;
    }
    return false;
}

void Light::setBrightness(uint8_t b) {
    brightness = b;
    savedBrightness = b;
}

uint8_t Light::getBrightness() {
    return brightness;
}

void Light::loop() {
  if (millis() > nextCycle) {

    nextCycle = millis() + pauseTime;

    if (powerState || (ledState[0]+ledState[1]+ledState[2]) > 0) {
      int updated = updateLeds();

      if (mode == RAINBOW) {
        if (!updated) {
          if (stage == 0) { // Red is up, bring up green
            requestedLedState[1] = 255;
          } else if (stage == 1) { // Red and green are up. take down red.
            requestedLedState[0] = 0;
          } else if (stage == 2) { // Green is up, bring up blue
            requestedLedState[2] = 255;
          } else if (stage == 3) { // Green and blue are up, take down green
            requestedLedState[1] = 0;
          } else if (stage == 4) { // Blue is up, bring up red
            requestedLedState[0] = 255;
          } else if (stage == 5) { // Blue and red are up, take down blue
            requestedLedState[2] = 0;
          }

          if (stage == 5)
            stage = 0;
          else
            stage++;
        }
      }
    }
  }
}