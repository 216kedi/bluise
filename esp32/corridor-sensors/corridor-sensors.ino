/*
 * Elis Evleri - Koridor Sensör Sistemi
 * ESP32 + WS2811 24V Warm White LED Strip (10m, 400 piksel) + PIR
 *
 * LED: DORALED DRL-2811-WW-24
 *   - WS2811, 24V, Warm White (gün ışığı)
 *   - 120 LED/m, 3 LED per piksel -> 40 piksel/m -> 10m = 400 piksel
 *
 * Davranış:
 *   - PIR hareket algılar -> fade-in ile tüm strip yanar
 *   - 10sn hareket yoksa -> snake efekti ile bir uçtan sırayla söner
 *
 * Bağlantı:
 *   ESP32 GPIO 5  -> LED Data (yeşil)
 *   ESP32 GND     -> LED GND (beyaz) + 24V PSU GND (ortak toprak)
 *   24V PSU +     -> LED +24V (kırmızı)
 *   PIR OUT       -> ESP32 GPIO 27
 */

#include <FastLED.h>
#include "config.h"

// ===================== DURUM DEĞİŞKENLERİ =====================

CRGB leds[NUM_PIXELS];

// PIR
bool motionDetected = false;
unsigned long lastMotionTime = 0;

// LED
bool ledsActive = false;

// Sensör zamanlama
unsigned long lastSensorRead = 0;

// ===================== KURULUM =====================

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println();
  Serial.println("====================================");
  Serial.println("Elis Evleri - Koridor Sensör Sistemi");
  Serial.println("LED: WS2811 24V Warm White - 10m");
  Serial.print("Piksel: ");
  Serial.println(NUM_PIXELS);
  Serial.println("====================================");

  pinMode(PIR_1_PIN, INPUT);
  #ifdef MODE_PRODUCTION
    pinMode(PIR_2_PIN, INPUT);
    pinMode(PIR_3_PIN, INPUT);
  #endif

  // WS2811 başlat
  FastLED.addLeds<WS2811, LED_PIN, RGB>(leds, NUM_PIXELS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  fill_solid(leds, NUM_PIXELS, CRGB::Black);
  FastLED.show();

  // Başlangıç testi - snake koşu
  startupTest();

  // PIR kalibrasyonu
  Serial.println("PIR kalibrasyon (30sn)...");
  delay(30000);
  Serial.println("Sistem hazir! Hareket bekleniyor...");
}

// ===================== ANA DÖNGÜ =====================

void loop() {
  unsigned long now = millis();

  if (now - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = now;
    readPIR();
  }

  // Hareket varsa -> aç
  if (motionDetected && !ledsActive) {
    snakeFadeIn();
    ledsActive = true;
  }

  // LED'ler açıksa ve timeout olduysa -> snake ile kapat
  if (ledsActive && !motionDetected) {
    // motionDetected zaten readPIR'da false yapılıyor timeout sonrası değil,
    // burada timeout kontrolü yapalım
  }

  if (ledsActive && (now - lastMotionTime >= MOTION_TIMEOUT)) {
    // Kapanmadan önce son bir PIR kontrolü
    bool stillMotion = digitalRead(PIR_1_PIN) == HIGH;
    #ifdef MODE_PRODUCTION
      stillMotion = stillMotion || digitalRead(PIR_2_PIN) == HIGH || digitalRead(PIR_3_PIN) == HIGH;
    #endif

    if (stillMotion) {
      // Hala hareket var, timeout'u sıfırla
      lastMotionTime = now;
    } else {
      // Hareket yok, snake ile kapat
      Serial.println("[LED] 10sn doldu - snake kapaniyor...");
      snakeFadeOut();
      ledsActive = false;
      motionDetected = false;
    }
  }
}

// ===================== PIR OKUMA =====================

void readPIR() {
  bool pir1 = digitalRead(PIR_1_PIN) == HIGH;
  bool anyMotion = pir1;

  #ifdef MODE_PRODUCTION
    bool pir2 = digitalRead(PIR_2_PIN) == HIGH;
    bool pir3 = digitalRead(PIR_3_PIN) == HIGH;
    anyMotion = pir1 || pir2 || pir3;
  #endif

  if (anyMotion) {
    if (!motionDetected) {
      Serial.println("[PIR] Hareket algilandi!");
    }
    motionDetected = true;
    lastMotionTime = millis();
  }
}

// ===================== LED EFEKTLERİ =====================

// Snake fade-in: pikseller sırayla bir uçtan diğerine yanar
// Her piksel yanarken arkasındakiler de parlaklaşır -> akıcı görünüm
void snakeFadeIn() {
  Serial.println("[LED] Snake fade-in basliyor...");

  // Tüm pikselleri sırayla yak
  for (int i = 0; i < NUM_PIXELS; i++) {
    leds[i] = CRGB::White;
    FastLED.show();
    delay(FADE_IN_DELAY);
  }

  Serial.println("[LED] Tamamen acik.");
}

// Snake fade-out: pikseller bir uçtan diğerine sırayla söner
// Yılan gibi karanlık dalga ilerler
void snakeFadeOut() {
  Serial.println("[LED] Snake fade-out basliyor...");

  // Her piksel grubu kademeli sönsün (kuyruk efekti)
  // Önce sönen piksellerin etrafında yumuşak geçiş
  int tailLength = 15;  // Kuyruk uzunluğu - yumuşak geçiş için

  for (int head = 0; head < NUM_PIXELS + tailLength; head++) {
    // Kapanma sırasında hareket algılanırsa iptal et
    if (digitalRead(PIR_1_PIN) == HIGH) {
      Serial.println("[LED] Kapanma iptal - hareket var!");
      // Tekrar tam yak
      fill_solid(leds, NUM_PIXELS, CRGB::White);
      FastLED.show();
      motionDetected = true;
      lastMotionTime = millis();
      return;
    }

    // Kuyruk bölgesindeki pikselleri kademeli karart
    for (int t = 0; t < tailLength; t++) {
      int pos = head - t;
      if (pos >= 0 && pos < NUM_PIXELS) {
        // Kuyruğun başı karanlık, sonu parlak
        uint8_t brightness = map(t, 0, tailLength - 1, 0, 255);
        leds[pos] = CRGB(brightness, brightness, brightness);
      }
    }

    // Kuyruktan geride kalanları tamamen kapat
    int offPos = head - tailLength;
    if (offPos >= 0 && offPos < NUM_PIXELS) {
      leds[offPos] = CRGB::Black;
    }

    FastLED.show();
    delay(SNAKE_OUT_DELAY);
  }

  // Hepsini kapat (garanti)
  fill_solid(leds, NUM_PIXELS, CRGB::Black);
  FastLED.show();
  Serial.println("[LED] Tamamen kapali.");
}

// Başlangıç testi
void startupTest() {
  Serial.println("LED test...");
  // Hızlı snake koşusu
  for (int i = 0; i < NUM_PIXELS; i++) {
    leds[i] = CRGB::White;
    if (i > 0) leds[i - 1] = CRGB::Black;
    FastLED.setBrightness(LED_BRIGHTNESS / 3);
    FastLED.show();
    delay(2);
  }
  leds[NUM_PIXELS - 1] = CRGB::Black;

  // Kısa flash
  fill_solid(leds, NUM_PIXELS, CRGB::White);
  FastLED.setBrightness(LED_BRIGHTNESS / 2);
  FastLED.show();
  delay(300);

  fill_solid(leds, NUM_PIXELS, CRGB::Black);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.show();
  Serial.println("LED test tamam.");
}
