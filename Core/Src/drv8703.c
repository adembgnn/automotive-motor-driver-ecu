/**
  ******************************************************************************
  * @file           : drv8703.c
  * @brief          : DRV8703-Q1 H-Bridge Gate Driver Sürücü Kaynak Dosyası
  ******************************************************************************
  */

#include "drv8703.h"

/**
  * @brief  DRV8703-Q1 içindeki bir kayıtçıya 16-bit SPI paketi yazar.
  * @param  hspi: SPI donanım bekleme yapısı adresi
  * @param  reg_addr: Hedef kayıtçı adresi (0x00 - 0x03)
  * @param  data: Yazılacak 11-bitlik veri
  * @retval HAL Durum Kodu (HAL_OK, HAL_ERROR, vb.)
  */
HAL_StatusTypeDef DRV8703_WriteRegister(SPI_HandleTypeDef *hspi, uint8_t reg_addr, uint16_t data)
{
    uint16_t frame = 0;
    uint8_t tx_buf[2];

    /* SPI Paket Formatı: [Bit 15 = 0 (Write)] | [Bits 14:11 = Reg Addr] | [Bits 10:0 = Data] */
    frame = ((uint16_t)(reg_addr & 0x0FU) << 11) | (data & 0x07FFU);

    /* Big-Endian (MSB First) Dönüşümü */
    tx_buf[0] = (uint8_t)(frame >> 8);   /* Yüksek Bayt (MSB) */
    tx_buf[1] = (uint8_t)(frame & 0xFFU); /* Düşük Bayt (LSB)  */

    /* Chip Select (nCS) Aktif (LOW) */
    HAL_GPIO_WritePin(DRV8703_NCS_GPIO_Port, DRV8703_NCS_Pin, GPIO_PIN_RESET);

    /* SPI Veri İletimi */
    HAL_StatusTypeDef status = HAL_SPI_Transmit(hspi, tx_buf, 2, DRV8703_SPI_TIMEOUT_MS);

    /* Chip Select (nCS) Pasif (HIGH) */
    HAL_GPIO_WritePin(DRV8703_NCS_GPIO_Port, DRV8703_NCS_Pin, GPIO_PIN_SET);

    return status;
}

/**
  * @brief  DRV8703-Q1 entegresini ilklendirir ve konfigüre eder.
  * @param  hspi: SPI donanım bekleme yapısı adresi
  * @retval HAL Durum Kodu
  */
HAL_StatusTypeDef DRV8703_Init(SPI_HandleTypeDef *hspi)
{
    HAL_StatusTypeDef status;

    /* 1. Donanımsal Reset: nRESET Pinini HIGH seviyesine çekerek entegreyi uyandır */
    HAL_GPIO_WritePin(DRV8703_NRESET_GPIO_Port, DRV8703_NRESET_Pin, GPIO_PIN_SET);
    HAL_Delay(2); /* Entegre dâhilî şarj pompasının kararlı hale gelmesi için bekleme süresi */

    /* 2. IC1_CTRL Kayıtçısı Konfigürasyonu (Reg 0x02)
       Bits [3:2] GAIN = 01b (20 V/V Kazanç) -> Veri: 0x0004 */
    status = DRV8703_WriteRegister(hspi, DRV8703_REG_IC1_CTRL, 0x0004U);
    if (status != HAL_OK)
    {
        return status;
    }

    /* 3. OCP_CTRL Kayıtçısı Konfigürasyonu (Reg 0x03)
       Bits [5:4] OCP_DEG = 01b (2 us Süzme Süresi)
       Bits [3:0] OCP_TH  = 0010b (1.60V Eşik = 40A Aşırı Akım Koruması)
       Veri: (0x01 << 4) | 0x02 = 0x0012 */
    status = DRV8703_WriteRegister(hspi, DRV8703_REG_OCP_CTRL, 0x0012U);

    return status;
}

/**
  * @brief  Motor çalışma yönünü ve hızını PWM doluluk oranına göre ayarlar.
  * @param  htim: PWM konfigürasyonuna sahip Timer yapısı adresi
  * @param  speed_percentage: Motor hızı ve yönü (-100 ile +100 arası)
  * @retval Yok
  */
void DRV8703_SetMotorSpeed(TIM_HandleTypeDef *htim, int16_t speed_percentage)
{
    uint32_t compare_val = 0;

    /* Sınırlandırma (Clamping) */
    if (speed_percentage > 100)
    {
        speed_percentage = 100;
    }
    else if (speed_percentage < -100)
    {
        speed_percentage = -100;
    }

    /* Yön ve PWM Hesabı */
    if (speed_percentage > 0)
    {
        /* İleri Yön (Cam Kapanıyor) */
        HAL_GPIO_WritePin(MOTOR_DIR_GPIO_Port, MOTOR_DIR_Pin, GPIO_PIN_SET);
        compare_val = ((uint32_t)speed_percentage * PWM_TIMER_ARR_MAX) / 100U;
        __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, compare_val);
    }
    else if (speed_percentage < 0)
    {
        /* Geri Yön (Cam Açılıyor) */
        HAL_GPIO_WritePin(MOTOR_DIR_GPIO_Port, MOTOR_DIR_Pin, GPIO_PIN_RESET);
        compare_val = ((uint32_t)(-speed_percentage) * PWM_TIMER_ARR_MAX) / 100U;
        __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, compare_val);
    }
    else
    {
        /* Motor Durma Durumu (Duty %0) */
        __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, 0U);
    }
}
