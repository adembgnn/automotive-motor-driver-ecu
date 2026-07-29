/**
  ******************************************************************************
  * @file           : drv8703.h
  * @brief          : DRV8703-Q1 H-Bridge Gate Driver Sürücü Başlık Dosyası
  ******************************************************************************
  */

#ifndef INC_DRV8703_H_
#define INC_DRV8703_H_

#include "main.h"

/* DRV8703-Q1 SPI Kayıtçı Adresleri (4-Bit ADDR) */
#define DRV8703_REG_FAULT_STAT   0x00U  /* Salt Okunur: Arıza Durumu */
#define DRV8703_REG_VGS_STAT     0x01U  /* Salt Okunur: Gate Sürücü Arızası */
#define DRV8703_REG_IC1_CTRL     0x02U  /* Okuma/Yazma: Sürücü Ayarları & CSA Kazancı */
#define DRV8703_REG_OCP_CTRL     0x03U  /* Okuma/Yazma: Donanımsal Aşırı Akım Ayarları */

/* Donanım Sabitleri */
#define PWM_TIMER_ARR_MAX        8499U  /* 20 kHz PWM için Timer ARR Tavan Değeri */
#define DRV8703_SPI_TIMEOUT_MS   100U   /* SPI İletişim Zaman Aşımı Süresi (ms) */

/* Fonksiyon Prototipleri */
HAL_StatusTypeDef DRV8703_Init(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef DRV8703_WriteRegister(SPI_HandleTypeDef *hspi, uint8_t reg_addr, uint16_t data);
uint16_t DRV8703_ReadRegister(SPI_HandleTypeDef *hspi, uint8_t reg_addr);
void DRV8703_SetMotorSpeed(TIM_HandleTypeDef *htim, int16_t speed_percentage);

#endif /* INC_DRV8703_H_ */
