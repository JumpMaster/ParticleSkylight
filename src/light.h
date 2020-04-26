#ifndef __LIGHT_H_
#define __LIGHT_H_

#include "Particle.h"
#include "FastLED.h"
#define PARTICLE_NO_ARDUINO_COMPATIBILITY 1
FASTLED_USING_NAMESPACE

#define LED_COUNT 273
#define LED_PIN D0
#define BOUNCE_ARRAY_SIZE 5
#define BOUNCE_LENGTH 5

class Light {

public:
  typedef enum { // Leave 0 undefined for loading test
    NONE = 0,
    STATIC = 1,
    RAINBOW = 2,
    CHRISTMAS = 3,
    METEORS = 4,
    LIGHT_SWIPE = 5,
    BOUNCE = 6,
  } MODES;
  bool showFPS = false;
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
  CRGB randomBrightColor(bool includeWhite);
  
private:
  struct SaveData {
    MODES mode;
    uint8_t brightness;
    CRGB color;
  };
  struct BounceData {
    bool enabled = false;
    bool fadeOut = false;
    bool direction = true;
    uint8_t speed = 20;
    uint16_t position[5];
    CRGB color = CRGB::White;
  };
  CRGB leds[LED_COUNT];
  uint32_t nextLedCycle;
  uint16_t loop_count = 0;
  bool powerState = false;
  bool colorPublished = false;
  MODES mode = RAINBOW;
  MODES targetMode = NONE;

  // CRGB ledState = CRGB::Black;
  CRGB targetColor = CRGB::Black;
  CRGB savedColor = CRGB::Red;
  CRGB previousColor = CRGB::Black;
  uint8_t brightness = 0;
  uint8_t savedBrightness = 255;
  uint8_t targetBrightness = 0;

  CRGB meteorColor[2] = {CRGB::Blue, CRGB::HotPink};
  uint16_t meteorPosition[2] = {0, 0};
  uint8_t meteorSpeed[2] = {1, 2};
  void updateLeds();
  void addColorToLed(uint16_t p, CRGB c);

  void changeModeTo(MODES newMode);
  uint32_t nextPublishTime;
  uint32_t lastRuntime;
  uint32_t lastLedShow;
  uint16_t fps;

  BounceData bounces[BOUNCE_ARRAY_SIZE];
  uint32_t nextBounceRelease;
  bool lightsOn = false;
};
#endif