#include "RLCDButtonControls.h"
#include <esp_log.h>
#include <esp_sleep.h>

static const char *TAG = "RLCDButtonControls";

#define DEBOUNCE_MS 50
#define DOUBLE_CLICK_MS 300
#define LONG_PRESS_MS 1000

RLCDButtonControls::RLCDButtonControls(gpio_num_t back, gpio_num_t key,
                                       int active_level, QueueHandle_t ui_queue)
  : m_back_pin(back), m_key_pin(key),
    m_active_level(active_level), m_ui_queue(ui_queue),
    m_last_key_time(0), m_key_click_count(0)
{
}

void RLCDButtonControls::setup()
{
  ESP_LOGI(TAG, "Setting up button controls: BACK=%d, KEY=%d", m_back_pin, m_key_pin);
  
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  
  io_conf.pin_bit_mask = (1ULL << m_back_pin);
  gpio_config(&io_conf);
  
  io_conf.pin_bit_mask = (1ULL << m_key_pin);
  gpio_config(&io_conf);
  
  gpio_install_isr_service(0);
  gpio_isr_handler_add(m_back_pin, gpio_isr_handler, this);
  gpio_isr_handler_add(m_key_pin, gpio_isr_handler, this);
  
  ESP_LOGI(TAG, "Button controls initialized");
}

void IRAM_ATTR RLCDButtonControls::gpio_isr_handler(void *arg)
{
  RLCDButtonControls *controls = (RLCDButtonControls *)arg;
  uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
  
  if ((current_time - controls->m_last_key_time) < DEBOUNCE_MS)
  {
    return;
  }
  
  // BACK button - always sends SELECT (go back)
  if (gpio_get_level(controls->m_back_pin) == controls->m_active_level)
  {
    UIAction action = SELECT;
    xQueueSendFromISR(controls->m_ui_queue, &action, NULL);
    controls->m_last_key_time = current_time;
    return;
  }
  
  // KEY button - multi-click detection
  if (gpio_get_level(controls->m_key_pin) == controls->m_active_level)
  {
    uint32_t time_since_last = current_time - controls->m_last_key_time;
    
    if (time_since_last < DOUBLE_CLICK_MS)
    {
      controls->m_key_click_count++;
    }
    else
    {
      controls->m_key_click_count = 1;
    }
    
    controls->m_last_key_time = current_time;
  }
  else
  {
    // Key released - check for single/double click
    uint32_t time_since_press = current_time - controls->m_last_key_time;
    
    if (controls->m_key_click_count == 1 && time_since_press < DOUBLE_CLICK_MS)
    {
      // Single click -> DOWN
      UIAction action = DOWN;
      xQueueSendFromISR(controls->m_ui_queue, &action, NULL);
    }
    else if (controls->m_key_click_count >= 2)
    {
      // Double click -> UP
      UIAction action = UP;
      xQueueSendFromISR(controls->m_ui_queue, &action, NULL);
    }
    else if (time_since_press >= LONG_PRESS_MS)
    {
      // Long press -> SELECT
      UIAction action = SELECT;
      xQueueSendFromISR(controls->m_ui_queue, &action, NULL);
    }
    
    controls->m_key_click_count = 0;
  }
}

void RLCDButtonControls::poll()
{
}

bool RLCDButtonControls::did_wake_from_deep_sleep()
{
  esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
  return (wakeup_cause == ESP_SLEEP_WAKEUP_GPIO);
}

UIAction RLCDButtonControls::get_deep_sleep_action()
{
  if (gpio_get_level(m_back_pin) == m_active_level)
  {
    return SELECT;
  }
  if (gpio_get_level(m_key_pin) == m_active_level)
  {
    return DOWN;
  }
  return NONE;
}

void RLCDButtonControls::setup_deep_sleep()
{
  ESP_LOGI(TAG, "Configuring deep sleep with GPIO wake-up");
  
  gpio_wakeup_enable(m_back_pin, m_active_level ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
  gpio_wakeup_enable(m_key_pin, m_active_level ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
  
  esp_sleep_enable_gpio_wakeup();
}
