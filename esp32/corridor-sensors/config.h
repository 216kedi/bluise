/*
 * Elis Evleri - Koridor Sensör Sistemi
 * Yapılandırma Dosyası
 *
 * LED: DORALED DRL-2811-WW-24 (WS2811, 24V, Warm White, 120LED/m)
 * WS2811 @ 24V -> 3 LED per piksel -> 40 adreslenebilir piksel/m
 *
 * TEST veya PRODUCTION modunu burada seçin.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ===================== MOD SEÇİMİ =====================
// Sadece birini aktif bırakın:
#define MODE_TEST          // Test modu: 1x PIR
// #define MODE_PRODUCTION // Üretim: 2-3x PIR + MQ5 + Alev

// ===================== LED AYARLARI =====================
// DORALED DRL-2811-WW-24: WS2811, 24V, Warm White, 2835 chip
// 120 LED/m fiziksel, WS2811 3'lü gruplar -> 40 piksel/m
// 24V harici güç kaynağı gerekli! ESP32 sadece data sinyali gönderir.
// NOT: ESP32 3.3V çıkış verir, WS2811 5V sinyal bekler.
//      Kısa mesafede 3.3V ile çalışabilir, sorun olursa
//      logic level shifter (3.3V -> 5V) kullanın.

#define LED_PIN             5
#define NUM_PIXELS          40      // Koridor uzunluğuna göre ayarla (40 piksel = 1m)
#define LED_BRIGHTNESS      255     // 0-255 (warm white tek renk, tam parlaklık)

// ===================== PIR AYARLARI =====================
#define PIR_1_PIN           27

#ifdef MODE_PRODUCTION
  #define PIR_2_PIN         26
  #define PIR_3_PIN         25
  #define PIR_COUNT         3
#else
  #define PIR_COUNT         1
#endif

// ===================== GAZ & ALEV (İLERİDE EKLENECEK) =====================
#ifdef MODE_PRODUCTION
  #define MQ5_PIN           34    // Analog - LPG/doğalgaz
  #define FLAME_PIN         35    // Analog - alev/IR

  #define MQ5_THRESHOLD     400   // Gaz alarm eşiği (0-4095)
  #define FLAME_THRESHOLD   500   // Alev alarm eşiği (0-4095)
#endif

// ===================== ZAMANLAMALAR =====================
#define MOTION_TIMEOUT      15000  // Hareket sonrası LED açık kalma (ms)
#define FADE_STEP_DELAY     8      // Fade animasyon adım gecikmesi (ms)
#define SENSOR_READ_INTERVAL 100   // Sensör okuma aralığı (ms)

// ===================== SERİ PORT =====================
#define SERIAL_BAUD         115200

#endif // CONFIG_H
