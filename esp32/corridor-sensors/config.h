/*
 * Elis Evleri - Koridor Sensör Sistemi
 * Yapılandırma Dosyası
 *
 * LED: DORALED DRL-2811-WW-24 (WS2811, 24V, Warm White, 120LED/m)
 * WS2811 @ 24V -> 3 LED per piksel -> 40 adreslenebilir piksel/m
 * 10m rulo = 400 piksel
 */

#ifndef CONFIG_H
#define CONFIG_H

// ===================== MOD SEÇİMİ =====================
#define MODE_TEST          // Test modu: 1x PIR
// #define MODE_PRODUCTION // Üretim: 2-3x PIR + MQ5 + Alev

// ===================== LED AYARLARI =====================
// DORALED DRL-2811-WW-24: WS2811, 24V, Warm White
// 120 LED/m fiziksel, 3 LED per IC -> 40 piksel/m
// 10m rulo = 400 adreslenebilir piksel
// 24V harici güç kaynağı gerekli!

#define LED_PIN             5
#define NUM_PIXELS          400     // 10m x 40 piksel/m = 400 piksel
#define LED_BRIGHTNESS      255     // Tam parlaklık

// ===================== PIR AYARLARI =====================
#define PIR_1_PIN           27

#ifdef MODE_PRODUCTION
  #define PIR_2_PIN         26
  #define PIR_3_PIN         25
  #define PIR_COUNT         3
#else
  #define PIR_COUNT         1
#endif

// ===================== GAZ & ALEV (İLERİDE) =====================
#ifdef MODE_PRODUCTION
  #define MQ5_PIN           34
  #define FLAME_PIN         35
  #define MQ5_THRESHOLD     400
  #define FLAME_THRESHOLD   500
#endif

// ===================== ZAMANLAMALAR =====================
#define MOTION_TIMEOUT      10000  // 10 saniye sonra kapanmaya başla
#define FADE_IN_DELAY       2      // Fade-in adım gecikmesi (ms) - hızlı açılsın
#define SNAKE_OUT_DELAY     8      // Snake kapanma piksel gecikmesi (ms)
#define SENSOR_READ_INTERVAL 100

// ===================== SERİ PORT =====================
#define SERIAL_BAUD         115200

#endif // CONFIG_H
