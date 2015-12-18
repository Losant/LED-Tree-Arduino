#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include <math.h>

#include <WiFi101.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// TODO: revert to background animations.

// Which pin the center light strip is connected to.
#define CENTER_PIN 8

// Which pin the outer light strip is connected to.
#define OUTER_PIN 9

// Structure setup.
#define DEVICE_ID "DEVICE_ID"
#define ACCESS_KEY "ACCESS_KEY"
#define ACCESS_SECRET "ACCESS_SECRET"
#define BROKER "broker.getstructure.io"
#define TOPIC "structure/DEVICE_ID/message"

// WiFi setup.
char ssid[] = "MORTAR";
char pass[] = "BrickOTR";
int status = WL_IDLE_STATUS;
WiFiClient wifiClient;

// The MQTT client.
PubSubClient client(BROKER, 1883, wifiClient);

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel centerStrip = Adafruit_NeoPixel(240, CENTER_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel outerStrip = Adafruit_NeoPixel(960, OUTER_PIN, NEO_GRB + NEO_KHZ800);

unsigned long lastAnimationRunTime = 0;
uint32_t lastAmbientColor = 0;

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

struct WarpCoreAnimation {
  unsigned long startTime;
  unsigned long currentTime;
  unsigned long duration;     // animation length
  int centerLights;           // center lights to light up
  int outerLights;            // outer lights to light up
  uint32_t offStartColor;     // off color at the start of the animation
  uint32_t offStopColor;      // off color at the end of the animation
  uint32_t onColor;           // on color of center lights
  int pulseSpeedStart;
  int pulseSpeedEnd;
  int currentPulseSpeed;
  unsigned long currentPulseStart;
  unsigned long currentPulseDuration;
  bool pulsing;
  bool done;
  Adafruit_NeoPixel *centerStrip;
  Adafruit_NeoPixel *outerStrip;
};

struct FireworkAnimation {
  unsigned long startTime;
  unsigned long currentTime;
  unsigned long duration;
  float shotDuration;
  float burstDuration;
  float fadeDuration;
  unsigned long burstStartTime;
  unsigned long fadeStartTime;
  unsigned long lastFadeUpdate;
  bool done;
  Adafruit_NeoPixel *centerStrip;
  Adafruit_NeoPixel *outerStrip;
};

struct FadeAnimation {
  unsigned long startTime;
  unsigned long currentTime;
  unsigned long duration;
  uint32_t toColor;
  uint32_t fromColor;
  bool done;
  Adafruit_NeoPixel *centerStrip;
  Adafruit_NeoPixel *outerStrip;
};

struct BackgroundAnimation {
  bool done;
  bool runningWarpCore;
  bool runningFadeRed;
  bool runningFadeGreen;
  bool runningFadeBlue;
  bool runningFadeBlack;
  bool runningFirework;
};

BackgroundAnimation backgroundAnimation {
  false,
  false,
  false,
  false,
  false,
  false,
  false
};

WarpCoreAnimation warpCoreAnimation { 
  0,
  0,
  10000,
  8,
  80,
  centerStrip.Color(0, 0, 255),
  centerStrip.Color(255, 0, 0),
  centerStrip.Color(0, 255, 255),
  2000,
  500,
  0,
  0,
  0,
  false,
  true,
  &centerStrip,
  &outerStrip
};

FireworkAnimation fireworkAnimation {
  0,
  0,
  10000,
  0.1,
  0.05,
  0.85,
  0,
  0,
  0,
  true,
  &centerStrip,
  &outerStrip
};

FadeAnimation fadeAnimation {
  0,
  0,
  3000,
  0,
  0,
  true,
  &centerStrip,
  &outerStrip
};

void setup() {

  Serial.begin(115200);
  Serial.println("LED Tree Application Started");

  while(status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);

    delay(3000);
  }

  Serial.println("Connected to network!");

  client.setCallback(mqttMessageReceived);

  centerStrip.begin();
  outerStrip.begin();
  centerStrip.show(); // Initialize all pixels to 'off'
  outerStrip.show();
}

/**
 * Called whenever an mqtt message is published on a subscribed topic.
 * topic - the topic that was published to.
 * payload - the contents of the message.
 * length - the length of the payload.
 */
