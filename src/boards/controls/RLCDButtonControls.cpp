#include "RLCDButtonControls.h"
#include <esp_log.h>
#include <esp_sleep.h>

static const char *TAG = "RLCDButtonControls";

#define DEBOUNCE_MS 50
#define DOUBLE_CLICK_MS 300
#define LONG_PRESS_MS 2000

RLCDButtonControls::RLCDButtonControls(gpio_num_t back, gpio_num_t key,
                                       gpio_num_t fwd, gpio_num_t prev,
                                       int active_level, QueueHandle_t ui_queue)
  : m_back_pin(back), m_key_pin(key),
    m_fwd_pin(fwd), m_prev_pin(prev),
    m_active_level(active_level), m_ui_queue(ui_queue),
    m_last_key_time(0), m_key_press_start(0), m_key_click_count(0), m_key_pressed(false),
    m_last_back_time(0), m_back_press_start(0), m_back_pressed(false),
    m_last_fwd_time(0), m_fwd_press_start(0), m_fwd_pressed(false),
    m_last_prev_time(0), m_prev_press_start(0), m_prev_pressed(false),
    m_dbg_fwd_edge(false), m_dbg_prev_edge(false),
    m_dbg_fwd_action(false), m_dbg_prev_action(false)
{
}

void RLCDButtonControls::setup()
{
  ESP_LOGI(TAG, "Setting up button controls: FWD=%d, PREV=%d",
           m_fwd_pin, m_prev_pin);

  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

  io_conf.pin_bit_mask = (1ULL << m_fwd_pin);
  gpio_config(&io_conf);

  io_conf.pin_bit_mask = (1ULL << m_prev_pin);
  gpio_config(&io_conf);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(m_fwd_pin, fwd_isr_handler, this);
  gpio_isr_handler_add(m_prev_pin, prev_isr_handler, this);

  // Initialize pressed states from actual GPIO levels to avoid stale state
  m_fwd_pressed = gpio_get_level(m_fwd_pin) == m_active_level;
  m_prev_pressed = gpio_get_level(m_prev_pin) == m_active_level;

  ESP_LOGI(TAG, "Button controls initialized: FWD pressed=%d PREV pressed=%d",
           m_fwd_pressed, m_prev_pressed);
}

void IRAM_ATTR RLCDButtonControls::fwd_isr_handler(void *arg)
{
  RLCDButtonControls *c = (RLCDButtonControls *)arg;
  uint32_t t = xTaskGetTickCount() * portTICK_PERIOD_MS;

  bool level = gpio_get_level(c->m_fwd_pin) == c->m_active_level;
  if (level != c->m_fwd_pressed)
  {
    c->m_dbg_fwd_edge = true;
    c->m_fwd_pressed = level;

    if ((t - c->m_last_fwd_time) < DEBOUNCE_MS) return;
    c->m_last_fwd_time = t;

    if (level)
    {
      c->m_fwd_press_start = t;
    }
    else
    {
      uint32_t held = t - c->m_fwd_press_start;
      UIAction action = (held >= LONG_PRESS_MS) ? SELECT : DOWN;
      c->m_dbg_fwd_action = true;
      xQueueSendFromISR(c->m_ui_queue, &action, NULL);
      c->m_fwd_press_start = 0;
    }
  }
}

void IRAM_ATTR RLCDButtonControls::prev_isr_handler(void *arg)
{
  RLCDButtonControls *c = (RLCDButtonControls *)arg;
  uint32_t t = xTaskGetTickCount() * portTICK_PERIOD_MS;

  bool level = gpio_get_level(c->m_prev_pin) == c->m_active_level;
  if (level != c->m_prev_pressed)
  {
    c->m_dbg_prev_edge = true;
    c->m_prev_pressed = level;

    if ((t - c->m_last_prev_time) < DEBOUNCE_MS) return;
    c->m_last_prev_time = t;

    if (level)
    {
      c->m_prev_press_start = t;
    }
    else
    {
      uint32_t held = t - c->m_prev_press_start;
      UIAction action = (held >= LONG_PRESS_MS) ? SELECT : UP;
      c->m_dbg_prev_action = true;
      xQueueSendFromISR(c->m_ui_queue, &action, NULL);
      c->m_prev_press_start = 0;
    }
  }
}

void RLCDButtonControls::poll()
{
  bool fwd_low = gpio_get_level(m_fwd_pin) == 0;
  bool prev_low = gpio_get_level(m_prev_pin) == 0;

  bool fwd_edge = m_dbg_fwd_edge;
  bool prev_edge = m_dbg_prev_edge;
  bool fwd_action = m_dbg_fwd_action;
  bool prev_action = m_dbg_prev_action;
  m_dbg_fwd_edge = false;
  m_dbg_prev_edge = false;
  m_dbg_fwd_action = false;
  m_dbg_prev_action = false;

  if (fwd_edge || prev_edge || fwd_action || prev_action || fwd_low || prev_low)
  {
    ESP_LOGI(TAG, "GPIO: FWD(GP%d)=%s PREV(GP%d)=%s | state: fwd_pressed=%d prev_pressed=%d | ISR: fwd_edge=%d prev_edge=%d fwd_action=%d prev_action=%d",
             m_fwd_pin, fwd_low ? "LOW" : "HIGH",
             m_prev_pin, prev_low ? "LOW" : "HIGH",
             m_fwd_pressed, m_prev_pressed,
             fwd_edge, prev_edge, fwd_action, prev_action);
  }
}

bool RLCDButtonControls::did_wake_from_deep_sleep()
{
  esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
  return (wakeup_cause == ESP_SLEEP_WAKEUP_GPIO);
}

UIAction RLCDButtonControls::get_deep_sleep_action()
{
  if (gpio_get_level(m_fwd_pin) == m_active_level)
  {
    return UP;
  }
  if (gpio_get_level(m_prev_pin) == m_active_level)
  {
    return DOWN;
  }
  return NONE;
}

void RLCDButtonControls::setup_deep_sleep()
{
  ESP_LOGI(TAG, "Configuring deep sleep with GPIO wake-up");

  gpio_wakeup_enable(m_fwd_pin, m_active_level ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
  gpio_wakeup_enable(m_prev_pin, m_active_level ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);

  esp_sleep_enable_gpio_wakeup();
}
