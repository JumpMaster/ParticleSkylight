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

CRGB Light::randomBrightColor(bool includeWhite) {
  uint8_t color;
  if (includeWhite)
    color = random8(0, 7);
  else
    color = random8(0, 6);
  
  switch (color) {
    case 0 :
      return CRGB(0, 0, 255);
    case 1 :
      return CRGB(0, 255, 0);
    case 2 :
      return CRGB(255, 0, 0);
    case 3 :
      return CRGB(255, 255, 0);
    case 4 :
      return CRGB(0, 255,255);
    case 5 :
      return CRGB(255, 0, 255);
  }

  return CRGB::White;
}

void Light::setColor(uint8_t r, uint8_t g, uint8_t b) {
  savedColor = CRGB(r, g, b);
  targetColor = savedColor;
}

uint32_t Light::getColor() {
  uint32_t c = ((uint32_t)savedColor.r << 16) | ((uint32_t)savedColor.g <<  8) | (uint32_t)savedColor.b;
  return c;
}

void Light::setMode(MODES newMode) {
  if (mode == newMode)
    return;
  
  targetMode = newMode;
}

void Light::changeModeTo(MODES newMode) {
  mode = newMode;
  FastLED.clear();
  loop_count = 0;

  if (newMode == METEORS) {
    meteorColor[0] = randomBrightColor(true);
    meteorColor[1] = randomBrightColor(true);
    meteorPosition[0] = 0;
    meteorPosition[1] = 0;
  } else if (newMode == STATIC) {
    targetColor = savedColor;
    targetMode = STATIC;
  } else if (newMode == LIGHT_SWIPE) {
    targetColor = randomBrightColor(false);
    previousColor = CRGB::Black;
  }
}

Light::MODES Light::getMode() {
  if (targetMode != NONE)
    return targetMode;

  return mode;
}

void Light::updateLeds() {
  bool changesMade = false;

  for (int i = 0; i < 3; i++) {
    if (leds[0][i] != targetColor[i]) {
      changesMade = true;
      if (targetColor[i] > leds[0][i]) {
        leds[0][i] = targetColor[i]-leds[0][i] > 5 ? leds[0][i]+5 : targetColor[i];
      } else {
        leds[0][i] = leds[0][i]-targetColor[i] > 5 ? leds[0][i]-5 : targetColor[i];
      }
    }
  }
  
  if (changesMade)// || leds[0] != leds)
    fill_solid(leds, LED_COUNT, leds[0]);

  colorPublished = changesMade ? false : colorPublished;
}

void Light::loadSettings() {
  SaveData saveData;
  EEPROM.get(0, saveData);
  if (saveData.mode > 0) {
    mode = saveData.mode;
    savedBrightness = saveData.brightness;
    savedColor = saveData.color;
    targetColor = saveData.color;
  }
}

void Light::saveSettings() {
  SaveData saveData;
  saveData.mode = mode;
  saveData.brightness = savedBrightness;
  saveData.color = savedColor;
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

  if (millis() >= nextLedCycle) {
    nextLedCycle = millis() + 5; // roughly 200 fps

    if (targetMode) {
      if (brightness == 0 && targetMode != NONE) {
        changeModeTo(targetMode);
        targetMode = NONE;
      }

      brightness = brightness > 5 ? brightness-5 : 0;

      FastLED.setBrightness(brightness);
      ledsUpdated = true;
    } else if (brightness != targetBrightness) {
      if (targetBrightness > brightness)
        brightness = targetBrightness - brightness > 5 ? brightness+5 : targetBrightness;
      else
        brightness = brightness - targetBrightness > 5 ? brightness-5 : targetBrightness;

      FastLED.setBrightness(brightness);
      ledsUpdated = true;
    }

  
    // We need to continue all animations until
    // we're powered off and the brightness is 0
    if (powerState || brightness > 0) {
      ledsUpdated = true;
      if (mode == STATIC) {
        updateLeds();
      } else if (mode == RAINBOW) {
        loop_count--;
        fill_rainbow( leds, LED_COUNT, loop_count/4, 2);
      } else if (mode == CHRISTMAS) {
        if (loop_count % 3 == 0) {
          for (uint16_t i = 0; i < (LED_COUNT/2); i++) {
            uint8_t t = ((loop_count/3) + i) / 7 % 3;
            CRGB color;
            if (t == 0) {
              color = CRGB::Red;
            } else if (t == 1) {
              color = CRGB::Green;
            } else {
              color = CRGB::Blue;
            }

            int16_t l = 231 + i;
            int16_t r = 229 - i;

            if (l >= LED_COUNT)
              l -= LED_COUNT;

            leds[l] = color;
            leds[r] = color;
          }
        }
        loop_count++;

        if (loop_count >= 63) // 63 is divisible by 7 and 3
          loop_count = 0;

      } else if (mode == METEORS) {
        fadeToBlackBy(leds, LED_COUNT, random8(5, 20));

        for (uint8_t m = 0; m <= 1; m++) {
          if (loop_count % meteorSpeed[m] == 0) {
            leds[meteorPosition[m]].r = qadd8(meteorColor[m].r, leds[meteorPosition[m]].r);
            leds[meteorPosition[m]].g = qadd8(meteorColor[m].g, leds[meteorPosition[m]].g);
            leds[meteorPosition[m]].b = qadd8(meteorColor[m].b, leds[meteorPosition[m]].b);
            meteorPosition[m]++;
          }

          if (meteorPosition[m] >= LED_COUNT) {
            meteorPosition[m] = 0;
            meteorColor[m] = randomBrightColor(true);
            meteorSpeed[m] = random8(1,3); // random number between 1 and 2
          }
        }
        loop_count++;
      } else if (mode == LIGHT_SWIPE) {
        leds[loop_count++] = CRGB::White;

        for (int i = 0; i < LED_COUNT; i++) {
          if (i < loop_count) {
            if (leds[i].r > targetColor.r)
              leds[i].r -= 5;
            if (leds[i].g > targetColor.g)
              leds[i].g -= 5;
            if (leds[i].b > targetColor.b)
              leds[i].b -= 5;
          } else if (i > loop_count) {
            if (leds[i].r > previousColor.r)
              leds[i].r -= 5;
            if (leds[i].g > previousColor.g)
              leds[i].g -= 5;
            if (leds[i].b > previousColor.b)
              leds[i].b -= 5;
          }
        }

        if (loop_count >= LED_COUNT) {
          loop_count = 0;
          previousColor = targetColor;
          targetColor = randomBrightColor(false);
        }
      }
    }
  }

  if (ledsUpdated) {
    ledsUpdated = false;
    FastLED.show();
    if (showFPS)
      fps++;
  }

  if (showFPS && millis() > nextPublishTime) {
    Particle.publish("FPS", String::format("%d %d", millis()-lastPublishTime, fps), PRIVATE);
    lastPublishTime = nextPublishTime;
    nextPublishTime = millis() + 1000;
    fps = 0;
  }
}