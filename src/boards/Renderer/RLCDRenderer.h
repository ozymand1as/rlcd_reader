#pragma once

#include "../../lib/Epub/Renderer/Renderer.h"
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <driver/spi_master.h>

class RLCDRenderer : public Renderer
{
private:
  uint8_t *m_frame_buffer;
  int m_width;
  int m_height;
  int m_buffer_size;
  
  // Font properties
  int m_line_height;
  int m_space_width;
  
  // Internal drawing helpers
  void set_pixel(int x, int y, bool black);
  bool get_pixel(int x, int y);
  void draw_char(int x, int y, char c, bool bold, bool italic);
  
  // RLCD display functions
  void display_flush();
  void set_window(int x0, int y0, int x1, int y1);
  
  // Simple bitmap font (5x7)
  static const uint8_t font5x7[][5];
  
public:
  RLCDRenderer(int width, int height);
  ~RLCDRenderer();
  
  // Renderer interface
  void draw_pixel(int x, int y, uint8_t color) override;
  int get_text_width(const char *text, bool bold = false, bool italic = false) override;
  void draw_text(int x, int y, const char *text, bool bold = false, bool italic = false) override;
  void draw_rect(int x, int y, int width, int height, uint8_t color = 0) override;
  void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color) override;
  void draw_circle(int x, int y, int r, uint8_t color = 0) override;
  void fill_rect(int x, int y, int width, int height, uint8_t color = 0) override;
  void fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color) override;
  void fill_circle(int x, int y, int r, uint8_t color = 0) override;
  void needs_gray(uint8_t color) override;
  bool has_gray() override;
  void show_busy() override;
  void show_img(int x, int y, int width, int height, const uint8_t *img_buffer) override;
  void clear_screen() override;
  void flush_display() override;
  void flush_area(int x, int y, int width, int height) override;
  
  int get_page_width() override;
  int get_page_height() override;
  int get_space_width() override;
  int get_line_height() override;
};
