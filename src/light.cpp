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

CRGB Light::randomBrightColor() {
  CRGB rColor;
  while (!rColor)
    rColor = CRGB(rand() & 1 ? 255 : 0, rand() & 1 ? 255 : 0, rand() & 1 ? 255 : 0);
  return rColor;
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
  if (mode == newMode)
    return;
  
  targetMode = newMode;
  targetBrightness = 0;
}

void Light::changeModeTo(MODES newMode) {
  mode = newMode;
  loop_count = 0;
  if (newMode == METEORS && mode != METEORS) {
    targetMode = METEORS;
    // FastLED.clear();
    // fill_solid(leds, LED_COUNT, CRGB::Black);
    // FastLED.show();
    meteorColor[0] = randomBrightColor();
    meteorColor[1] = randomBrightColor();
    meteorPosition[0] = 0;
    meteorPosition[1] = 0;
  } else if (newMode == STATIC && mode != STATIC) {
    targetLedState = savedLedState;
    ledState = targetLedState;
    targetMode = STATIC;
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
    if (ledState[i] != targetLedState[i]) {
      changesMade = true;
      if (targetLedState[i] > ledState[i]) {
        ledState[i] = targetLedState[i]-ledState[i] > 5 ? ledState[i]+5 : targetLedState[i];
      } else {
        ledState[i] = ledState[i]-targetLedState[i] > 5 ? ledState[i]-5 : targetLedState[i];
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

  if (millis() >= nextLedCycle) {
    nextLedCycle = millis() + 10; // roughly 100 fps

    if (brightness != targetBrightness) {
      if (targetBrightness > brightness)
        if ((targetBrightness - brightness) > 10)
          brightness += 10;
        else
          brightness = targetBrightness;
      else
        if ((brightness - targetBrightness) > 10)
          brightness -= 10;
        else
          brightness = targetBrightness;

      FastLED.setBrightness(brightness);

      if (brightness == 0 && targetMode != NONE) {
        changeModeTo(targetMode);
        targetMode = NONE;
        targetBrightness = savedBrightness;
      }
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
        fill_rainbow( leds, LED_COUNT, loop_count, 2);
      } else if (mode == CHRISTMAS) {
        if (loop_count % 2 == 0) {
          for (uint16_t i = 0; i < (LED_COUNT/2); i++) {
            uint8_t t = ((loop_count/2) + i) / 7 % 3;
            CRGB color;
            if (t == 0) {
              color = CRGB::Red;
            } else if (t == 1) {
              color = CRGB::Green;
            } else {
              color = CRGB::Blue;
            }

            //int16_t l = 93-i;
            //int16_t r = 94+i;

            //if (l < 0)
            //  l = LED_COUNT - (i-93);

            int16_t l = 231 + i;
            int16_t r = 229 - i;

            if (l >= LED_COUNT)
              l -= LED_COUNT;

            leds[l] = color;
            leds[r] = color;
          }
        }
        loop_count++;

        if (loop_count >= 252) // 252 is divisible by 7 and 2
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
            meteorColor[m] = randomBrightColor();
            meteorSpeed[m] = random8(1,3); // random number between 1 and 2
          }
        }
        loop_count++;
      }
    }
  }

  if (ledsUpdated) {
    ledsUpdated = false;
    FastLED.show();
    // fps++;
  }

  /*
  if (millis() > nextPublishTime) {
    Particle.publish("FPS", String::format("%d %d", millis()-lastPublishTime, fps), PRIVATE);
    lastPublishTime = nextPublishTime;
    nextPublishTime = millis() + 1000;
    fps = 0;
  }
  */

}