void mqttMessageReceived(char *topic, byte *payload, unsigned int length) {

  StaticJsonBuffer<1024> msgBuffer;
  StaticJsonBuffer<1024> payloadBuffer;

  Serial.println("Message Received");
  Serial.println((char *)payload);
  
  JsonObject &root = msgBuffer.parseObject((char *)payload);

  if(!root.success()) {
    Serial.println("ERROR: Failed to parse JSON body.");
    return;
  }
  
  JsonObject &payloadRoot = payloadBuffer.parseObject(root["payload"].asString());

  if(!payloadRoot.success()) {
    Serial.println("ERROR: Failed to parse payload JSON body.");
    return;
  }

  if(!payloadRoot.containsKey("animation")) {
    Serial.println("ERROR: No animation defined on payload.");
    return;
  }

  stopAllAnimations();

  // Fade.
  if(strcmp(payloadRoot["animation"], "fade") == 0) {

    uint32_t fromColor = centerStrip.Color(
        payloadRoot["options"]["from"]["r"],
        payloadRoot["options"]["from"]["g"],
        payloadRoot["options"]["from"]["b"]);

    if(!fromColor) {
      fromColor = lastAmbientColor;
    }

    uint32_t toColor = centerStrip.Color(
      payloadRoot["options"]["to"]["r"],
      payloadRoot["options"]["to"]["g"],
      payloadRoot["options"]["to"]["b"]);

    unsigned long duration = payloadRoot["options"]["duration"];

    if(!duration) {
      duration = 1000;
    }

    fadeAnimation.duration = duration;
    fadeAnimation.fromColor = fromColor;
    fadeAnimation.toColor = toColor;
    startFade();
  }
  // Warp Core.
  else if(strcmp(payloadRoot["animation"], "warpcore") == 0) {
    unsigned long duration = payloadRoot["options"]["duration"];
    if(!duration) {
      duration = 10000;
    }
    warpCoreAnimation.duration = duration;
    startWarpCore();
  }
  // Fireworks.
  else if(strcmp(payloadRoot["animation"], "firework") == 0) {
    unsigned long duration = payloadRoot["options"]["duration"];
    if(!duration) {
      duration = 6000;
    }
    fireworkAnimation.duration = duration;
    startFireworks();
  }
  else {
    Serial.println("Unknown animation.");
  }
}

/**
 * Stops all animations. Typically called when a new animation request is received.
 */
void stopAllAnimations() {
  Serial.println("Stopping all animations.");
  warpCoreAnimation.done = true;
  fireworkAnimation.done = true;
  fadeAnimation.done = true;
  backgroundAnimation.done = true;
}

/**
 * Whether or not any animation is running.
 */
bool runningAnything() {
  return !fadeAnimation.done ||
    !fireworkAnimation.done ||
    !warpCoreAnimation.done;
}

/**
 * Reconnects to MQTT broker.
 */
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print(millis());
    Serial.print(": ");
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(DEVICE_ID, ACCESS_KEY, ACCESS_SECRET)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // ... and resubscribe
      client.subscribe(TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

unsigned long previousMillis = 0;
const long interval = 33;

void loop() {

  if(!client.connected()) {
    Serial.println(client.state());
    Serial.println("MQTT not connected.");
    reconnect();
  }

  client.loop();

  // If the tree has been idle for a few seconds, run the background animation.
  if(runningAnything()) {
    lastAnimationRunTime = millis();
  }

  if(millis() - lastAnimationRunTime > 5000) {
    startBackground();
  }

  // Loop the animations.
  warpCore();
  fireworks();
  fade();
  background();

  // Only render the lights if enough time has passed.
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval) {
    centerStrip.show();
    outerStrip.show();
    previousMillis = currentMillis;
  }
}

/**
 * Starts the warp core animation.
 */
void startWarpCore() {
  warpCoreAnimation.done = false;
  warpCoreAnimation.startTime = millis();
  warpCoreAnimation.pulsing = false;
  warpCoreAnimation.currentPulseSpeed = 0;

  Serial.println();
  Serial.println("Starting Animation: Warp Core");
  Serial.print("Duration: ");
  Serial.println(warpCoreAnimation.duration);
  Serial.println();

  lastAmbientColor = centerStrip.Color(255, 0, 0);
}

