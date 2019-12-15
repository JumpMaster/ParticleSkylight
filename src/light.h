#ifndef __LIGHT_H_
#define __LIGHT_H_

#include "Particle.h"
#include "FastLED.h"
#define PARTICLE_NO_ARDUINO_COMPATIBILITY 1
FASTLED_USING_NAMESPACE

#define LED_COUNT 273
#define LED_PIN D0

class Light {

public:
  typedef enum { // Leave 0 undefined for loading test
    STATIC = 1,
    RAINBOW = 2,
    CHRISTMAS = 3
  } MODES;
  
  void setup();
  void setMode(MODES newMode);
  MODES getMode();
  void setColor(uint8_t r, uint8_t g, uint8_t b);
  uint32_t getColor();
  void setBrightness(uint8_t b);
  uint8_t getBrightness();
  void on();
  void off();
  bool isOn();
  bool isColorPublished();
  void loadSettings();
  void saveSettings();
  void loop();
  
private:
  struct SaveData {
    MODES mode;
    uint8_t brightness;
    CRGB color;
  };
  struct CRGB leds[LED_COUNT];
  const uint8_t fadePauseTime = 20;
  const uint8_t transitionPauseTime = 5;
  uint8_t pauseTime = fadePauseTime;
  unsigned long nextLedCycle;
  unsigned long nextBrightnessCycle;
  uint8_t loop_count = 0;
  bool powerState = false;
  bool colorPublished = false;
  MODES mode = RAINBOW;

  CRGB ledState = CRGB::Black;
  CRGB targetLedState = CRGB::Black;
  CRGB savedLedState = CRGB::Red;
  uint8_t brightness = 0;
  uint8_t savedBrightness = 255;
  uint8_t targetBrightness = 0;
  bool updateLeds();
};
#endif