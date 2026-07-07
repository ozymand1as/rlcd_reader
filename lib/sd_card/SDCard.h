#pragma once

#include <hal/gpio_types.h>
#include <driver/sdmmc_types.h>
#include <string>

class SDCard
{
private:
  std::string m_mount_point;
  sdmmc_card_t *m_card = nullptr;

public:
  SDCard(const char *mount_point, gpio_num_t clk, gpio_num_t cmd, gpio_num_t d0, int width = 1);
  ~SDCard();
  const std::string &get_mount_point() { return m_mount_point; }
};