void startBackground() {
  backgroundAnimation.done = false;
  backgroundAnimation.runningWarpCore = false;
  backgroundAnimation.runningFadeBlue = false;
  backgroundAnimation.runningFadeGreen = false;
  backgroundAnimation.runningFadeRed = false;
  backgroundAnimation.runningFadeBlack = false;
  backgroundAnimation.runningFirework = false;
};

/**
 * Starts the fireworks animation.
 */
void startFireworks() {
  fireworkAnimation.done = false;
  fireworkAnimation.startTime = millis();
  fireworkAnimation.burstStartTime = 0;
  fireworkAnimation.fadeStartTime = 0;
  fireworkAnimation.lastFadeUpdate = 0;

  Serial.println();
  Serial.println("Starting Animation: Firework");
  Serial.print("Duration: ");
  Serial.println(fireworkAnimation.duration);
  Serial.println();

  lastAmbientColor = centerStrip.Color(0, 0, 0);
}

/**
 * Starts the fade animation.
 */
void startFade() {
  fadeAnimation.done = false;
  fadeAnimation.startTime = millis();

  Serial.println();
  Serial.println("Starting Animation: Fade");
  Serial.print("From: ");
  Serial.println(fadeAnimation.fromColor);
  Serial.print("To: ");
  Serial.println(fadeAnimation.toColor);
  Serial.print("Duration: ");
  Serial.println(fadeAnimation.duration);
  Serial.println();

  lastAmbientColor = fadeAnimation.toColor;
}

void background() {

  struct BackgroundAnimation *ani = &backgroundAnimation;

  if(ani->done) {
    return;
  }

  if(!ani->runningWarpCore &&
      !ani->runningFadeBlue &&
      !ani->runningFadeGreen &&
      !ani->runningFadeRed &&
      !ani->runningFadeBlack &&
      !ani->runningFirework) {
    ani->runningFadeBlue = true;
    fadeAnimation.fromColor = centerStrip.Color(0,0,0);
    fadeAnimation.toColor = centerStrip.Color(0,0,255);
    fadeAnimation.duration = 3000;
    startFade();
  }

  if(ani->runningFadeBlue && fadeAnimation.done) {
    ani->runningFadeBlue = false;
    ani->runningWarpCore = true;
    startWarpCore();
  }

  if(ani->runningWarpCore && warpCoreAnimation.done) {
    ani->runningWarpCore = false;
    ani->runningFadeGreen = true;
    fadeAnimation.fromColor = centerStrip.Color(255,0,0);
    fadeAnimation.toColor = centerStrip.Color(0,255,0);
    fadeAnimation.duration = 3000;
    startFade();
  }

  if(ani->runningFadeGreen && fadeAnimation.done) {
    ani->runningFadeGreen = false;
    ani->runningFadeRed = true;
    fadeAnimation.fromColor = centerStrip.Color(0,255,0);
    fadeAnimation.toColor = centerStrip.Color(255,0,0);
    fadeAnimation.duration = 3000;
    startFade();
  }

  if(ani->runningFadeRed && fadeAnimation.done) {
    ani->runningFadeRed = false;
    ani->runningFadeBlack = true;
    fadeAnimation.fromColor = centerStrip.Color(255,0,0);
    fadeAnimation.toColor = centerStrip.Color(0,0,0);
    fadeAnimation.duration = 3000;
    startFade();
  }

  if(ani->runningFadeBlack && fadeAnimation.done) {
    ani->runningFadeBlack = false;
    ani->runningFirework = true;
    startFireworks();
  }

  if(ani->runningFirework && fireworkAnimation.done) {
    ani->runningFirework = false;
  }
  
};

/**
 * Runs the fade animation.
 */
