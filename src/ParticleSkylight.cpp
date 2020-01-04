#include "Particle.h"
#include "mqtt.h"
#include "papertrail.h"
#include "secrets.h"
#include "light.h"

// Stubs
void mqttCallback(char* topic, byte* payload, unsigned int length);
void publishPowerState();
void publishMode();
void publishColor();
void publishBrightness();

Light light;
unsigned long nextRGBPublish;

uint32_t resetTime = 0;
retained uint32_t lastHardResetTime;
retained int resetCount;

MQTT mqttClient(mqttServer, 1883, mqttCallback);
uint32_t lastMqttConnectAttempt;
const int mqttConnectAttemptTimeout1 = 5000;
const int mqttConnectAttemptTimeout2 = 30000;
unsigned int mqttConnectionAttempts;

unsigned int udpLocalPort = 8888;
UDP Udp;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = '\0';

  Log.info("%s - %s", topic, p);
  if (strcmp(topic, "home/light/playroom/skylight/switch/set") == 0) {
    bool publishState = false;
    if (strcmp(p, "ON") == 0 && !light.isOn()) {
      light.on();
      publishState = true;
    } else if (strcmp(p, "OFF") == 0 && light.isOn()) {
      light.off();
      publishState = true;
    }

    if (publishState && mqttClient.isConnected()) {
      publishPowerState();
      publishBrightness();
    }
  } else if (strcmp(topic, "home/light/playroom/skylight/rgb/set") == 0) {
    int r, g, b = 0;
    char *a;
    a = strtok(p, ",");
    r = atoi(a);
    a = strtok(NULL, ",");
    g = atoi(a);
    a = strtok(NULL, ",");
    b = atoi(a);
    light.setColor(r, g, b);
    light.setMode(Light::STATIC);
    publishMode();
    publishColor();
  } else if (strcmp(topic, "home/light/playroom/skylight/effect/set") == 0) {
    if (strcmp(p, "static") == 0)
      light.setMode(Light::STATIC);
    else if (strcmp(p, "rainbow") == 0)
      light.setMode(Light::RAINBOW);
    else if (strcmp(p, "christmas") == 0)
      light.setMode(Light::CHRISTMAS);
    else if (strcmp(p, "meteors") == 0)
      light.setMode(Light::METEORS);
    else if (strcmp(p, "light_swipe") == 0)
      light.setMode(Light::LIGHT_SWIPE);
    publishMode();
  } else if (strcmp(topic, "home/light/playroom/skylight/brightness/set") == 0) {
    char b = atoi(p);
    light.setBrightness(b);
    mqttClient.publish("home/light/playroom/skylight/brightness", p, true);
    publishBrightness();
  }

  light.saveSettings();
}

void connectToMQTT() {
  lastMqttConnectAttempt = millis();
  bool mqttConnected = mqttClient.connect(System.deviceID(), mqttUsername, mqttPassword);
  if (mqttConnected) {
    mqttConnectionAttempts = 0;
    Log.info("MQTT Connected");
    mqttClient.subscribe("home/light/playroom/skylight/+/set");

    publishPowerState();
    publishMode();
    publishColor();
    publishBrightness();
  } else {
    mqttConnectionAttempts++;
    Log.info("MQTT failed to connect");
  }
}

void publishPowerState() {
  mqttClient.publish("home/light/playroom/skylight/switch", light.isOn() ? "ON" : "OFF", true);
}

void publishMode() {
  if (light.getMode() == Light::STATIC)
    mqttClient.publish("home/light/playroom/skylight/effect", "static", true);
  else if (light.getMode() == Light::RAINBOW)
    mqttClient.publish("home/light/playroom/skylight/effect", "rainbow", true);
  else if (light.getMode() == Light::CHRISTMAS)
    mqttClient.publish("home/light/playroom/skylight/effect", "christmas", true);
  else if (light.getMode() == Light::METEORS)
    mqttClient.publish("home/light/playroom/skylight/effect", "meteors", true);
  else if (light.getMode() == Light::LIGHT_SWIPE)
    mqttClient.publish("home/light/playroom/skylight/effect", "light_swipe", true);
}

void publishColor() {
  char rgbString[12];
  uint32_t c = light.getColor();
  uint8_t
    r = (uint8_t)(c >> 16),
    g = (uint8_t)(c >>  8),
    b = (uint8_t)c;
  sprintf(rgbString, "%d,%d,%d", r, g, b);
  mqttClient.publish("home/light/playroom/skylight/rgb", rgbString, true);
}

void publishBrightness() {
  char b[4];
  sprintf(b, "%d", light.getBrightness());
  mqttClient.publish("home/light/playroom/skylight/brightness", b, true);
}

void random_seed_from_cloud(unsigned seed) {
   srand(seed);
}

SYSTEM_THREAD(ENABLED);
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL)); // selects the u.FL antenna

PapertrailLogHandler papertrailHandler(papertrailAddress, papertrailPort,
  "ArgonTexecom", System.deviceID(),
  LOG_LEVEL_NONE, {
  { "app", LOG_LEVEL_ALL }
  // TOO MUCH!!! { “system”, LOG_LEVEL_ALL },
  // TOO MUCH!!! { “comm”, LOG_LEVEL_ALL }
});

void setup() {
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  light.loadSettings();
  light.setup();

  Particle.variable("resetTime", resetTime);
  Particle.publishVitals(900);

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

  Udp.begin(udpLocalPort);

  Log.info("Boot complete. Reset count = %d", resetCount);

  connectToMQTT();
}

void loop() {

  light.loop();

  if (millis() > nextRGBPublish && !light.isColorPublished()) {
    nextRGBPublish = millis() + 500;
    publishColor();
  }
  
  if (mqttClient.isConnected()) {
    mqttClient.loop();
  } else if ((mqttConnectionAttempts < 5 && millis() > (lastMqttConnectAttempt + mqttConnectAttemptTimeout1)) ||
              millis() > (lastMqttConnectAttempt + mqttConnectAttemptTimeout2)) {
    connectToMQTT();
  }

  if (Udp.parsePacket() > 0) {

    // Read first char of data received
    char c = Udp.read();
    // Ignore other chars
    while(Udp.available())
      Udp.read();
    
    uint8_t b = c - '0';
    if (b >= 0 && b <= 9) {
      light.setBrightness((b+1) * 25.5);
    }
  } 
}