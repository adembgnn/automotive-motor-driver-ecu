/**
  ******************************************************************************
  * @file           : anti_pinch.h
  * @brief          : Cam Sıkışma Önleme (Anti-Pinch) Algoritması Başlık Dosyası
  ******************************************************************************
  */

#ifndef INC_ANTI_PINCH_H_
#define INC_ANTI_PINCH_H_

#include "main.h"
#include "drv8703.h"

/* Algoritma Zamanlama ve Akım Eşik Sabitleri */
#define ANTIPINCH_ADC_THRESHOLD      1389U  /* 28A Akım Eşiği (12-bit ADC Karşılığı) */
#define ANTIPINCH_INRUSH_TIME_MS     40U    /* Kalkış Akımını Görmezden Gelme Süresi (Blanking) */
#define ANTIPINCH_DEGLITCH_TIME_MS   10U    /* Yanlış Tetiklemeyi Önleme Süzme Süresi */
#define ANTIPINCH_REVERSE_TIME_MS    1500U  /* Engelden Kaçış İçin Geri Çalışma Süresi (ms) */
#define ANTIPINCH_REVERSE_SPEED      -80    /* Geri Dönüş Motor Hızı Yüzdesi (%-80) */

/**
  * @brief Sistem Durum Makinesi (State Machine) Numaralandırması
  */
typedef enum {
    ANTIPINCH_STATE_IDLE = 0U,       /* Motor Duruyor */
    ANTIPINCH_STATE_INRUSH_BLANKING, /* Motor Kalkış Anı (Akım Ölçümü Körleme) */
    ANTIPINCH_STATE_RUNNING,         /* Normal Çalışma (Sıkışma Koruması Aktif) */
    ANTIPINCH_STATE_REVERSING,       /* Sıkışma Tespiti Sonrası Geriye Dönüş */
    ANTIPINCH_STATE_FAULT            /* Donanımsal Arıza Modu (40A Aşırı Akım) */
} AntiPinch_State_t;

/* Fonksiyon Prototipleri */
void AntiPinch_Init(void);
void AntiPinch_StartMotor(TIM_HandleTypeDef *htim, int8_t direction_speed);
void AntiPinch_StopMotor(TIM_HandleTypeDef *htim);
void AntiPinch_Process(TIM_HandleTypeDef *htim, uint16_t adc_measured_val);
void AntiPinch_HandleHardwareFault(TIM_HandleTypeDef *htim);
AntiPinch_State_t AntiPinch_GetState(void);

#endif /* INC_ANTI_PINCH_H_ */