void fade() {

  struct FadeAnimation *ani = &fadeAnimation;
  
  if(ani->done) {
    return;
  }

  ani->currentTime = millis();
  unsigned long duration = ani->currentTime - ani->startTime;
  float percentDone = (float)duration / (float)ani->duration;

  if(duration >= ani->duration) {
    ani->done = true;
    return;
  }

  uint8_t rStop = ani->toColor >> 16;
  uint8_t gStop = ani->toColor >> 8;
  uint8_t bStop = ani->toColor;
  
  uint8_t rStart = ani->fromColor >> 16;
  uint8_t gStart = ani->fromColor >> 8;
  uint8_t bStart = ani->fromColor;

  uint8_t rNow = floor((float)rStart - (((float)rStart - (float)rStop) * percentDone));
  uint8_t gNow = floor((float)gStart - (((float)gStart - (float)gStop) * percentDone));
  uint8_t bNow = floor((float)bStart - (((float)bStart - (float)bStop) * percentDone));

  for(uint16_t i=0; i< ani->outerStrip->numPixels(); i++) {
    ani->outerStrip->setPixelColor(i, ani->outerStrip->Color(rNow, gNow, bNow));
  }

  for(uint16_t i=0; i< ani->centerStrip->numPixels(); i++) {
    ani->centerStrip->setPixelColor(i, ani->centerStrip->Color(rNow, gNow, bNow));
  }
}

/**
 * Runs the fireworks animation.
 */
void fireworks() {

  struct FireworkAnimation *ani = &fireworkAnimation;

  if(ani->done) {
    return;
  }

  unsigned long shotDuration = floor((float)ani->duration * (float)ani->shotDuration);
  unsigned long burstDuration = floor((float)ani->duration * (float)ani->burstDuration);
  unsigned long fadeDuration = floor((float)ani->duration * (float)ani->fadeDuration);
  
  ani->currentTime = millis();
  unsigned long duration = ani->currentTime - ani->startTime;
  if(duration >= ani->duration) {

    for(uint16_t i = 0; i< ani->centerStrip->numPixels(); i++) {
      ani->centerStrip->setPixelColor(i, ani->centerStrip->Color(0, 0, 0));
    }

    for(uint16_t i = 0; i< ani->outerStrip->numPixels(); i++) {
      ani->outerStrip->setPixelColor(i, ani->centerStrip->Color(0, 0, 0));
    }
        
    Serial.println("Done with firework animation.");
    ani->done = true;
    return;
  }

  float shotPercentDone = (float)duration / (float)shotDuration;

  int shotLights = 30;

  // Running the shot.
  if(shotPercentDone < 1) {
    int shotPos = floor((float)ani->centerStrip->numPixels() * shotPercentDone);

    // Clear the outer strip.
    for(uint16_t i = 0; i < ani->outerStrip->numPixels(); i++) {
      ani->outerStrip->setPixelColor(i, ani->outerStrip->Color(0, 0, 0));
    }

    for(uint16_t i = 0; i< ani->centerStrip->numPixels(); i++) {
      if(i > shotPos || i < shotPos - shotLights) {
        ani->centerStrip->setPixelColor(i, ani->centerStrip->Color(0, 0, 0));
      }
      else {
        ani->centerStrip->setPixelColor(i, ani->centerStrip->Color(255, 255, 255));
      }
    }
  }
  else {
    // Shot is done, start the burst.
    if(!ani->burstStartTime) {
      Serial.println("Starting firework burst section.");
      ani->burstStartTime = ani->currentTime;
    }
  }

  // Running the burst.
  if(ani->burstStartTime) {

    unsigned long duration = ani->currentTime - ani->burstStartTime;

    float burstPercentDone = (float)duration / (float)burstDuration;

    if(burstPercentDone < 0.5) {
      for(uint16_t i = ani->outerStrip->numPixels() - 180; i < ani->outerStrip->numPixels(); i++) {
        int color = floor((float)255 * burstPercentDone * 2);
        ani->outerStrip->setPixelColor(i, ani->outerStrip->Color(color, color, color));
      }
    }
    else if(burstPercentDone < 1) {
      for(uint16_t i = ani->outerStrip->numPixels() - 180; i < ani->outerStrip->numPixels(); i++) {
        int color = 255 - floor((float)255 * ((burstPercentDone * 2.0) - 1.0));
        ani->outerStrip->setPixelColor(i, ani->outerStrip->Color(color, color, color));
      }
    }
    else {
      if(!ani->fadeStartTime) {
        for(uint16_t i = 0; i< ani->centerStrip->numPixels(); i++) {
          ani->centerStrip->setPixelColor(i, ani->centerStrip->Color(0, 0, 0));
        }
        Serial.println("Starting firework fade section.");
        ani->fadeStartTime = ani->currentTime;
      }
    }
  }

  // Running the fade.
  if(ani->fadeStartTime) {

    unsigned long duration = ani->currentTime - ani->fadeStartTime;

    float fadePercentDone = (float)duration / (float)fadeDuration;

    if(fadePercentDone < 1) {

      int intensitySubtract = floor(255.0 * fadePercentDone);

      // Lights have been lit for enough time, pick new random set.
      if(ani->currentTime - ani->lastFadeUpdate > 75) {
        ani->lastFadeUpdate = ani->currentTime;;
        int endLight = (floor)(ani->outerStrip->numPixels() - ((float)ani->outerStrip->numPixels() * fadePercentDone));

        for(uint16_t i = endLight; i < ani->outerStrip->numPixels(); i++) {

          float percentDown = (float)i / (float)ani->outerStrip->numPixels();

          int c = 255.0 - intensitySubtract;

          long randomNum = random(0, 5);
          if(randomNum == 2) {
            ani->outerStrip->setPixelColor(i, ani->outerStrip->Color(c, c, c));
          }
          else {
            ani->outerStrip->setPixelColor(i, ani->outerStrip->Color(0, 0, 0));
          }
        }
        
      }
    }
  }
}

