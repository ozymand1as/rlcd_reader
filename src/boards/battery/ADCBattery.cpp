#include "ADCBattery.h"
#include <esp_log.h>

static const char *TAG = "ADCBattery";

ADCBattery::ADCBattery(adc1_channel_t channel, float vref, float divider_ratio)
  : m_channel(channel), m_vref(vref), m_divider_ratio(divider_ratio)
{
}

ADCBattery::~ADCBattery()
{
}

void ADCBattery::setup()
{
  ESP_LOGI(TAG, "Setting up ADC battery monitor on channel %d", m_channel);
  
  // Configure ADC
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(m_channel, ADC_ATTEN_DB_11);
}

float ADCBattery::get_voltage()
{
  // Read ADC value
  int raw = adc1_get_raw(m_channel);
  
  // Convert to voltage (12-bit ADC, 3.3V reference)
  float voltage = (raw / 4095.0f) * m_vref * m_divider_ratio;
  
  m_voltage = voltage;
  return voltage;
}

float ADCBattery::get_percentage()
{
  float voltage = get_voltage();
  
  // Simple battery percentage calculation
  // Assumes 3.7V Li-ion battery (3.0V empty, 4.2V full)
  float min_voltage = 3.0f;
  float max_voltage = 4.2f;
  
  if (voltage >= max_voltage)
  {
    m_percentage = 100.0f;
  }
  else if (voltage <= min_voltage)
  {
    m_percentage = 0.0f;
  }
  else
  {
    m_percentage = ((voltage - min_voltage) / (max_voltage - min_voltage)) * 100.0f;
  }
  
  return m_percentage;
}
