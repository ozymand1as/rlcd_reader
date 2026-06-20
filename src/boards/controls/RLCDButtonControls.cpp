#include "RLCDButtonControls.h"
#include <esp_log.h>
#include <esp_sleep.h>

static const char *TAG = "RLCDButtonControls";

// Debounce time in milliseconds
#define DEBOUNCE_MS 50

RLCDButtonControls::RLCDButtonControls(gpio_num_t up, gpio_num_t down, gpio_num_t select,
                                       int active_level, QueueHandle_t ui_queue)
  : m_up_pin(up), m_down_pin(down), m_select_pin(select),
    m_active_level(active_level), m_ui_queue(ui_queue),
    m_last_interrupt_time(0)
{
}

void RLCDButtonControls::setup()
{
  ESP_LOGI(TAG, "Setting up button controls");
  
  // Configure GPIO pins as inputs with pull-ups
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  
  // Configure each button pin
  io_conf.pin_bit_mask = (1ULL << m_up_pin);
  gpio_config(&io_conf);
  
  io_conf.pin_bit_mask = (1ULL << m_down_pin);
  gpio_config(&io_conf);
  
  io_conf.pin_bit_mask = (1ULL << m_select_pin);
  gpio_config(&io_conf);
  
  // Install GPIO ISR service
  gpio_install_isr_service(0);
  
  // Hook ISR handlers for each button
  gpio_isr_handler_add(m_up_pin, gpio_isr_handler, this);
  gpio_isr_handler_add(m_down_pin, gpio_isr_handler, this);
  gpio_isr_handler_add(m_select_pin, gpio_isr_handler, this);
  
  ESP_LOGI(TAG, "Button controls initialized");
}

void IRAM_ATTR RLCDButtonControls::gpio_isr_handler(void *arg)
{
  RLCDButtonControls *controls = (RLCDButtonControls *)arg;
  uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
  
  // Simple debounce
  if ((current_time - controls->m_last_interrupt_time) > DEBOUNCE_MS)
  {
    controls->m_last_interrupt_time = current_time;
    
    UIAction action = NONE;
    
    // Check which button was pressed
    if (gpio_get_level(controls->m_up_pin) == controls->m_active_level)
    {
      action = UP;
    }
    else if (gpio_get_level(controls->m_down_pin) == controls->m_active_level)
    {
      action = DOWN;
    }
    else if (gpio_get_level(controls->m_select_pin) == controls->m_active_level)
    {
      action = SELECT;
    }
    
    if (action != NONE)
    {
      xQueueSendFromISR(controls->m_ui_queue, &action, NULL);
    }
  }
}

void RLCDButtonControls::poll()
{
  // Polling is handled by interrupts, but we could add
  // additional logic here if needed
}

bool RLCDButtonControls::did_wake_from_deep_sleep()
{
  // Check if we woke up from deep sleep
  esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
  
  // Check if any button caused the wake-up
  if (wakeup_cause == ESP_SLEEP_WAKEUP_GPIO)
  {
    ESP_LOGI(TAG, "Woke up from deep sleep via GPIO");
    return true;
  }
  
  return false;
}

UIAction RLCDButtonControls::get_deep_sleep_action()
{
  // Determine which button caused the wake-up
  if (gpio_get_level(m_up_pin) == m_active_level)
  {
    return UP;
  }
  else if (gpio_get_level(m_down_pin) == m_active_level)
  {
    return DOWN;
  }
  else if (gpio_get_level(m_select_pin) == m_active_level)
  {
    return SELECT;
  }
  
  return NONE;
}

void RLCDButtonControls::setup_deep_sleep()
{
  ESP_LOGI(TAG, "Configuring deep sleep with GPIO wake-up");
  
  // Configure GPIO pins as wake-up sources
  gpio_wakeup_enable(m_up_pin, m_active_level ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
  gpio_wakeup_enable(m_down_pin, m_active_level ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
  gpio_wakeup_enable(m_select_pin, m_active_level ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
  
  // Enable GPIO as wake-up source
  esp_sleep_enable_gpio_wakeup();
}