/**
 * Runs the warp core animation.
 */
void warpCore() {

  struct WarpCoreAnimation *ani = &warpCoreAnimation;

  if(ani->done) {
    return;
  }

  ani->currentTime = millis();

  unsigned long duration = ani->currentTime - ani->startTime;
  if(duration >= ani->duration) {
    ani->done = true;
    return;
  }

  float percentDone = (float)duration / (float)ani->duration;

  // Fade the off color.
  uint8_t rStart = ani->offStartColor >> 16;
  uint8_t gStart = ani->offStartColor >> 8;
  uint8_t bStart = ani->offStartColor;

  uint8_t rStop = ani->offStopColor >> 16;
  uint8_t gStop = ani->offStopColor >> 8;
  uint8_t bStop = ani->offStopColor;

  uint8_t rNow = floor((float)rStart - (((float)rStart - (float)rStop) * percentDone));
  uint8_t gNow = floor((float)gStart - (((float)gStart - (float)gStop) * percentDone));
  uint8_t bNow = floor((float)bStart - (((float)bStart - (float)bStop) * percentDone));

  if(!ani->pulsing) {
    ani->pulsing = true;
    ani->currentPulseStart = ani->currentTime;
    ani->currentPulseSpeed = floor((float)ani->pulseSpeedStart - (((float)ani->pulseSpeedStart - (float)ani->pulseSpeedEnd) * percentDone));
  }

  ani->currentPulseDuration = ani->currentTime - ani->currentPulseStart;
  if(ani->currentPulseDuration >= ani->currentPulseSpeed) {
    ani->pulsing = false;
  }

  float pulsePercentDone = (float)ani->currentPulseDuration / (float)ani->currentPulseSpeed;

  int centerPos = floor((pulsePercentDone * 2) * (double)(ani->centerStrip->numPixels()));
  int outerPos = floor((pulsePercentDone * 2) * (double)(ani->outerStrip->numPixels()));

  if(pulsePercentDone > 0.5) {
    centerPos = ani->centerStrip->numPixels() - (centerPos - ani->centerStrip->numPixels());
    outerPos = ani->outerStrip->numPixels() - (outerPos - ani->outerStrip->numPixels());
  }

  for(uint16_t i=0; i< ani->centerStrip->numPixels(); i++) {
    if(i > centerPos && i <= centerPos + ani->centerLights) {
      ani->centerStrip->setPixelColor(i, ani->onColor);
    }
    else {
      ani->centerStrip->setPixelColor(i, ani->centerStrip->Color(rNow, gNow, bNow));
    }
  }

  for(uint16_t i=0; i< ani->outerStrip->numPixels(); i++) {
    if(ani->outerStrip->numPixels() - i > outerPos && ani->outerStrip->numPixels() - i <= outerPos + ani->outerLights) {
      ani->outerStrip->setPixelColor(i, ani->onColor);
    }
    else {
      ani->outerStrip->setPixelColor(i, ani->outerStrip->Color(rNow, gNow, bNow));
    }
  }
}
