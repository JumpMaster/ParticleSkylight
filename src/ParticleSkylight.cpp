#include "Particle.h"
#include "mqtt.h"
#include "papertrail.h"
#include "secrets.h"

typedef enum { // Leave 0 undefined for load test
  STATIC = 1,
  RAINBOW = 2
} MODES;

struct SaveData {
  MODES mode;
  uint8_t brightness;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

// Stubs
void mqttCallback(char* topic, byte* payload, unsigned int length);
void setMode(MODES newMode);
void setColor(uint8_t r, uint8_t g, uint8_t b);
void powerLights(bool on);

const uint8_t fadePauseTime = 20;
const uint8_t transitionPauseTime = 5;
uint8_t pauseTime = fadePauseTime;

unsigned long nextCycle;
int stage = 0;
bool isOn = false;
bool publishColor = false;

MODES mode = RAINBOW;

int ledPins[3] = {A0, A1, A2};
uint8_t ledState[3];
uint8_t requestedLedState[3] = {0, 0, 0};
uint8_t savedLedState[3] = {255, 0, 0};
uint8_t brightness = 0;
uint8_t savedBrightness = 255;

unsigned long nextRGBPublish;

uint32_t resetTime = 0;
retained uint32_t lastHardResetTime;
retained int resetCount;

void loadSettings() {
  SaveData saveData;
  EEPROM.get(0, saveData);
  if (saveData.mode > 0) {
    mode = saveData.mode;
    savedBrightness = saveData.brightness;
    savedLedState[0] = saveData.red;
    savedLedState[1] = saveData.green;
    savedLedState[2] = saveData.blue;
    
    brightness = savedBrightness;
    for (int i; i < 3; i++)
      requestedLedState[i] = savedLedState[i];
  }
}

void saveSettings() {
  SaveData saveData;
  saveData.mode = mode;
  saveData.brightness = savedBrightness;
  saveData.red = savedLedState[0];
  saveData.green = savedLedState[1];
  saveData.blue = savedLedState[2];
  EEPROM.put(0, saveData);
}

MQTT mqttClient(mqttServer, 1883, mqttCallback);
uint32_t lastMqttConnectAttempt;
const int mqttConnectAttemptTimeout1 = 5000;
const int mqttConnectAttemptTimeout2 = 30000;
unsigned int mqttConnectionAttempts;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = '\0'; 

    Log.info("%s - %s", topic, p);
    if (strcmp(topic, "home/light/playroom/skylight/switch/set") == 0) {
      if (strcmp(p, "ON") == 0)
        powerLights(true);
      else if (strcmp(p, "OFF") == 0)
        powerLights(false);
    } else if (strcmp(topic, "home/light/playroom/skylight/rgb/set") == 0) {
      int r, g, b = 0;
      char *a;
      a = strtok(p, ",");
      r = atoi(a);
      a = strtok(NULL, ",");
      g = atoi(a);
      a = strtok(NULL, ",");
      b = atoi(a);
      setColor(r, g, b);
      setMode(STATIC);
    } else if (strcmp(topic, "home/light/playroom/skylight/effect/set") == 0) {
      if (strcmp(p, "static") == 0) {
        setMode(STATIC);
      } else if (strcmp(p, "rainbow") == 0) {
        setMode(RAINBOW);
      }
    } else if (strcmp(topic, "home/light/playroom/skylight/brightness/set") == 0) {
      char b = atoi(p);
      brightness = b;
      savedBrightness = brightness;
      mqttClient.publish("home/light/playroom/skylight/brightness", p, true);
    }

    saveSettings();
}

void connectToMQTT() {
    lastMqttConnectAttempt = millis();
    bool mqttConnected = mqttClient.connect(System.deviceID(), mqttUsername, mqttPassword);
    if (mqttConnected) {
        mqttConnectionAttempts = 0;
        Log.info("MQTT Connected");
        mqttClient.subscribe("home/light/playroom/skylight/+/set");

        mqttClient.publish("home/light/playroom/skylight/switch", isOn ? "ON" : "OFF", true);
        char b[4];
        sprintf(b, "%d", brightness);
        mqttClient.publish("home/light/playroom/skylight/brightness", b, true);
        if (mode == STATIC) {
          mqttClient.publish("home/light/playroom/skylight/effect", "static", true);
          char rgbString[12];
          sprintf(rgbString, "%d,%d,%d", requestedLedState[0], requestedLedState[1], requestedLedState[2]);
          mqttClient.publish("home/light/playroom/skylight/rgb", rgbString, true);
        } else if (mode == RAINBOW) {
          mqttClient.publish("home/light/playroom/skylight/effect", "rainbow", true);
        }

    } else {
        mqttConnectionAttempts++;
        Log.info("MQTT failed to connect");
    }
}

