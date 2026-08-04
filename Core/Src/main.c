/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Smart Window Lift ECU - Ana Kontrol ve Entegrasyon Katmanı
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "drv8703.h"
#include "anti_pinch.h"
#include "can_handler.h"

/* Donanım Bekleme Yapıları (CubeMX Tarafından Oluşturulur) -------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

FDCAN_HandleTypeDef hfdcan1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim1;

/* Global Değişkenler --------------------------------------------------------*/
static uint32_t adc_dma_buffer[1] = {0}; /* ADC DMA Dairesel Tamponu */
static uint32_t last_can_tx_tick = 0;    /* CAN Durum Gönderim Zamanlayıcısı */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_FDCAN1_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM1_Init(void);

/**
  * @brief  Uygulama Giriş Noktası
  */
int main(void)
{
    /* 1. STM32 Alt Sistem ve Sistem Saat Donanım İlklendirmesi */
    HAL_Init();
    SystemClock_Config();

    /* 2. Çevresel Birim Çevre Bağlantı İnsitiyatifleri (Peripherals Init) */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC1_Init();
    MX_FDCAN1_Init();
    MX_SPI1_Init();
    MX_TIM1_Init();

    /* 3. DRV8703-Q1 Motor Sürücü Konfigürasyonu (SPI ile Registir Ayarları) */
    if (DRV8703_Init(&hspi1) != HAL_OK)
    {
        /* Entegre başlatılamadıysa Güvenli Moda Geç */
        Error_Handler();
    }

    /* 4. CAN Bus Donanım ve Filtre Yapılandırması */
    if (CANHandler_Init(&hfdcan1) != HAL_OK)
    {
        Error_Handler();
    }

    /* 5. Anti-Pinch Algoritma Katmanını Sıfırla */
    AntiPinch_Init();

    /* 6. PWM Çıkışını (Timer 1 Kanal 1) Başlat (Başlangıç Duty: %0) */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

    /* 7. ADC1 Akım Okumalarını DMA Dairesel Modda Kesintisiz Başlat */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buffer, 1);

    /* Sonsuz Çalışma Döngüsü (Super Loop) */
    while (1)
    {
        /* A. CAN Bus Üzerinden Yeni Komut Gelip Gelmediğini Denetle */
        if (CANHandler_IsNewCommandAvailable())
        {
            CAN_Command_t cmd = CANHandler_GetLatestCommand();

            switch (cmd)
            {
                case CAN_CMD_MANUAL_UP:
                case CAN_CMD_AUTO_UP:
                    /* Cam Kapanıyor (İleri Yön: +100 Hız) */
                    AntiPinch_StartMotor(&htim1, 100);
                    break;

                case CAN_CMD_MANUAL_DOWN:
                case CAN_CMD_AUTO_DOWN:
                    /* Cam Açılıyor (Geri Yön: -100 Hız) */
                    AntiPinch_StartMotor(&htim1, -100);
                    break;

                case CAN_CMD_STOP:
                default:
                    AntiPinch_StopMotor(&htim1);
                    break;
            }
        }

        /* B. Anti-Pinch Algoritma Durum Makinesini Tetikle (1 ms Döngü) */
        uint16_t current_adc_raw = (uint16_t)adc_dma_buffer[0];
        AntiPinch_Process(&htim1, current_adc_raw);

        /* C. Periyodik CAN Durum Bildirimi Yayınla (Her 50 ms'de Bir) */
        if ((HAL_GetTick() - last_can_tx_tick) >= 50U)
        {
            last_can_tx_tick = HAL_GetTick();

            CAN_FaultCode_t fault = CAN_FAULT_NONE;
            AntiPinch_State_t current_state = AntiPinch_GetState();

            if (current_state == ANTIPINCH_STATE_REVERSING)
            {
                fault = CAN_FAULT_ANTIPINCH;
            }
            else if (current_state == ANTIPINCH_STATE_FAULT)
            {
                fault = CAN_FAULT_HARDWARE;
            }

            CANHandler_SendStatus(&hfdcan1, current_state, current_adc_raw, fault);
        }

        /* 1 ms Sabit Zaman Çözünürlüklü Döngü Beklemesi */
        HAL_Delay(1);
    }
}

/**
  * @brief  DRV8703 nFAULT Pini Kesme Çağrısı (Donanımsal 40A Aşırı Akım / Sıcaklık Arızası)
  * @param  GPIO_Pin: Kesmeyi Tetikleyen Pin
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == DRV8703_NFAULT_Pin)
    {
        /* Entegre arıza durumuna geçti (nFAULT = 0), Motoru İvedilikle Kapat */
        AntiPinch_HandleHardwareFault(&htim1);
    }
}

/**
  * @brief  Hata Yönetim Fonksiyonu
  */
void Error_Handler(void)
{
    __disable_irq();
    /* Motor PWM'i Güvenlik İçin Kesilir */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    
    while (1)
    {
        /* Sistem Kilitlenme Modu */
    }
}
