#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

typedef enum
{
  NONE,
  UP,
  DOWN,
  SELECT
} UIAction;

class ButtonControls
{
public:
  virtual ~ButtonControls() {}
  virtual void setup() = 0;
  virtual void poll() = 0;
  virtual bool did_wake_from_deep_sleep() = 0;
  virtual UIAction get_deep_sleep_action() = 0;
  virtual void setup_deep_sleep() = 0;
};
