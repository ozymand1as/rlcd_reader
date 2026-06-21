#pragma once

#include "../../lib/Epub/Renderer/Renderer.h"
#include <u8g2.h>
#include <string>

class U8g2RLCDRenderer : public Renderer
{
private:
  u8g2_t m_u8g2;
  int m_width;
  int m_height;
  
  // Current font settings
  bool m_bold;
  bool m_italic;
  int m_font_size;
  
  // Helper to set font based on style
  void setFont(bool bold, bool italic);

public:
  U8g2RLCDRenderer(int width, int height);
  ~U8g2RLCDRenderer();
  
  void init();
  
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
  
  // U8g2 specific methods
  u8g2_t *getU8g2() { return &m_u8g2; }
  void setFontSize(int size);
  
  // Deep sleep state
  bool dehydrate() override;
  bool hydrate() override;
  void reset() override;
};
