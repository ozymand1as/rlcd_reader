#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <stdint.h>

class Renderer;
class ButtonControls;
class Battery;

class Board
{
public:
  virtual void power_up() = 0;
  virtual void prepare_to_sleep() = 0;
  virtual Renderer *get_renderer() = 0;
  virtual void start_filesystem();
  virtual void stop_filesystem();
  virtual Battery *get_battery();
  virtual ButtonControls *get_button_controls(xQueueHandle ui_queue) = 0;
  
  static Board *factory();

protected:
  void *m_sd_card = nullptr;
};
