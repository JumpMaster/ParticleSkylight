/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#line 1 "/home/kevin/Documents/GitHub/ParticleSkylight/src/ParticleSkylight.ino"
#include "Particle.h"
#include "mqtt.h"
#include "papertrail.h"
#include "secrets.h"

void connectToMQTT();
void selectExternalMeshAntenna();
void powerLights(bool on);
bool updateLeds();
void setup();
void loop();
#line 6 "/home/kevin/Documents/GitHub/ParticleSkylight/src/ParticleSkylight.ino"
typedef enum {
  OFF,
  STATIC_COLOR,
  RAINBOW
} MODES;

// Stubs
void mqttCallback(char* topic, byte* payload, unsigned int length);
void setMode(MODES newMode, char r, char g, char b);

int cycle = 20;
unsigned long nextCycle;
int stage = 0;
bool isOn = false;

MODES mode = OFF;

int ledPins[3] = {A0, A1, A2};
char ledState[3];
char requestedLedState[3];
char savedLedState[3] = {255, 0, 0};

uint32_t resetTime = 0;
retained uint32_t lastHardResetTime;
retained int resetCount;

MQTT mqttClient(mqttServer, 1883, mqttCallback);
uint32_t lastMqttConnectAttempt;
const int mqttConnectAtemptTimeout1 = 5000;
const int mqttConnectAtemptTimeout2 = 30000;
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
      setMode(STATIC_COLOR, r, g, b);
    }
}

void connectToMQTT() {
    lastMqttConnectAttempt = millis();
    bool mqttConnected = mqttClient.connect(System.deviceID(), mqttUsername, mqttPassword);
    if (mqttConnected) {
        mqttConnectionAttempts = 0;
        Log.info("MQTT Connected");
        mqttClient.subscribe("home/light/playroom/skylight/+/set");
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

void setMode(MODES newMode, char r, char g, char b) {
    
  if (newMode == OFF) {
    for (int i = 0; i < 3; i++) {
      if (mode == 1)
        savedLedState[i] = ledState[i];
      requestedLedState[i] = 0;
    }
    isOn = false;
    mode = newMode;
  } else if (newMode == RAINBOW) {
    for (int i = 0; i < 3; i++)
      requestedLedState[i] = savedLedState[i];
    mode = newMode;
  } else if (newMode == STATIC_COLOR) {
    if (mode == 1) {
      for (int i = 0; i < 3; i++)
        savedLedState[i] = ledState[i];
    }
    requestedLedState[0] = r;
    requestedLedState[1] = g;
    requestedLedState[2] = b;
    mode = newMode;
  }
}

void powerLights(bool on) {
  bool publishState = false;
  if (!isOn && on) {
    setMode(RAINBOW, 0, 0, 0);
    publishState = true;
    isOn = true;
  } else if (isOn && !on) {
    setMode(OFF, 0, 0, 0);
    publishState = true;
    isOn = false;
  }

  if (publishState && mqttClient.isConnected())
    mqttClient.publish("home/light/playroom/skylight/switch", isOn ? "ON" : "OFF", true);
}

bool updateLeds() {
    bool changesMade = false;
    
    for (int i = 0; i < 3; i++) {
        if (requestedLedState[i] != ledState[i]) {
            changesMade = true;
            if (requestedLedState[i] > ledState[i])
                ledState[i]++;
            else
                ledState[i]--;
            
            analogWrite(ledPins[i], ledState[i]);
        }
    }
    
    return changesMade;
}

STARTUP(selectExternalMeshAntenna());

void setup() {
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);

  // Particle.function("setState", setState);

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

  Log.info("Boot complete");//;. Reset count = %d", resetCount);

  connectToMQTT();
}

void loop() {
    
  if (millis() > nextCycle) {
      
    nextCycle = millis() + cycle;
    
    if (!updateLeds() && mode == RAINBOW) {
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

  if (mqttClient.isConnected()) {
    mqttClient.loop();
  } else if ((mqttConnectionAttempts < 5 && millis() > (lastMqttConnectAttempt + mqttConnectAtemptTimeout1)) ||
                millis() > (lastMqttConnectAttempt + mqttConnectAtemptTimeout2)) {
    connectToMQTT();
  }
}