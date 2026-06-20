#pragma once

#include <driver/adc.h>

class Battery
{
protected:
  float m_voltage;
  float m_percentage;
  
public:
  Battery() : m_voltage(0.0f), m_percentage(0.0f) {}
  virtual ~Battery() {}
  
  virtual void setup() = 0;
  virtual float get_voltage() { return m_voltage; }
  virtual float get_percentage() { return m_percentage; }
};
