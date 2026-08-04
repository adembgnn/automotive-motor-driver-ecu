/**
  ******************************************************************************
  * @file           : can_handler.c
  * @brief          : FDCAN Haberleşme Katmanı Kaynak Dosyası
  ********************************----------------------------------------------
  */

#include "can_handler.h"

/* Modül İçi Değişkenler */
static volatile CAN_Command_t latest_command = CAN_CMD_STOP;
static volatile uint8_t new_command_flag = 0U;

/**
  * @brief  FDCAN Donanımını ve Filtrelerini Yapılandırır.
  * @param  hfdcan: FDCAN Donanım Bekleme Yapısı Adresi
  * @retval HAL Durum Kodu
  */
HAL_StatusTypeDef CANHandler_Init(FDCAN_HandleTypeDef *hfdcan)
{
    FDCAN_FilterTypeDef sFilterConfig;
    HAL_StatusTypeDef status;

    /* Donanımsal CAN Filtre Yapılandırması:
       Sadece ID = 0x100 olan komut mesajlarını RX FIFO0'a kabul et, gerisini donanımda reddet. */
    sFilterConfig.IdType       = FDCAN_STANDARD_ID;
    sFilterConfig.FilterIndex  = 0;
    sFilterConfig.FilterType   = FDCAN_FILTER_EXACT;
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    sFilterConfig.FilterID1    = CAN_ID_COMMAND;
    sFilterConfig.FilterID2    = 0x000U;

    status = HAL_FDCAN_ConfigFilter(hfdcan, &sFilterConfig);
    if (status != HAL_OK)
    {
        return status;
    }

    /* Küresel Reddetme Politikasını Ayarla (Tanımlanmayan ID'ler doğrudan elenir) */
    status = HAL_FDCAN_ConfigGlobalFilter(hfdcan, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
    if (status != HAL_OK)
    {
        return status;
    }

    /* RX FIFO0 Yeni Mesaj Geldi Kesmesini (Interrupt) Aktifleştir */
    status = HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    if (status != HAL_OK)
    {
        return status;
    }

    /* FDCAN Modülünü Başlat */
    status = HAL_FDCAN_Start(hfdcan);

    return status;
}

/**
  * @brief  Sistem Durumunu CAN Bus Üzerinden BCM Modülüne Gönderir (ID: 0x101)
  * @param  hfdcan: FDCAN Donanım Bekleme Yapısı Adresi
  * @param  state: Mevcut Anti-Pinch Durumu
  * @param  adc_current: Okunan 12-bit ADC Akım Değeri
  * @param  fault: Hata Kodu
  * @retval HAL Durum Kodu
  */
HAL_StatusTypeDef CANHandler_SendStatus(FDCAN_HandleTypeDef *hfdcan, AntiPinch_State_t state, uint16_t adc_current, CAN_FaultCode_t fault)
{
    FDCAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[4];

    /* CAN Paketi Başlık Yapılandırması */
    TxHeader.Identifier          = CAN_ID_STATUS;
    TxHeader.IdType              = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType         = FDCAN_DATA_FRAME;
    TxHeader.DataLength          = FDCAN_DLC_BYTES_4;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch       = FDCAN_BRS_OFF;
    TxHeader.FDFormat            = FDCAN_CLASSIC_CAN;
    TxHeader.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker       = 0;

    /* Veri Alanı Hazırlığı (Big-Endian Düzeneği) */
    TxData[0] = (uint8_t)state;
    TxData[1] = (uint8_t)((adc_current >> 8) & 0xFFU); /* ADC Yüksek Bayt */
    TxData[2] = (uint8_t)(adc_current & 0xFFU);        /* ADC Düşük Bayt  */
    TxData[3] = (uint8_t)fault;

    /* Mesajı TX FIFO Kuyruğuna Ekle */
    return HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &TxHeader, TxData);
}

/**
  * @brief  STM32 HAL FDCAN Alım Kesme Çağrısı (Interrupt Callback)
  * @param  hfdcan: FDCAN Donanım Bekleme Yapısı Adresi
  * @param  RpcFlags: Kesme Bayrakları
  */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RpcFlags)
{
    FDCAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[1];

    if ((RpcFlags & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0U)
    {
        /* FIFO0'dan Gelen Mesajı Çek */
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK)
        {
            /* Sadece Gelen ID Komut ID'si ise İşle */
            if ((RxHeader.Identifier == CAN_ID_COMMAND) && (RxHeader.IdType == FDCAN_STANDARD_ID))
            {
                latest_command = (CAN_Command_t)RxData[0];
                new_command_flag = 1U;
            }
        }
    }
}

/**
  * @brief  Son Alınan Komutu Döndürür ve Yeni Komut Bayrağını Temizler.
  * @retval CAN_Command_t
  */
CAN_Command_t CANHandler_GetLatestCommand(void)
{
    new_command_flag = 0U;
    return latest_command;
}

/**
  * @brief  İşlenmemiş Yeni Bir CAN Komutu Olup Olmadığını Bildirir.
  * @retval 1: Yeni Komut Var, 0: Yok
  */
uint8_t CANHandler_IsNewCommandAvailable(void)
{
    return new_command_flag;
}
