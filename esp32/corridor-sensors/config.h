/*
 * Elis Evleri - Koridor Sensör Sistemi
 * Yapılandırma Dosyası
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
#define LED_PIN             5
#define NUM_LEDS            30
#define LED_TYPE            WS2812B
#define COLOR_ORDER         GRB
#define LED_BRIGHTNESS      150

// Koridor aydınlatma rengi (sıcak beyaz)
#define LED_COLOR_R         255
#define LED_COLOR_G         200
#define LED_COLOR_B         120

// Tehlike (gaz/alev) uyarı rengi (kırmızı yanıp sönen)
#define ALARM_COLOR_R       255
#define ALARM_COLOR_G       0
#define ALARM_COLOR_B       0

// ===================== PIR AYARLARI =====================
#define PIR_1_PIN           27

#ifdef MODE_PRODUCTION
  #define PIR_2_PIN         26
  #define PIR_3_PIN         25
  #define PIR_COUNT         3
#else
  #define PIR_COUNT         1
#endif

// ===================== GAZ & ALEV (ÜRETIM) =====================
#ifdef MODE_PRODUCTION
  #define MQ5_PIN           34    // Analog - LPG/doğalgaz
  #define FLAME_PIN         35    // Analog - alev/IR

  #define MQ5_THRESHOLD     400   // Gaz alarm eşiği (0-4095)
  #define FLAME_THRESHOLD   500   // Alev alarm eşiği (0-4095)
#endif

// ===================== ZAMANLAMALAR =====================
#define MOTION_TIMEOUT      15000  // Hareket sonrası LED açık kalma (ms)
#define FADE_STEP_DELAY     10     // Fade animasyon adım gecikmesi (ms)
#define SENSOR_READ_INTERVAL 100   // Sensör okuma aralığı (ms)
#define ALARM_BLINK_INTERVAL 300   // Alarm yanıp sönme aralığı (ms)

// ===================== SERİ PORT =====================
#define SERIAL_BAUD         115200

#endif // CONFIG_H
