/**
  ******************************************************************************
  * @file           : can_handler.h
  * @brief          : FDCAN Haberleşme Katmanı Başlık Dosyası
  ******************************************************************************
  */

#ifndef INC_CAN_HANDLER_H_
#define INC_CAN_HANDLER_H_

#include "main.h"
#include "anti_pinch.h"

/* CAN Mesaj ID Tanımları */
#define CAN_ID_COMMAND         0x100U  /* BCM -> ECU Komut Mesajı */
#define CAN_ID_STATUS          0x101U  /* ECU -> BCM Durum Bildirim Mesajı */

/* Komut Kodları */
typedef enum {
    CAN_CMD_STOP        = 0x00U,
    CAN_CMD_MANUAL_UP   = 0x01U,
    CAN_CMD_MANUAL_DOWN = 0x02U,
    CAN_CMD_AUTO_UP     = 0x03U,
    CAN_CMD_AUTO_DOWN   = 0x04U
} CAN_Command_t;

/* Hata Kodları */
typedef enum {
    CAN_FAULT_NONE       = 0x00U,
    CAN_FAULT_ANTIPINCH  = 0x01U,
    CAN_FAULT_HARDWARE   = 0x02U
} CAN_FaultCode_t;

/* Fonksiyon Prototipleri */
HAL_StatusTypeDef CANHandler_Init(FDCAN_HandleTypeDef *hfdcan);
HAL_StatusTypeDef CANHandler_SendStatus(FDCAN_HandleTypeDef *hfdcan, AntiPinch_State_t state, uint16_t adc_current, CAN_FaultCode_t fault);
CAN_Command_t CANHandler_GetLatestCommand(void);
uint8_t CANHandler_IsNewCommandAvailable(void);

#endif /* INC_CAN_HANDLER_H_ */
