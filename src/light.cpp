#include "light.h"

void Light::setup() {
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_COUNT);
  FastLED.setBrightness(0);
  FastLED.show();
}

void Light::on() {
  targetBrightness = savedBrightness;
  powerState = true;
}

void Light::off() {
  savedBrightness = targetBrightness;
  targetBrightness = 0;
  powerState = false;

  // pauseTime = transitionPauseTime;
}

bool Light::isOn() {
  return powerState;
}

void Light::setColor(uint8_t r, uint8_t g, uint8_t b) {
  savedLedState = CRGB(r, g, b);
}

uint32_t Light::getColor() {
  uint32_t c = ((uint32_t)ledState.r << 16) | ((uint32_t)ledState.g <<  8) | (uint32_t)ledState.b;
  return c;
}

void Light::setMode(MODES newMode) {
  if (newMode == RAINBOW) {
    targetLedState = CRGB::Red;
    loop_count = 0;
    mode = RAINBOW;
    pauseTime = fadePauseTime;
  } else if (newMode == CHRISTMAS) {
    loop_count = 0;
    mode = CHRISTMAS;
    pauseTime = 50;
  } else if (newMode == STATIC) {
    targetLedState = savedLedState;
    mode = STATIC;
    pauseTime = transitionPauseTime;
  }
}

Light::MODES Light::getMode() {
  return mode;
}

bool Light::updateLeds() {
  bool changesMade = false;

  for (int i = 0; i < 3; i++) {
    if (ledState[i] != targetLedState[i]) {
      changesMade = true;
      if (targetLedState[i] > ledState[i])
        ledState[i]++;
      else
        ledState[i]--;
    }
  }
  
  if (changesMade) {
    for (int i = 0; i < LED_COUNT; i++) {
      leds[i] = ledState;
    }
  }

  if (changesMade || brightness != targetBrightness)
    FastLED.show();

  colorPublished = changesMade ? false : colorPublished;
  return changesMade;
}

void Light::loadSettings() {
  SaveData saveData;
  EEPROM.get(0, saveData);
  if (saveData.mode > 0) {
    mode = saveData.mode;
    savedBrightness = saveData.brightness;
    savedLedState = saveData.color;
    targetLedState = saveData.color;
  }
}

void Light::saveSettings() {
  SaveData saveData;
  saveData.mode = mode;
  saveData.brightness = savedBrightness;
  saveData.color = savedLedState;
  EEPROM.put(0, saveData);
}

bool Light::isColorPublished() {
  if (!colorPublished) {
    colorPublished = true;
    return false;
  }
  return true;
}

void Light::setBrightness(uint8_t b) {
  targetBrightness = b;
  savedBrightness = b;
}

uint8_t Light::getBrightness() {
  return targetBrightness;
}

void Light::loop() {

  if (millis() > nextBrightnessCycle) {
    nextBrightnessCycle = millis() + 5;

    if (brightness != targetBrightness) {
      if (targetBrightness > brightness)
        brightness++;
      else
        brightness--;

      FastLED.setBrightness(brightness);

      if (brightness == 0) {
        ledState = CRGB::Black;
        FastLED.show();
      }
    }

  }

  if (millis() > nextLedCycle) {

    nextLedCycle = millis() + pauseTime;

    if (powerState || brightness > 0) {

      if (mode == STATIC) {
        updateLeds();
      } else if (mode == RAINBOW) {
        int updated = updateLeds();

        if (!updated) {
          if (loop_count == 0) { // Red is up, bring up green
            targetLedState.g = 255;
          } else if (loop_count == 1) { // Red and green are up. take down red.
            targetLedState.r = 0;
          } else if (loop_count == 2) { // Green is up, bring up blue
            targetLedState.b = 255;
          } else if (loop_count == 3) { // Green and blue are up, take down green
            targetLedState.g = 0;
          } else if (loop_count == 4) { // Blue is up, bring up red
            targetLedState.r = 255;
          } else if (loop_count == 5) { // Blue and red are up, take down blue
            targetLedState.b = 0;
          }

          if (loop_count == 5)
            loop_count = 0;
          else
            loop_count++;
        }
      } else if (mode == CHRISTMAS) {

        if (loop_count >= 20)
          loop_count = 0;
        else
          loop_count++;
      
        for (int i = 0; i < LED_COUNT; i++) {
          uint8_t t = ((21+i-loop_count) / 7) % 3;
          if (t == 0) {
            leds[i] = CRGB::Red;
          } else if (t == 1) {
            leds[i] = CRGB::Green;
          } else {
            leds[i] = CRGB::Blue;
          }
        }
        FastLED.show();
      }
    }
  }
}