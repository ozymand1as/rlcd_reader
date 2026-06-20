#pragma once

#include "ButtonControls.h"
#include <driver/gpio.h>

class RLCDButtonControls : public ButtonControls
{
private:
  gpio_num_t m_up_pin;
  gpio_num_t m_down_pin;
  gpio_num_t m_select_pin;
  int m_active_level;
  QueueHandle_t m_ui_queue;
  
  // Debounce state
  uint32_t m_last_interrupt_time;
  
  static void IRAM_ATTR gpio_isr_handler(void *arg);

public:
  RLCDButtonControls(gpio_num_t up, gpio_num_t down, gpio_num_t select,
                     int active_level, QueueHandle_t ui_queue);
  
  void setup() override;
  void poll() override;
  bool did_wake_from_deep_sleep() override;
  UIAction get_deep_sleep_action() override;
  void setup_deep_sleep() override;
};
