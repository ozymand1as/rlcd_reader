#pragma once

#include "ButtonControls.h"
#include <driver/gpio.h>

class RLCDButtonControls : public ButtonControls
{
private:
  gpio_num_t m_back_pin;
  gpio_num_t m_key_pin;
  int m_active_level;
  QueueHandle_t m_ui_queue;
  
  uint32_t m_last_key_time;
  int m_key_click_count;
  
  static void IRAM_ATTR gpio_isr_handler(void *arg);

public:
  RLCDButtonControls(gpio_num_t back, gpio_num_t key,
                     int active_level, QueueHandle_t ui_queue);
  
  void setup() override;
  void poll() override;
  bool did_wake_from_deep_sleep() override;
  UIAction get_deep_sleep_action() override;
  void setup_deep_sleep() override;
};
