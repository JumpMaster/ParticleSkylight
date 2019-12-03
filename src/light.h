#ifndef __LIGHT_H_
#define __LIGHT_H_

#include "Particle.h"

class Light {

public:
    typedef enum { // Leave 0 undefined for loading test
        STATIC = 1,
        RAINBOW = 2
    } MODES;

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
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    };

    const uint8_t fadePauseTime = 20;
    const uint8_t transitionPauseTime = 5;
    uint8_t pauseTime = fadePauseTime;
    unsigned long nextCycle;
    int stage = 0;
    bool powerState = false;
    bool colorPublished = false;
    MODES mode = RAINBOW;

    int ledPins[3] = {A0, A1, A2};
    uint8_t ledState[3];
    uint8_t requestedLedState[3] = {0, 0, 0};
    uint8_t savedLedState[3] = {255, 0, 0};
    uint8_t brightness = 0;
    uint8_t savedBrightness = 255;

    bool updateLeds();
};
#endif