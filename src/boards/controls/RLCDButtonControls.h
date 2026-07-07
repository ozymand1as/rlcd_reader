#pragma once

#include "ButtonControls.h"
#include <driver/gpio.h>

class RLCDButtonControls : public ButtonControls
{
private:
  gpio_num_t m_back_pin;
  gpio_num_t m_key_pin;
  gpio_num_t m_fwd_pin;
  gpio_num_t m_prev_pin;
  int m_active_level;
  QueueHandle_t m_ui_queue;

  uint32_t m_last_key_time;
  uint32_t m_key_press_start;
  int m_key_click_count;
  bool m_key_pressed;
  uint32_t m_last_back_time;
  uint32_t m_back_press_start;
  bool m_back_pressed;
  uint32_t m_last_fwd_time;
  uint32_t m_fwd_press_start;
  bool m_fwd_pressed;
  uint32_t m_last_prev_time;
  uint32_t m_prev_press_start;
  bool m_prev_pressed;

  static void IRAM_ATTR fwd_isr_handler(void *arg);
  static void IRAM_ATTR prev_isr_handler(void *arg);

public:
  volatile bool m_dbg_fwd_edge;
  volatile bool m_dbg_prev_edge;
  volatile bool m_dbg_fwd_action;
  volatile bool m_dbg_prev_action;

  RLCDButtonControls(gpio_num_t back, gpio_num_t key,
                     gpio_num_t fwd, gpio_num_t prev,
                     int active_level, QueueHandle_t ui_queue);

  void setup() override;
  void poll() override;
  bool did_wake_from_deep_sleep() override;
  UIAction get_deep_sleep_action() override;
  void setup_deep_sleep() override;
};
