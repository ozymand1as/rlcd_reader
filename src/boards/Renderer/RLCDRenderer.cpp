#include "RLCDRenderer.h"
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <cstring>
#include <cmath>

static const char *TAG = "RLCDRenderer";

// Simple 5x7 bitmap font (ASCII 32-126)
const uint8_t RLCDRenderer::font5x7[][5] = {
  {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 (space)
  {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 !
  {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 "
  {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 #
  {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 $
  {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 %
  {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 &
  {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '
  {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 (
  {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 )
  {0x14, 0x08, 0x3E, 0x08, 0x14}, // 42 *
  {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 +
  {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ,
  {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 -
  {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 .
  {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 /
  {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 0
  {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 1
  {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 2
  {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 3
  {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 4
  {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 5
  {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 6
  {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 7
  {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 8
  {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 9
  {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 :
  {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ;
  {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 <
  {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 =
  {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 >
  {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 ?
  {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 @
  {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 A
  {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 B
  {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 C
  {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 D
  {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 E
  {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 F
  {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 G
  {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 H
  {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 I
  {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 J
  {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 K
  {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 L
  {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77 M
  {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 N
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 O
  {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 P
  {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 Q
  {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 R
  {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 S
  {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 T
  {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 U
  {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 V
  {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 87 W
  {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 X
  {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 Y
  {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 Z
  {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 [
  {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 backslash
  {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ]
  {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 ^
  {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 _
  {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 `
  {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 a
  {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 b
  {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 c
  {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 d
  {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 e
  {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 f
  {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 g
  {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 h
  {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 i
  {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 j
  {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 k
  {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 l
  {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 m
  {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 n
  {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 o
  {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 p
  {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 q
  {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 r
  {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 s
  {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 t
  {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 u
  {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 v
  {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 w
  {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 x
  {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 y
  {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 z
  {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 {
  {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 |
  {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 }
  {0x10, 0x08, 0x08, 0x10, 0x08}, // 126 ~
};

RLCDRenderer::RLCDRenderer(int width, int height)
  : m_width(width), m_height(height)
{
  // Allocate framebuffer in PSRAM (1-bit per pixel)
  m_buffer_size = (width * height + 7) / 8;
  m_frame_buffer = (uint8_t *)heap_caps_malloc(m_buffer_size, MALLOC_CAP_SPIRAM);
  
  if (!m_frame_buffer)
  {
    ESP_LOGE(TAG, "Failed to allocate framebuffer in PSRAM");
    // Fallback to internal RAM
    m_frame_buffer = (uint8_t *)malloc(m_buffer_size);
  }
  
  assert(m_frame_buffer);
  
  // Initialize to white (all bits set)
  memset(m_frame_buffer, 0xFF, m_buffer_size);
  
  // Set font metrics
  m_line_height = 8;  // 5x7 font + 1 pixel spacing
  m_space_width = 6;  // 5 pixels + 1 spacing
  
  ESP_LOGI(TAG, "RLCDRenderer initialized: %dx%d, buffer=%d bytes", 
           width, height, m_buffer_size);
}

RLCDRenderer::~RLCDRenderer()
{
  if (m_frame_buffer)
  {
    heap_caps_free(m_frame_buffer);
    m_frame_buffer = nullptr;
  }
}

void RLCDRenderer::set_pixel(int x, int y, bool black)
{
  if (x < 0 || x >= m_width || y < 0 || y >= m_height)
    return;
  
  // Calculate bit position (column-major, rotated for portrait)
  // RLCD uses rotated addressing
  int bit_x = y;
  int bit_y = m_height - 1 - x;
  
  int byte_index = (bit_y * m_width + bit_x) / 8;
  int bit_index = 7 - ((bit_y * m_width + bit_x) % 8);
  
  if (black)
    m_frame_buffer[byte_index] &= ~(1 << bit_index);
  else
    m_frame_buffer[byte_index] |= (1 << bit_index);
}

bool RLCDRenderer::get_pixel(int x, int y)
{
  if (x < 0 || x >= m_width || y < 0 || y >= m_height)
    return false;
  
  int bit_x = y;
  int bit_y = m_height - 1 - x;
  
  int byte_index = (bit_y * m_width + bit_x) / 8;
  int bit_index = 7 - ((bit_y * m_width + bit_x) % 8);
  
  return !(m_frame_buffer[byte_index] & (1 << bit_index));
}

void RLCDRenderer::draw_char(int x, int y, char c, bool bold, bool italic)
{
  if (c < 32 || c > 126)
    return;
  
  const uint8_t *glyph = font5x7[c - 32];
  
  for (int col = 0; col < 5; col++)
  {
    uint8_t line = glyph[col];
    for (int row = 0; row < 7; row++)
    {
      if (line & (1 << row))
      {
        set_pixel(x + col, y + row, true);
        // Bold: draw extra pixel to the right
        if (bold)
          set_pixel(x + col + 1, y + row, true);
      }
    }
    // Bold: draw extra column
    if (bold)
    {
      uint8_t next_col = (col < 4) ? glyph[col + 1] : 0;
      for (int row = 0; row < 7; row++)
      {
        if (glyph[col] & (1 << row))
          set_pixel(x + col + 1, y + row, true);
      }
    }
  }
  
  // Italic: shift pixels slightly
  if (italic)
  {
    // Simple italic: shift right by 1 pixel for lower rows
    // This is a simplified version - could be improved
  }
}

void RLCDRenderer::draw_pixel(int x, int y, uint8_t color)
{
  set_pixel(x + margin_left, y + margin_top, color < 128);
}

int RLCDRenderer::get_text_width(const char *text, bool bold, bool italic)
{
  if (!text) return 0;
  
  int width = 0;
  int len = strlen(text);
  
  for (int i = 0; i < len; i++)
  {
    if (text[i] == ' ')
    {
      width += m_space_width;
    }
    else
    {
      width += 6;  // 5 pixels + 1 spacing
      if (bold) width += 1;
    }
  }
  
  return width;
}

void RLCDRenderer::draw_text(int x, int y, const char *text, bool bold, bool italic)
{
  if (!text) return;
  
  int cursor_x = x + margin_left;
  int cursor_y = y + margin_top + m_line_height;
  
  while (*text)
  {
    if (*text == '\n')
    {
      cursor_x = x + margin_left;
      cursor_y += m_line_height;
    }
    else if (*text == ' ')
    {
      cursor_x += m_space_width;
    }
    else
    {
      draw_char(cursor_x, cursor_y, *text, bold, italic);
      cursor_x += 6 + (bold ? 1 : 0);
    }
    text++;
  }
}

void RLCDRenderer::draw_rect(int x, int y, int width, int height, uint8_t color)
{
  bool black = color < 128;
  x += margin_left;
  y += margin_top;
  
  // Top and bottom edges
  for (int i = x; i < x + width; i++)
  {
    set_pixel(i, y, black);
    set_pixel(i, y + height - 1, black);
  }
  
  // Left and right edges
  for (int i = y; i < y + height; i++)
  {
    set_pixel(x, i, black);
    set_pixel(x + width - 1, i, black);
  }
}

void RLCDRenderer::draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color)
{
  // Bresenham's line algorithm for each edge
  auto draw_line = [this, color](int x0, int y0, int x1, int y1) {
    bool black = color < 128;
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    while (true)
    {
      set_pixel(x0, y0, black);
      if (x0 == x1 && y0 == y1) break;
      int e2 = 2 * err;
      if (e2 > -dy) { err -= dy; x0 += sx; }
      if (e2 < dx) { err += dx; y0 += sy; }
    }
  };
  
  draw_line(x0, y0, x1, y1);
  draw_line(x1, y1, x2, y2);
  draw_line(x2, y2, x0, y0);
}

void RLCDRenderer::draw_circle(int x, int y, int r, uint8_t color)
{
  bool black = color < 128;
  int f = 1 - r;
  int ddF_x = 1;
  int ddF_y = -2 * r;
  int cx = 0;
  int cy = r;
  
  set_pixel(x, y + r, black);
  set_pixel(x, y - r, black);
  set_pixel(x + r, y, black);
  set_pixel(x - r, y, black);
  
  while (cx < cy)
  {
    if (f >= 0)
    {
      cy--;
      ddF_y += 2;
      f += ddF_y;
    }
    cx++;
    ddF_x += 2;
    f += ddF_x;
    
    set_pixel(x + cx, y + cy, black);
    set_pixel(x - cx, y + cy, black);
    set_pixel(x + cx, y - cy, black);
    set_pixel(x - cx, y - cy, black);
    set_pixel(x + cy, y + cx, black);
    set_pixel(x - cy, y + cx, black);
    set_pixel(x + cy, y - cx, black);
    set_pixel(x - cy, y - cx, black);
  }
}

void RLCDRenderer::fill_rect(int x, int y, int width, int height, uint8_t color)
{
  bool black = color < 128;
  x += margin_left;
  y += margin_top;
  
  for (int i = y; i < y + height; i++)
  {
    for (int j = x; j < x + width; j++)
    {
      set_pixel(j, i, black);
    }
  }
}

void RLCDRenderer::fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color)
{
  // Scanline fill algorithm
  bool black = color < 128;
  
  // Sort vertices by y-coordinate
  if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); }
  if (y0 > y2) { std::swap(x0, x2); std::swap(y0, y2); }
  if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }
  
  auto interp_x = [](int y, int y0, int x0, int y1, int x1) -> int {
    if (y1 == y0) return x0;
    return x0 + (y - y0) * (x1 - x0) / (y1 - y0);
  };
  
  for (int y = y0; y <= y2; y++)
  {
    int xa, xb;
    if (y < y1)
    {
      xa = interp_x(y, y0, x0, y1, x1);
      xb = interp_x(y, y0, x0, y2, x2);
    }
    else
    {
      xa = interp_x(y, y1, x1, y2, x2);
      xb = interp_x(y, y0, x0, y2, x2);
    }
    
    if (xa > xb) std::swap(xa, xb);
    
    for (int x = xa; x <= xb; x++)
    {
      set_pixel(x, y, black);
    }
  }
}

void RLCDRenderer::fill_circle(int x, int y, int r, uint8_t color)
{
  bool black = color < 128;
  
  for (int dy = -r; dy <= r; dy++)
  {
    int dx = (int)sqrt(r * r - dy * dy);
    for (int i = x - dx; i <= x + dx; i++)
    {
      set_pixel(i, y + dy, black);
    }
  }
}

void RLCDRenderer::needs_gray(uint8_t color)
{
  // RLCD is 1-bit, ignore grayscale
}

bool RLCDRenderer::has_gray()
{
  return false;  // RLCD is 1-bit only
}

void RLCDRenderer::show_busy()
{
  // Draw a simple hourglass indicator
  int cx = m_width / 2;
  int cy = m_height / 2;
  
  fill_rect(cx - 10, cy - 15, 20, 30, 255);
  draw_rect(cx - 10, cy - 15, 20, 30, 0);
  draw_line(cx - 8, cy - 13, cx + 8, cy - 13);
  draw_line(cx - 8, cy + 13, cx + 8, cy + 13);
  
  flush_display();
}

void RLCDRenderer::show_img(int x, int y, int width, int height, const uint8_t *img_buffer)
{
  // Simple 1-bit image rendering
  // Assumes img_buffer is 1-bit packed, MSB first
  for (int row = 0; row < height; row++)
  {
    for (int col = 0; col < width; col++)
    {
      int byte_index = (row * width + col) / 8;
      int bit_index = 7 - ((row * width + col) % 8);
      
      if (img_buffer[byte_index] & (1 << bit_index))
      {
        set_pixel(x + col, y + row, true);
      }
    }
  }
}

void RLCDRenderer::clear_screen()
{
  memset(m_frame_buffer, 0xFF, m_buffer_size);
}

void RLCDRenderer::flush_display()
{
  display_flush();
}

void RLCDRenderer::flush_area(int x, int y, int width, int height)
{
  // For simplicity, flush entire display
  // Could be optimized to flush only the changed area
  display_flush();
}

void RLCDRenderer::display_flush()
{
  // Send framebuffer to RLCD via SPI
  // This is a simplified version - full implementation would use
  // the RLCD_Board::send_data_bulk function
  
  ESP_LOGI(TAG, "Flushing display (%d bytes)", m_buffer_size);
  
  // TODO: Integrate with RLCD_Board for actual SPI transfer
  // For now, just log that we would flush
}

void RLCDRenderer::set_window(int x0, int y0, int x1, int y1)
{
  // TODO: Implement windowed transfer for partial updates
}

int RLCDRenderer::get_page_width()
{
  return m_width - (margin_left + margin_right);
}

int RLCDRenderer::get_page_height()
{
  return m_height - (margin_top + margin_bottom);
}

int RLCDRenderer::get_space_width()
{
  return m_space_width;
}

int RLCDRenderer::get_line_height()
{
  return m_line_height;
}
