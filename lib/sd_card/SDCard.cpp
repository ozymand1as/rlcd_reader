#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "SDCard.h"

static const char *TAG = "SDC";

SDCard::SDCard(const char *mount_point, gpio_num_t clk, gpio_num_t cmd, gpio_num_t d0, int width)
{
  m_mount_point = mount_point;

  ESP_LOGI(TAG, "Initializing SD card (SDMMC 1-bit, CLK=%d CMD=%d D0=%d)", clk, cmd, d0);

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = true,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = width;
  slot_config.clk = clk;
  slot_config.cmd = cmd;
  slot_config.d0 = d0;
  slot_config.d1 = GPIO_NUM_NC;
  slot_config.d2 = GPIO_NUM_NC;
  slot_config.d3 = GPIO_NUM_NC;
  slot_config.cd = GPIO_NUM_NC;
  slot_config.wp = GPIO_NUM_NC;

  esp_err_t ret = esp_vfs_fat_sdmmc_mount(m_mount_point.c_str(), &host, &slot_config, &mount_config, &m_card);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount filesystem.");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to initialize the card (%s)", esp_err_to_name(ret));
    }
    return;
  }

  sdmmc_card_print_info(stdout, m_card);
  ESP_LOGI(TAG, "SD card mounted at %s", m_mount_point.c_str());
}

SDCard::~SDCard()
{
  if (m_card)
  {
    esp_vfs_fat_sdcard_unmount(m_mount_point.c_str(), m_card);
    ESP_LOGI(TAG, "Card unmounted");
  }
}
