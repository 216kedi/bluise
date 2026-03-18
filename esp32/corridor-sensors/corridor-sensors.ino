/*
 * Elis Evleri - Koridor Sensör Sistemi
 * ESP32 + WS2811 Warm White LED Strip + PIR Sensör
 *
 * LED: DORALED DRL-2811-WW-24
 *   - WS2811 IC, 24V, Warm White (gün ışığı)
 *   - 2835 chip, 120 LED/m, 3 LED per piksel
 *   - 24V harici güç kaynağı ile beslenir
 *
 * Test:    1x PIR (GPIO 27)
 * Üretim:  2-3x PIR + MQ5 + Alev (ileride eklenecek)
 *
 * Bağlantı:
 *   ESP32 GPIO 5  -> LED Strip Data (yeşil kablo)
 *   ESP32 GND     -> LED Strip GND  (beyaz kablo)
 *   24V PSU +     -> LED Strip +24V (kırmızı kablo)
 *   24V PSU GND   -> LED Strip GND  (beyaz kablo)
 *   ESP32 GND     -> 24V PSU GND    (ortak toprak!)
 *   PIR OUT       -> ESP32 GPIO 27
 */

#include <FastLED.h>
#include "config.h"

// ===================== DURUM DEĞİŞKENLERİ =====================

CRGB leds[NUM_PIXELS];

// PIR durumu
bool motionDetected = false;
unsigned long lastMotionTime = 0;

// LED durumu
bool ledsActive = false;
uint8_t currentBrightness = 0;

// Sensör zamanlama
unsigned long lastSensorRead = 0;

// ===================== KURULUM =====================

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println();
  Serial.println("====================================");
  Serial.println("Elis Evleri - Koridor Sensör Sistemi");
  Serial.println("LED: WS2811 24V Warm White");
  Serial.print("Piksel sayisi: ");
  Serial.println(NUM_PIXELS);

  #ifdef MODE_PRODUCTION
    Serial.println("Mod: URETIM");
  #else
    Serial.println("Mod: TEST (1xPIR)");
  #endif

  Serial.println("====================================");

  // PIR sensör pinleri
  pinMode(PIR_1_PIN, INPUT);
  #ifdef MODE_PRODUCTION
    pinMode(PIR_2_PIN, INPUT);
    pinMode(PIR_3_PIN, INPUT);
  #endif

  // WS2811 LED strip başlat
  // WS2811 warm white: RGB kanallarının hepsi aynı LED'i kontrol eder
  // CRGB::White = tam parlaklık warm white
  FastLED.addLeds<WS2811, LED_PIN, RGB>(leds, NUM_PIXELS);
  FastLED.setBrightness(0);

  // Başlangıçta LED'leri kapat
  fill_solid(leds, NUM_PIXELS, CRGB::Black);
  FastLED.show();

  // Başlangıç testi - LED'lerin çalıştığını doğrula
  startupTest();

  // PIR kalibrasyonu
  Serial.println("PIR kalibrasyon (30sn)...");
  delay(30000);
  Serial.println("Sistem hazir!");
}

// ===================== ANA DÖNGÜ =====================

void loop() {
  unsigned long now = millis();

  // Sensör okuma
  if (now - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = now;
    readPIR();
  }

  // LED kontrolü
  updateMotionLEDs(now);
}

// ===================== PIR OKUMA =====================

void readPIR() {
  bool pir1 = digitalRead(PIR_1_PIN) == HIGH;
  bool anyMotion = pir1;

  if (pir1 && !motionDetected) {
    Serial.println("[PIR-1] Hareket algilandi!");
  }

  #ifdef MODE_PRODUCTION
    bool pir2 = digitalRead(PIR_2_PIN) == HIGH;
    bool pir3 = digitalRead(PIR_3_PIN) == HIGH;
    anyMotion = pir1 || pir2 || pir3;

    if (pir2 && !motionDetected) Serial.println("[PIR-2] Hareket!");
    if (pir3 && !motionDetected) Serial.println("[PIR-3] Hareket!");
  #endif

  if (anyMotion) {
    motionDetected = true;
    lastMotionTime = millis();
  }
}

// ===================== LED KONTROL =====================

void updateMotionLEDs(unsigned long now) {
  if (motionDetected) {
    // Hareket var -> LED'leri aç
    if (!ledsActive) {
      fadeIn();
      ledsActive = true;
    }

    // Timeout: hareket yoksa kapat
    if (now - lastMotionTime >= MOTION_TIMEOUT) {
      Serial.println("[LED] Timeout - kapaniyor");
      fadeOut();
      ledsActive = false;
      motionDetected = false;
    }
  }
}

// ===================== LED EFEKTLERİ =====================

// Yumuşak açılma - gün ışığı sıcak beyaz
void fadeIn() {
  Serial.println("[LED] Aciliyor...");
  // Warm white LED: tüm pikselleri beyaza ayarla, parlaklıkla kontrol et
  fill_solid(leds, NUM_PIXELS, CRGB::White);

  for (int b = 0; b <= LED_BRIGHTNESS; b += 3) {
    FastLED.setBrightness(b);
    FastLED.show();
    delay(FADE_STEP_DELAY);
  }
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.show();
  currentBrightness = LED_BRIGHTNESS;
}

// Yumuşak kapanma
void fadeOut() {
  Serial.println("[LED] Kapaniyor...");
  for (int b = LED_BRIGHTNESS; b >= 0; b -= 3) {
    FastLED.setBrightness(b);
    FastLED.show();
    delay(FADE_STEP_DELAY);
  }
  fill_solid(leds, NUM_PIXELS, CRGB::Black);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.show();
  currentBrightness = 0;
}

// Başlangıç testi: sırayla pikselleri yak
void startupTest() {
  Serial.println("LED test basliyor...");
  for (int i = 0; i < NUM_PIXELS; i++) {
    leds[i] = CRGB::White;
    FastLED.setBrightness(LED_BRIGHTNESS / 3);
    FastLED.show();
    delay(30);
    leds[i] = CRGB::Black;
  }
  // Hepsini bir anlığına yak
  fill_solid(leds, NUM_PIXELS, CRGB::White);
  FastLED.setBrightness(LED_BRIGHTNESS / 2);
  FastLED.show();
  delay(500);
  // Kapat
  fill_solid(leds, NUM_PIXELS, CRGB::Black);
  FastLED.setBrightness(0);
  FastLED.show();
  Serial.println("LED test tamam.");
}