PapertrailLogHandler papertrailHandler(papertrailAddress, papertrailPort,
  "ArgonTexecom", System.deviceID(),
  LOG_LEVEL_NONE, {
  { "app", LOG_LEVEL_ALL }
  // TOO MUCH!!! { “system”, LOG_LEVEL_ALL },
  // TOO MUCH!!! { “comm”, LOG_LEVEL_ALL }
});

void selectExternalMeshAntenna() {
#if (PLATFORM_ID == PLATFORM_ARGON)
    digitalWrite(ANTSW1, 1);
    digitalWrite(ANTSW2, 0);
#elif (PLATFORM_ID == PLATFORM_BORON)
    digitalWrite(ANTSW1, 0);
#else
    digitalWrite(ANTSW1, 0);
    digitalWrite(ANTSW2, 1);
#endif
}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
  savedLedState[0] = r;
  savedLedState[1] = g;
  savedLedState[2] = b;
}

void setMode(MODES newMode) {
  if (newMode == RAINBOW) {
    requestedLedState[0] = 255;
    requestedLedState[1] = 0;
    requestedLedState[2] = 0;
    stage = 0;
    mode = RAINBOW;
    pauseTime = fadePauseTime;
    if (mqttClient.isConnected())
      mqttClient.publish("home/light/playroom/skylight/effect", "rainbow", true);
  } else if (newMode == STATIC) {
    for (int i = 0; i < 3; i++)
      requestedLedState[i] = savedLedState[i];
    mode = STATIC;
    pauseTime = transitionPauseTime;
    if (mqttClient.isConnected()) {
      mqttClient.publish("home/light/playroom/skylight/effect", "static", true);
    }
  }
}

void powerLights(bool on) {
  bool publishState = false;
  if (!isOn && on) {
    brightness = savedBrightness;
    publishState = true;
    isOn = true;
  } else if (isOn && !on) {
    savedBrightness = brightness;
    brightness = 0;
    publishState = true;
    isOn = false;
    pauseTime = transitionPauseTime;
  }

  if (publishState && mqttClient.isConnected()) {
    mqttClient.publish("home/light/playroom/skylight/switch", isOn ? "ON" : "OFF", true);
    char b[4];
    sprintf(b, "%d", brightness);
    mqttClient.publish("home/light/playroom/skylight/brightness", b, true);
  }
}

bool updateLeds() {
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

STARTUP(selectExternalMeshAntenna());

void setup() {
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  loadSettings();

  waitFor(Particle.connected, 30000);

  do {
      resetTime = Time.now();
      Particle.process();
  } while (resetTime < 1500000000 || millis() < 10000);

  if (System.resetReason() == RESET_REASON_PANIC) {
    if ((Time.now() - lastHardResetTime) < 120) {
      resetCount++;
    } else {
      resetCount = 1;
    }

    lastHardResetTime = Time.now();

    if (resetCount > 3) {
      System.enterSafeMode();
    }
  } else {
    resetCount = 0;
  }

  Log.info("Boot complete. Reset count = %d", resetCount);

  connectToMQTT();
}

void loop() {

  if (millis() > nextCycle) {

    nextCycle = millis() + pauseTime;

    if (isOn || (ledState[0]+ledState[1]+ledState[2]) > 0) {
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

      if (publishColor && millis() > nextRGBPublish) {
        publishColor = false;
        nextRGBPublish = millis() + 500;
        char rgbString[12];
        sprintf(rgbString, "%d,%d,%d", ledState[0], ledState[1], ledState[2]);
        mqttClient.publish("home/light/playroom/skylight/rgb", rgbString, true);
      }
    }
  }

  if (mqttClient.isConnected()) {
    mqttClient.loop();
  } else if ((mqttConnectionAttempts < 5 && millis() > (lastMqttConnectAttempt + mqttConnectAttemptTimeout1)) ||
              millis() > (lastMqttConnectAttempt + mqttConnectAttemptTimeout2)) {
    connectToMQTT();
  }
}