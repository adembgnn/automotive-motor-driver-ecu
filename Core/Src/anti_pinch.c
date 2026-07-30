/**
  ******************************************************************************
  * @file           : anti_pinch.c
  * @brief          : Cam Sıkışma Önleme (Anti-Pinch) Algoritması Kaynak Dosyası
  ******************************************************************************
  */

#include "anti_pinch.h"

/* Modül İçi Özel Değişkenler */
static AntiPinch_State_t current_state = ANTIPINCH_STATE_IDLE;
static uint32_t state_timer_ms = 0U;
static uint32_t deglitch_counter_ms = 0U;

/**
  * @brief  Anti-Pinch modülünü varsayılan duruma getirir.
  */
void AntiPinch_Init(void)
{
    current_state = ANTIPINCH_STATE_IDLE;
    state_timer_ms = 0U;
    deglitch_counter_ms = 0U;
}

/**
  * @brief  Motoru güvenli şekilde çalıştırır ve kalkış sürecini başlatan durum makinesine sokar.
  * @param  htim: PWM konfigürasyonuna sahip Timer yapısı adresi
  * @param  direction_speed: Hız ve yön (-100 ile +100 arası)
  */
void AntiPinch_StartMotor(TIM_HandleTypeDef *htim, int8_t direction_speed)
{
    if ((current_state == ANTIPINCH_STATE_IDLE) && (direction_speed != 0))
    {
        /* Motor hızını ve yönünü DRV8703 sürücüsüne gönder */
        DRV8703_SetMotorSpeed(htim, (int16_t)direction_speed);

        /* Kalkış anındaki deşarj/inrush akımını süzen körleme devresini başlat */
        state_timer_ms = HAL_GetTick();
        deglitch_counter_ms = 0U;
        current_state = ANTIPINCH_STATE_INRUSH_BLANKING;
    }
}

/**
  * @brief  Motoru durdurur ve sistemi bekleme (IDLE) durumuna getirir.
  * @param  htim: Timer yapısı adresi
  */
void AntiPinch_StopMotor(TIM_HandleTypeDef *htim)
{
    DRV8703_SetMotorSpeed(htim, 0);
    current_state = ANTIPINCH_STATE_IDLE;
    deglitch_counter_ms = 0U;
}

/**
  * @brief  DRV8703-Q1 nFAULT pininden gelen kesmede (40A Aşırı Akım) çağrılır.
  * @param  htim: Timer yapısı adresi
  */
void AntiPinch_HandleHardwareFault(TIM_HandleTypeDef *htim)
{
    DRV8703_SetMotorSpeed(htim, 0);
    current_state = ANTIPINCH_STATE_FAULT;
}

/**
  * @brief  Her 1 ms'de bir ana döngüde çağrılan Anti-Pinch durum makinesi.
  * @param  htim: Timer yapısı adresi
  * @param  adc_measured_val: STM32 ADC1'den okunan 12-bitlik ham akım değeri
  */
void AntiPinch_Process(TIM_HandleTypeDef *htim, uint16_t adc_measured_val)
{
    uint32_t current_time = HAL_GetTick();

    switch (current_state)
    {
        case ANTIPINCH_STATE_IDLE:
            /* Motor kapalı, işlem yapılmaz */
            break;

        case ANTIPINCH_STATE_INRUSH_BLANKING:
            /* Motor duruştan harekete geçerken indüktif etki nedeniyle akım sıçrar.
               40 ms boyunca akım okuması göz ardı edilir (Inrush Blanking). */
            if ((current_time - state_timer_ms) >= ANTIPINCH_INRUSH_TIME_MS)
            {
                current_state = ANTIPINCH_STATE_RUNNING;
                deglitch_counter_ms = 0U;
            }
            break;

        case ANTIPINCH_STATE_RUNNING:
            /* Sıkışma takibi aktif bölge */
            if (adc_measured_val >= ANTIPINCH_ADC_THRESHOLD)
            {
                deglitch_counter_ms++;

                /* Yanlış tetiklenmeyi önlemek için süzme (Deglitch): 
                   Akım eşiği kesintisiz 10 ms boyunca aşılmış olmalıdır. */
                if (deglitch_counter_ms >= ANTIPINCH_DEGLITCH_TIME_MS)
                {
                    /* SIKIŞMA ALGILANDI!
                       Motor durdurulur ve 1.5 saniye boyunca geriye sürülür. */
                    DRV8703_SetMotorSpeed(htim, 0);

                    state_timer_ms = HAL_GetTick();
                    current_state = ANTIPINCH_STATE_REVERSING;
                }
            }
            else
            {
                /* Akım eşiğin altına düşerse sayacı sıfırla */
                deglitch_counter_ms = 0U;
            }
            break;

        case ANTIPINCH_STATE_REVERSING:
            /* Sıkışan cismi serbest bırakmak için cam %80 hızla aşağı indirilir */
            DRV8703_SetMotorSpeed(htim, ANTIPINCH_REVERSE_SPEED);

            if ((current_time - state_timer_ms) >= ANTIPINCH_REVERSE_TIME_MS)
            {
                /* Geri dönme süresi tamamlandı, motor durdurulur */
                AntiPinch_StopMotor(htim);
            }
            break;

        case ANTIPINCH_STATE_FAULT:
            /* Donanımsal 40A kısa devre durumu. Güvenlik nedeniyle motor kilitlenir. */
            DRV8703_SetMotorSpeed(htim, 0);
            break;

        default:
            AntiPinch_StopMotor(htim);
            break;
    }
}

/**
  * @brief  Mevcut durum makinesi durumunu döndürür.
  * @retval AntiPinch_State_t
  */
AntiPinch_State_t AntiPinch_GetState(void)
{
    return current_state;
}
