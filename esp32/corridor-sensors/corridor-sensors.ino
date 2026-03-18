/*
 * Elis Evleri - Koridor Sensör Sistemi
 * ESP32 + Adreslenebilir LED (WS2812B) + PIR + MQ5 + Alev Sensörü
 *
 * Test Modu:  1x PIR (GPIO 27)
 * Üretim:     3x PIR (27,26,25) + MQ5 (34) + Alev (35)
 *
 * Mod seçimi config.h dosyasından yapılır.
 */

#include <FastLED.h>
#include "config.h"

// ===================== DURUM DEĞİŞKENLERİ =====================

CRGB leds[NUM_LEDS];

// PIR durumu
bool motionDetected = false;
unsigned long lastMotionTime = 0;

// LED durumu
bool ledsActive = false;

// Alarm durumu
bool alarmActive = false;
bool alarmLedState = false;
unsigned long lastAlarmBlink = 0;

// Sensör zamanlama
unsigned long lastSensorRead = 0;

// ===================== KURULUM =====================

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println();
  Serial.println("=================================");
  Serial.println("Elis Evleri - Koridor Sensör Sistemi");

  #ifdef MODE_PRODUCTION
    Serial.println("Mod: URETIM (3xPIR + MQ5 + Alev)");
  #else
    Serial.println("Mod: TEST (1xPIR)");
  #endif

  Serial.println("=================================");

  // PIR sensör pinleri
  pinMode(PIR_1_PIN, INPUT);
  #ifdef MODE_PRODUCTION
    pinMode(PIR_2_PIN, INPUT);
    pinMode(PIR_3_PIN, INPUT);
  #endif

  // LED strip başlat
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(LED_BRIGHTNESS);

  // Başlangıçta LED'leri kapat
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // PIR kalibrasyonu
  Serial.println("PIR kalibrasyon (30sn)...");
  // Kalibrasyon sırasında kısa bir LED test animasyonu
  startupAnimation();
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

    #ifdef MODE_PRODUCTION
      readGasSensor();
      readFlameSensor();
    #endif
  }

  // Alarm varsa alarm LED efekti, yoksa normal hareket kontrolü
  if (alarmActive) {
    updateAlarmLEDs(now);
  } else {
    updateMotionLEDs(now);
  }
}

// ===================== PIR OKUMA =====================

void readPIR() {
  bool pir1 = digitalRead(PIR_1_PIN) == HIGH;
  bool anyMotion = pir1;

  if (pir1) Serial.println("[PIR-1] Hareket!");

  #ifdef MODE_PRODUCTION
    bool pir2 = digitalRead(PIR_2_PIN) == HIGH;
    bool pir3 = digitalRead(PIR_3_PIN) == HIGH;
    anyMotion = pir1 || pir2 || pir3;

    if (pir2) Serial.println("[PIR-2] Hareket!");
    if (pir3) Serial.println("[PIR-3] Hareket!");
  #endif

  if (anyMotion) {
    motionDetected = true;
    lastMotionTime = millis();
  }
}

// ===================== GAZ SENSÖRÜ (ÜRETIM) =====================

#ifdef MODE_PRODUCTION
void readGasSensor() {
  int gasValue = analogRead(MQ5_PIN);

  if (gasValue > MQ5_THRESHOLD) {
    if (!alarmActive) {
      Serial.print("[MQ5] GAZ ALARMI! Deger: ");
      Serial.println(gasValue);
    }
    alarmActive = true;
  }
}
#endif

// ===================== ALEV SENSÖRÜ (ÜRETIM) =====================

#ifdef MODE_PRODUCTION
void readFlameSensor() {
  int flameValue = analogRead(FLAME_PIN);

  // Alev sensörü genelde düşük değer = alev var
  if (flameValue < FLAME_THRESHOLD) {
    if (!alarmActive) {
      Serial.print("[ALEV] YANGIN ALARMI! Deger: ");
      Serial.println(flameValue);
    }
    alarmActive = true;
  }
}
#endif

// ===================== HAREKET LED KONTROL =====================

void updateMotionLEDs(unsigned long now) {
  if (motionDetected) {
    if (!ledsActive) {
      fadeIn();
      ledsActive = true;
    }

    // Timeout kontrolü
    if (now - lastMotionTime >= MOTION_TIMEOUT) {
      Serial.println("[LED] Zaman asimi - kapaniyor");
      fadeOut();
      ledsActive = false;
      motionDetected = false;
    }
  }
}

// ===================== ALARM LED KONTROL =====================

void updateAlarmLEDs(unsigned long now) {
  // Kırmızı yanıp sönen alarm efekti
  if (now - lastAlarmBlink >= ALARM_BLINK_INTERVAL) {
    lastAlarmBlink = now;
    alarmLedState = !alarmLedState;

    if (alarmLedState) {
      fill_solid(leds, NUM_LEDS, CRGB(ALARM_COLOR_R, ALARM_COLOR_G, ALARM_COLOR_B));
    } else {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
    }
    FastLED.setBrightness(LED_BRIGHTNESS);
    FastLED.show();
  }

  // Alarm sırasında da sensörleri kontrol et
  // Gaz/alev değeri normale dönerse alarmı kapat
  #ifdef MODE_PRODUCTION
    int gasVal = analogRead(MQ5_PIN);
    int flameVal = analogRead(FLAME_PIN);

    if (gasVal <= MQ5_THRESHOLD && flameVal >= FLAME_THRESHOLD) {
      Serial.println("[ALARM] Tehlike gecti - normal moda donus");
      alarmActive = false;
      alarmLedState = false;
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      ledsActive = false;
    }
  #endif
}

// ===================== LED EFEKTLERİ =====================

void fadeIn() {
  Serial.println("[LED] Aciliyor...");
  CRGB color = CRGB(LED_COLOR_R, LED_COLOR_G, LED_COLOR_B);

  for (int b = 0; b <= LED_BRIGHTNESS; b += 5) {
    fill_solid(leds, NUM_LEDS, color);
    FastLED.setBrightness(b);
    FastLED.show();
    delay(FADE_STEP_DELAY);
  }
}

void fadeOut() {
  Serial.println("[LED] Kapaniyor...");
  for (int b = LED_BRIGHTNESS; b >= 0; b -= 5) {
    FastLED.setBrightness(b);
    FastLED.show();
    delay(FADE_STEP_DELAY);
  }
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.show();
}

// Başlangıç animasyonu (kalibrasyon sırasında)
void startupAnimation() {
  // Tek tek LED yak - koşu efekti
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(LED_COLOR_R, LED_COLOR_G, LED_COLOR_B);
    FastLED.setBrightness(LED_BRIGHTNESS / 2);
    FastLED.show();
    delay(50);
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}
