#include "Board.h"
#include "RLCD_Board.h"
#include <esp_log.h>

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
  // TODO: Initialize SD card
  ESP_LOGI(TAG, "Filesystem not implemented yet");
}

void Board::stop_filesystem()
{
  // TODO: Cleanup SD card
  ESP_LOGI(TAG, "Filesystem cleanup not implemented yet");
}

Battery *Board::get_battery()
{
  // TODO: Implement battery monitoring
  ESP_LOGI(TAG, "Battery monitoring not implemented yet");
  return nullptr;
}
