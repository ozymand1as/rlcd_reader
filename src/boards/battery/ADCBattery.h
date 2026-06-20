#pragma once

#include "Battery.h"
#include <driver/adc.h>

class ADCBattery : public Battery
{
private:
  adc1_channel_t m_channel;
  float m_vref;
  float m_divider_ratio;
  
public:
  ADCBattery(adc1_channel_t channel, float vref = 3.3f, float divider_ratio = 2.0f);
  ~ADCBattery();
  
  void setup() override;
  float get_voltage() override;
  float get_percentage() override;
};
