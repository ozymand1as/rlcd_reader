#pragma once

#include "Board.h"
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>

class RLCDRenderer;

class RLCD_Board : public Board
{
private:
  spi_device_handle_t m_spi;
  RLCDRenderer *m_renderer;
  
  void init_spi();
  void init_display();

public:
  RLCD_Board();
  ~RLCD_Board();
  
  void power_up() override;
  void prepare_to_sleep() override;
  Renderer *get_renderer() override;
  ButtonControls *get_button_controls(xQueueHandle ui_queue) override;
  
  static void send_command(uint8_t cmd);
  static void send_data(uint8_t data);
  static void send_data_bulk(uint8_t *data, int len);
};
