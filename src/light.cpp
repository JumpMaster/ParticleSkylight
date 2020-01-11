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
  } else if (newMode == BOUNCE) {
    
    // bounces[0].enabled = true;
    // bounces[0].direction = random8(0, 2);
    // bounces[0].speed = 20;
    // lastBounceRelease = millis();

    for (int i = 1; i < BOUNCE_ARRAY_SIZE; i++)
      bounces[i].enabled = false;
    /*
    bounces[0].enabled = true;
    bounces[0].position[0] = 0;
    bounces[0].direction = true;
    bounces[0].color = CRGB::Blue;
    bounces[0].speed = 4;

    bounces[1].enabled = true;
    bounces[1].position[0] = 0;
    bounces[1].direction = true;
    bounces[1].color = CRGB::Yellow;
    bounces[1].speed = 10;

    bounces[2].enabled = true;
    bounces[2].position[0] = LED_COUNT-1;
    bounces[2].direction = false;
    bounces[2].color = CRGB::Green;
    bounces[2].speed = 2;

    bounces[3].enabled = true;
    bounces[3].position[0] = LED_COUNT-1;
    bounces[3].direction = false;
    bounces[3].color = CRGB::Green;
    bounces[3].speed = 3;

    bounces[4].enabled = true;
    bounces[4].position[0] = 0;
    bounces[4].direction = true;
    bounces[4].color = CRGB::Magenta;
    bounces[4].speed = 10;
*/
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

void Light::addColorToLed(uint16_t p, CRGB c) {
  leds[p].r = qadd8(c.r, leds[p].r);
  leds[p].g = qadd8(c.g, leds[p].g);
  leds[p].b = qadd8(c.b, leds[p].b);  
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
  uint32_t currentTime = millis();

  if (currentTime >= nextLedCycle) {
    nextLedCycle = currentTime + 5; // roughly 200 fps

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
            addColorToLed(meteorPosition[m], meteorColor[m]);
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
      } else if (mode == BOUNCE) {
        FastLED.clear();

        // Update bounces
        for (int i = 0; i < BOUNCE_ARRAY_SIZE; i++) {
          if (!bounces[i].enabled)
            continue;

          if (loop_count % bounces[i].speed != 0) {
            for (int8_t a = BOUNCE_LENGTH-1; a > 0; a--) {
              bounces[i].position[a] = bounces[i].position[a-1];
            }
          
            if (bounces[i].direction) {
              bounces[i].position[0] = bounces[i].position[1] + 1;
            } else {
              bounces[i].position[0] = bounces[i].position[1] - 1;
            }

            if (bounces[i].position[0] >= LED_COUNT) {
              if (bounces[i].direction)
                bounces[i].position[0] = 0;
              else
                bounces[i].position[0] = LED_COUNT-1;
            }
          }
        }

        // Draw Bounces
        for (int i = 0; i < BOUNCE_ARRAY_SIZE; i++) {
          if (!bounces[i].enabled)
            continue;
          
          for (int a = 0; a < BOUNCE_LENGTH; a++) {
            if (bounces[i].position[a] < LED_COUNT-1)
              leds[bounces[i].position[a]] = bounces[i].color;
          }
        }

        //
        // Detect collisions
        //
        // Face-on-collisions
        for (uint8_t i = 0; i < BOUNCE_ARRAY_SIZE; i++) {
          if (!bounces[i].enabled)
            continue;

          uint16_t collision = bounces[i].direction ? bounces[i].position[0]+1 : bounces[i].position[0]-1;

          if (collision >= LED_COUNT)
            collision = bounces[i].direction ? 0 : LED_COUNT-1;
   
          for (uint8_t a = i+1; a < BOUNCE_ARRAY_SIZE; a++) {
            if (!bounces[a].enabled ||
                bounces[i].direction == bounces[a].direction)
              continue;
            
            if (bounces[a].position[0] == collision || bounces[i].position[0] == bounces[a].position[0]) {
              if (bounces[i].speed == 2)
                bounces[i].fadeOut = true;
              else
                bounces[i].speed--;

              if (bounces[a].speed == 2)
                bounces[a].fadeOut = true;
              else
                bounces[a].speed--;

              bounces[i].direction = !bounces[i].direction;
              bounces[a].direction = !bounces[a].direction;
            }
          }
        }


        // Rear collisions
        for (int i = 0; i < BOUNCE_ARRAY_SIZE; i++) {
          if (!bounces[i].enabled)
            continue;

          uint16_t collision = bounces[i].direction ? bounces[i].position[0]+1 : bounces[i].position[0]-1;
          
          if (collision >= LED_COUNT)
            collision = bounces[i].direction ? 0 : LED_COUNT-1;

          for (int a = 0; a < BOUNCE_ARRAY_SIZE; a++) {
            if (a == i ||
                !bounces[a].enabled ||
                bounces[i].direction != bounces[a].direction ||
                bounces[i].speed <= bounces[a].speed)
              continue;

            if (bounces[a].position[BOUNCE_LENGTH-1] == collision || bounces[a].position[BOUNCE_LENGTH-1] == bounces[i].position[0]) {
              uint8_t i_speed = 2;

              if (bounces[i].speed == 2)
                bounces[a].fadeOut = true;
              else
                i_speed = bounces[i].speed--;

              if (bounces[a].speed == 2)
                bounces[i].fadeOut = true;
              else
                bounces[i].speed = bounces[a].speed--;

              bounces[a].speed = i_speed;
            }
          }
        }

        // Deal with stopped bounces
        for (int i = 0; i < BOUNCE_ARRAY_SIZE; i++) {
          if (bounces[i].fadeOut) {
            // if (loop_count % 2 == 0)
              bounces[i].color.fadeToBlackBy(2);

            if (!bounces[i].color)
              bounces[i].enabled = false;
          }
        }

        // Deal with disabled bounces
        if (currentTime >= nextBounceRelease) {
          for (int i = 0; i < BOUNCE_ARRAY_SIZE; i++) {
            if (!bounces[i].enabled) {
              // Survey visible bounces for direction
              uint8_t forwards = 0;
              uint8_t backwards = 0;
              for (uint8_t a = 0; a < BOUNCE_ARRAY_SIZE; a++) {
                if (bounces[a].enabled) {
                  if (bounces[a].direction)
                    forwards++;
                  else
                    backwards++;
                }
              }

              nextBounceRelease = currentTime + 2000UL;
              bounces[i].enabled = true;
              bounces[i].fadeOut = false;

              if (forwards == 0)
                bounces[i].direction = true;
              else if (backwards == 0)
                bounces[i].direction = false;
              else
                bounces[i].direction = random8(0, 2);

              bounces[i].position[0] = bounces[i].direction ? 0 : LED_COUNT-1;
              for (uint8_t a = 1; a < BOUNCE_LENGTH; a++)
                bounces[i].position[a] = bounces[i].position[0];

              bounces[i].color = randomBrightColor(true);
              bounces[i].speed = 20;
              break;
            }
          }
        }

        loop_count++;
      }
    }
  }

  if (ledsUpdated) {
    ledsUpdated = false;
    FastLED.show();
    if (showFPS)
      fps++;
  }

  if (showFPS && currentTime > nextPublishTime) {
    Particle.publish("FPS", String::format("%d %d", currentTime-lastPublishTime, fps), PRIVATE);
    lastPublishTime = nextPublishTime;
    nextPublishTime = currentTime + 1000;
    fps = 0;
  }
}