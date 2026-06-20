#include "Board.h"
#include "RLCD_Board.h"
#include <esp_log.h>
#include "SDCard.h"
#include "battery/ADCBattery.h"

static const char *TAG = "Board";

Board *Board::factory()
{
#ifdef BOARD_TYPE_RLCD_42
  ESP_LOGI(TAG, "Creating RLCD 4.2 board");
  return new RLCD_Board();
#endif
  
  ESP_LOGE(TAG, "No board type defined");
  return nullptr;
}

void Board::start_filesystem()
{
  ESP_LOGI(TAG, "Starting SD card filesystem");
  m_sd_card = new SDCard(
    "/fs",
    (gpio_num_t)SD_CARD_PIN_NUM_MISO,
    (gpio_num_t)SD_CARD_PIN_NUM_MOSI,
    (gpio_num_t)SD_CARD_PIN_NUM_CLK,
    (gpio_num_t)SD_CARD_PIN_NUM_CS
  );
}

void Board::stop_filesystem()
{
  if (m_sd_card)
  {
    ESP_LOGI(TAG, "Stopping SD card filesystem");
    delete (SDCard *)m_sd_card;
    m_sd_card = nullptr;
  }
}

Battery *Board::get_battery()
{
#ifdef BATTERY_ADC_CHANNEL
  return new ADCBattery((adc1_channel_t)BATTERY_ADC_CHANNEL);
#else
  return nullptr;
#endif
}
