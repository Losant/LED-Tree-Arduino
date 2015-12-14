#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include <math.h>

#define CENTER_PIN 8
#define OUTER_PIN 9

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel centerStrip = Adafruit_NeoPixel(240, CENTER_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel outerStrip = Adafruit_NeoPixel(960, OUTER_PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

struct WarpCoreAnimation {
  unsigned long startTime;
  unsigned long currentTime;
  unsigned long length;       // animation length
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
  unsigned long length;
  unsigned long shotDuration;
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

WarpCoreAnimation warpCoreAnimation { 
  0,
  0,
  10000,
  8,
  80,
  centerStrip.Color(0, 0, 40),
  centerStrip.Color(40, 0, 0),
  centerStrip.Color(0, 255, 255),
  4000,
  4000,
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
  3000,
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

  //Serial.begin(115200);
  //Serial.println("--- Started Tree ---");

  centerStrip.begin();
  outerStrip.begin();
  centerStrip.show(); // Initialize all pixels to 'off'
  outerStrip.show();

  //startFade(&fadeAnimation, outerStrip.Color(0, 40, 0));
  startWarpCore(&warpCoreAnimation);
  //startFireworks(&fireworkAnimation);
}


unsigned long previousMillis = 0;
const long interval = 20;

bool runningWarpCore = true;
bool runningFadeBlue = false;
bool runningFadeGreen = false;
bool runningFadeRed = false;
bool runningFadeDarkBlue = false;

void loop() {

  if(!runningWarpCore && !runningFadeBlue && !runningFadeGreen && !runningFadeRed && !runningFadeDarkBlue) {
    runningWarpCore = true;
    startWarpCore(&warpCoreAnimation);
  }

  if(runningWarpCore && warpCoreAnimation.done) {
    runningWarpCore = false;
    runningFadeBlue = true;
    startFade(&fadeAnimation, centerStrip.Color(0, 0, 255), centerStrip.Color(40, 0, 0));
  }

  if(runningFadeBlue && fadeAnimation.done) {
    runningFadeBlue = false;
    runningFadeGreen = true;
    startFade(&fadeAnimation, centerStrip.Color(0, 255, 0), centerStrip.Color(0, 0, 255));
  }

  if(runningFadeGreen && fadeAnimation.done) {
    runningFadeGreen = false;
    runningFadeRed = true;
    startFade(&fadeAnimation, centerStrip.Color(255, 0, 0), centerStrip.Color(0, 255, 0));
  }

  if(runningFadeRed && fadeAnimation.done) {
    runningFadeRed = false;
    runningFadeDarkBlue = true;
    startFade(&fadeAnimation, centerStrip.Color(0, 0, 40), centerStrip.Color(255, 0, 0));
  }

  if(runningFadeDarkBlue && fadeAnimation.done) {
    runningFadeDarkBlue = false;
  }

  warpCore(&warpCoreAnimation);
  fireworks(&fireworkAnimation);
  fade(&fadeAnimation);

  // Only render the lights if enough time has passed.
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval) {
    centerStrip.show();
    outerStrip.show();
    previousMillis = currentMillis;
  }
}

/**
 * 
 * 
 */
void startWarpCore(struct WarpCoreAnimation *ani) {
  ani->done = false;
  ani->startTime = millis();
  ani->pulsing = false;
  ani->currentPulseSpeed = 0;
}

/**
 * 
 * 
 */
void startFireworks(struct FireworkAnimation *ani) {
  ani->done = false;
  ani->startTime = millis();
}

/**
 * 
 * 
 */
void startFade(struct FadeAnimation *ani, uint32_t toColor, uint32_t fromColor) {
  ani->toColor = toColor;
  ani->fromColor = fromColor;
  ani->done = false;
  ani->startTime = millis();
}

/**
 * 
 * 
 */
void fade(struct FadeAnimation *ani) {
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
 * 
 * 
 * 
 */
void fireworks(struct FireworkAnimation *ani) {

  if(ani->done) {
    return;
  }
  
  ani->currentTime = millis();
  unsigned long duration = ani->currentTime - ani->startTime;
  if(duration >= ani->length) {
    ani->done = true;
    return;
  }

  float shotPercentDone = (float)duration / (float)ani->shotDuration;

  int shotPos = floor((float)ani->centerStrip->numPixels() * shotPercentDone);

  for(uint16_t i=0; i< ani->centerStrip->numPixels(); i++) {
    if(i > shotPos) {
      ani->centerStrip->setPixelColor(i, ani->centerStrip->Color(0, 0, 0));
    }
    else {
      ani->centerStrip->setPixelColor(i, ani->centerStrip->Color(255, 255, 255));
    }
  }
  
}

/**
 * 
 * 
 * 
 */
void warpCore(struct WarpCoreAnimation *ani) {

  if(ani->done) {
    return;
  }

  ani->currentTime = millis();

  unsigned long duration = ani->currentTime - ani->startTime;
  if(duration >= ani->length) {
    ani->done = true;
    return;
  }

  float percentDone = (float)duration / (float)ani->length;

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

/*
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
*/
