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
  targetBrightness = 0;
  powerState = false;

  // pauseTime = transitionPauseTime;
}

bool Light::isOn() {
  return powerState;
}

void Light::setColor(uint8_t r, uint8_t g, uint8_t b) {
  savedLedState = CRGB(r, g, b);
  targetLedState = savedLedState;
}

uint32_t Light::getColor() {
  uint32_t c = ((uint32_t)ledState.r << 16) | ((uint32_t)ledState.g <<  8) | (uint32_t)ledState.b;
  return c;
}

void Light::setMode(MODES newMode) {
  if (newMode == RAINBOW && mode != RAINBOW) {
    targetMode = RAINBOW;
  } else if (newMode == CHRISTMAS && mode != CHRISTMAS) {
    targetMode = CHRISTMAS;
  } else if (newMode == STATIC && mode != STATIC) {
    targetLedState = savedLedState;
    ledState = targetLedState;
    targetMode = STATIC;
  } else
    return;

  targetBrightness = 0;
}

Light::MODES Light::getMode() {
  if (targetMode != NONE)
    return targetMode;

  return mode;
}

void Light::updateLeds() {
  bool changesMade = false;

  for (int i = 0; i < 3; i++) {
    if (ledState[i] != targetLedState[i]) {
      changesMade = true;
      if (targetLedState[i] > ledState[i]) {
        if ((targetLedState[i]-ledState[i]) > 5)
          ledState[i] += 5;
        else
          ledState[i] = targetLedState[i];
      } else {
        if ((ledState[i]-targetLedState[i]) > 5)
          ledState[i] -= 5;
        else
          ledState[i] = targetLedState[i];
      }
    }
  }
  
  if (changesMade || leds[0] != ledState)
    fill_solid(leds, LED_COUNT, ledState);

  colorPublished = changesMade ? false : colorPublished;
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
  return savedBrightness;
}

void Light::loop() {

  bool update = false;

  if (millis() > nextBrightnessCycle) {
    nextBrightnessCycle = millis() + 10;

    if (brightness != targetBrightness) {
      if (targetBrightness > brightness)
        if ((targetBrightness - brightness) > 5)
          brightness += 5;
        else
          brightness = targetBrightness;
      else
        if ((brightness - targetBrightness) > 5)
          brightness -= 5;
        else
          brightness = targetBrightness;

      FastLED.setBrightness(brightness);

      if (brightness == 0 && targetMode != NONE) {
        mode = targetMode;
        targetMode = NONE;
        loop_count = 0;
        targetBrightness = savedBrightness;
      }
      // }
    }
  }

  if (millis() > nextLedCycle) {
    update = true;

    if (powerState || brightness > 0) {
      if (mode == STATIC) {
        nextLedCycle = millis() + 8;
        updateLeds();
      } else if (mode == RAINBOW) {
        nextLedCycle = millis() + 8;
        loop_count--;
        fill_rainbow( leds, LED_COUNT, loop_count, 2);
        update = true;
      } else if (mode == CHRISTMAS) {
        nextLedCycle = millis() + 25;
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
        update = true;
      }
    }
  }

  if (update)
    FastLED.show();
}