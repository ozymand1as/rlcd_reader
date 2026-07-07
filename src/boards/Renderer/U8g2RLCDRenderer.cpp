#include "U8g2RLCDRenderer.h"
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <u8x8.h>

static const char *TAG = "U8g2RLCD";

// Display pin definitions (matching RLCD_Board)
#define RLCD_PIN_MOSI GPIO_NUM_12
#define RLCD_PIN_SCK  GPIO_NUM_11
#define RLCD_PIN_DC   GPIO_NUM_5
#define RLCD_PIN_CS   GPIO_NUM_40
#define RLCD_PIN_RST  GPIO_NUM_41

// ST7305 display info for u8g2
#define ST7305_TILE_WIDTH 38
#define ST7305_TILE_HEIGHT 50
#define ST7305_BUFFER_ROW_BYTES (ST7305_TILE_WIDTH * 8)

// Forward declaration for u8g2 callback
static uint8_t u8x8_d_st7305_custom(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
static uint8_t u8x8_byte_custom(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

static u8x8_display_info_t st7305_display_info = {
  0, 1, 0, 0, 20, 50, 0, 0,
  24000000, 0, 4, 0, 0,
  ST7305_TILE_WIDTH, ST7305_TILE_HEIGHT,
  0, 0, 300, 400
};

// SPI handle for display communication
static spi_device_handle_t s_spi = nullptr;

U8g2RLCDRenderer::U8g2RLCDRenderer(int width, int height)
  : m_width(width), m_height(height), m_bold(false), m_italic(false), m_font_size(0)
{
}

U8g2RLCDRenderer::~U8g2RLCDRenderer()
{
}

void U8g2RLCDRenderer::init()
{
  ESP_LOGI(TAG, "Initializing U8g2 RLCD renderer");
  
  // Initialize SPI
  spi_bus_config_t bus_cfg = {};
  bus_cfg.mosi_io_num = RLCD_PIN_MOSI;
  bus_cfg.sclk_io_num = RLCD_PIN_SCK;
  bus_cfg.miso_io_num = -1;
  bus_cfg.quadwp_io_num = -1;
  bus_cfg.quadhd_io_num = -1;
  bus_cfg.max_transfer_sz = ST7305_TILE_WIDTH * 8 * ST7305_TILE_HEIGHT;
  
  esp_err_t ret = spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
  ESP_ERROR_CHECK(ret);
  
  spi_device_interface_config_t dev_cfg = {};
  dev_cfg.clock_speed_hz = 24000000;
  dev_cfg.mode = 0;
  dev_cfg.spics_io_num = -1;
  dev_cfg.queue_size = 10;
  
  ret = spi_bus_add_device(SPI3_HOST, &dev_cfg, &s_spi);
  ESP_ERROR_CHECK(ret);
  
  // Configure DC and RST pins
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL << RLCD_PIN_DC) | (1ULL << RLCD_PIN_RST) | (1ULL << RLCD_PIN_CS);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);
  gpio_set_level(RLCD_PIN_CS, 1);
  
  // Setup u8g2 with custom callbacks
  u8g2_t *u = &m_u8g2;
  u8x8_Setup(u8g2_GetU8x8(u), u8x8_d_st7305_custom, u8x8_dummy_cb, u8x8_byte_custom, u8x8_dummy_cb);
  
  // Allocate display buffer in PSRAM
  size_t buf_size = ST7305_BUFFER_ROW_BYTES * ST7305_TILE_HEIGHT;
  uint8_t *buf = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
  if (!buf)
  {
    ESP_LOGE(TAG, "Failed to allocate display buffer in PSRAM");
    buf = (uint8_t *)malloc(buf_size);
  }
  assert(buf);
  
  u8g2_SetupBuffer(u, buf, ST7305_TILE_HEIGHT, u8g2_ll_hvline_vertical_top_lsb, &u8g2_cb_r0);
  u8g2_InitDisplay(u);
  u8g2_SetPowerSave(u, 0);
  
  // Set default font
  setFont(false, false);
  
  ESP_LOGI(TAG, "U8g2 RLCD renderer initialized");
}

void U8g2RLCDRenderer::setFont(bool bold, bool italic)
{
  // Cyrillic fonts for Russian support
  // _t_cyrillic suffix = Latin + Cyrillic characters
  if (bold)
  {
    u8g2_SetFont(&m_u8g2, u8g2_font_6x13B_t_cyrillic);
  }
  else
  {
    u8g2_SetFont(&m_u8g2, u8g2_font_9x15_t_cyrillic);
  }
}

void U8g2RLCDRenderer::setFontSize(int size)
{
  m_font_size = size;
  
  switch (size)
  {
  case 0: // Small
    u8g2_SetFont(&m_u8g2, u8g2_font_7x13_t_cyrillic);
    break;
  case 1: // Medium (default)
    u8g2_SetFont(&m_u8g2, u8g2_font_9x15_t_cyrillic);
    break;
  case 2: // Large
    u8g2_SetFont(&m_u8g2, u8g2_font_10x20_t_cyrillic);
    break;
  case 3: // XL
    u8g2_SetFont(&m_u8g2, u8g2_font_inr24_t_cyrillic);
    break;
  default:
    u8g2_SetFont(&m_u8g2, u8g2_font_9x15_t_cyrillic);
    break;
  }
}

void U8g2RLCDRenderer::draw_pixel(int x, int y, uint8_t color)
{
  u8g2_SetDrawColor(&m_u8g2, color < 128 ? 0 : 1);
  u8g2_DrawPixel(&m_u8g2, x + margin_left, y + margin_top);
}

int U8g2RLCDRenderer::get_text_width(const char *text, bool bold, bool italic)
{
  setFont(bold, italic);
  return u8g2_GetStrWidth(&m_u8g2, text);
}

void U8g2RLCDRenderer::draw_text(int x, int y, const char *text, bool bold, bool italic)
{
  setFont(bold, italic);
  u8g2_SetDrawColor(&m_u8g2, 1);
  u8g2_DrawStr(&m_u8g2, x + margin_left, y + margin_top + u8g2_GetAscent(&m_u8g2), text);
}

void U8g2RLCDRenderer::draw_rect(int x, int y, int width, int height, uint8_t color)
{
  u8g2_SetDrawColor(&m_u8g2, color < 128 ? 0 : 1);
  u8g2_DrawFrame(&m_u8g2, x + margin_left, y + margin_top, width, height);
}

void U8g2RLCDRenderer::draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color)
{
  u8g2_SetDrawColor(&m_u8g2, color < 128 ? 0 : 1);
  u8g2_DrawTriangle(&m_u8g2, 
                     x0 + margin_left, y0 + margin_top,
                     x1 + margin_left, y1 + margin_top,
                     x2 + margin_left, y2 + margin_top);
}

void U8g2RLCDRenderer::draw_circle(int x, int y, int r, uint8_t color)
{
  u8g2_SetDrawColor(&m_u8g2, color < 128 ? 0 : 1);
  u8g2_DrawCircle(&m_u8g2, x + margin_left, y + margin_top, r, U8G2_DRAW_ALL);
}

void U8g2RLCDRenderer::fill_rect(int x, int y, int width, int height, uint8_t color)
{
  u8g2_SetDrawColor(&m_u8g2, color < 128 ? 0 : 1);
  u8g2_DrawBox(&m_u8g2, x + margin_left, y + margin_top, width, height);
}

void U8g2RLCDRenderer::fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color)
{
  u8g2_SetDrawColor(&m_u8g2, color < 128 ? 0 : 1);
  u8g2_DrawTriangle(&m_u8g2,
                     x0 + margin_left, y0 + margin_top,
                     x1 + margin_left, y1 + margin_top,
                     x2 + margin_left, y2 + margin_top);
}

void U8g2RLCDRenderer::fill_circle(int x, int y, int r, uint8_t color)
{
  u8g2_SetDrawColor(&m_u8g2, color < 128 ? 0 : 1);
  u8g2_DrawDisc(&m_u8g2, x + margin_left, y + margin_top, r, U8G2_DRAW_ALL);
}

void U8g2RLCDRenderer::needs_gray(uint8_t color)
{
  // RLCD is 1-bit, ignore grayscale
}

bool U8g2RLCDRenderer::has_gray()
{
  return false;
}

void U8g2RLCDRenderer::show_busy()
{
  clear_screen();
  u8g2_SetDrawColor(&m_u8g2, 1);
  u8g2_SetFont(&m_u8g2, u8g2_font_7x13_t_cyrillic);
  const char *busy = "Loading...";
  int w = u8g2_GetStrWidth(&m_u8g2, busy);
  u8g2_DrawStr(&m_u8g2, (m_width - w) / 2, m_height / 2, busy);
  flush_display();
}

void U8g2RLCDRenderer::show_img(int x, int y, int width, int height, const uint8_t *img_buffer)
{
  // Simple 1-bit image rendering
  for (int row = 0; row < height; row++)
  {
    for (int col = 0; col < width; col++)
    {
      int byte_index = (row * width + col) / 8;
      int bit_index = 7 - ((row * width + col) % 8);
      
      if (img_buffer[byte_index] & (1 << bit_index))
      {
        draw_pixel(x + col, y + row, 0);
      }
    }
  }
}

void U8g2RLCDRenderer::clear_screen()
{
  u8g2_ClearBuffer(&m_u8g2);
}

void U8g2RLCDRenderer::flush_display()
{
  u8g2_SendBuffer(&m_u8g2);
}

void U8g2RLCDRenderer::flush_area(int x, int y, int width, int height)
{
  // For simplicity, flush entire display
  flush_display();
}

void U8g2RLCDRenderer::draw_status_bar(int current_page, int total_pages, float battery_pct)
{
  u8g2_SetFont(&m_u8g2, u8g2_font_6x12_tf);
  u8g2_SetDrawColor(&m_u8g2, 1);

  // Page number in top-left
  char page_str[20];
  snprintf(page_str, sizeof(page_str), "%d/%d", current_page + 1, total_pages);
  u8g2_DrawStr(&m_u8g2, margin_left, 10, page_str);

  // Battery in top-right
  char bat_str[10];
  snprintf(bat_str, sizeof(bat_str), "%d%%", (int)battery_pct);
  int bat_w = u8g2_GetStrWidth(&m_u8g2, bat_str);
  u8g2_DrawStr(&m_u8g2, m_width - margin_right - bat_w, 10, bat_str);
}

int U8g2RLCDRenderer::get_page_width()
{
  return m_width - (margin_left + margin_right);
}

int U8g2RLCDRenderer::get_page_height()
{
  return m_height - (margin_top + margin_bottom);
}

int U8g2RLCDRenderer::get_space_width()
{
  return u8g2_GetStrWidth(&m_u8g2, " ");
}

int U8g2RLCDRenderer::get_line_height()
{
  return (u8g2_GetAscent(&m_u8g2) - u8g2_GetDescent(&m_u8g2)) * 3 / 2;
}

bool U8g2RLCDRenderer::dehydrate()
{
  ESP_LOGI(TAG, "Dehydrating display state");
  
  uint8_t *buf = u8g2_GetBufferPtr(&m_u8g2);
  uint16_t buf_len = u8g2_GetBufferTileHeight(&m_u8g2) * u8g2_GetBufferTileWidth(&m_u8g2) * 8;
  
  if (!buf || buf_len == 0)
  {
    ESP_LOGE(TAG, "No buffer to save");
    return false;
  }
  
  FILE *fp = fopen("/fs/front_buffer.z", "w");
  if (!fp)
  {
    ESP_LOGE(TAG, "Failed to open file for writing");
    return false;
  }
  
  // Write buffer size header
  fwrite(&buf_len, sizeof(uint16_t), 1, fp);
  // Write buffer data
  size_t written = fwrite(buf, 1, buf_len, fp);
  fclose(fp);
  
  if (written != buf_len)
  {
    ESP_LOGE(TAG, "Failed to write buffer: %lu bytes", (unsigned long)written);
    remove("/fs/front_buffer.z");
    return false;
  }
  
  ESP_LOGI(TAG, "Buffer saved: %lu bytes", (unsigned long)buf_len);
  return true;
}

bool U8g2RLCDRenderer::hydrate()
{
  ESP_LOGI(TAG, "Hydrating display state");
  
  FILE *fp = fopen("/fs/front_buffer.z", "r");
  if (!fp)
  {
    ESP_LOGW(TAG, "No saved state found");
    return false;
  }
  
  // Read buffer size header
  uint16_t buf_len = 0;
  if (fread(&buf_len, sizeof(uint16_t), 1, fp) != 1 || buf_len == 0)
  {
    ESP_LOGE(TAG, "Invalid buffer header");
    fclose(fp);
    return false;
  }
  
  uint8_t *buf = u8g2_GetBufferPtr(&m_u8g2);
  uint16_t expected_len = u8g2_GetBufferTileHeight(&m_u8g2) * u8g2_GetBufferTileWidth(&m_u8g2) * 8;
  
  if (buf_len > expected_len)
  {
    ESP_LOGE(TAG, "Buffer size mismatch: %d > %d", buf_len, expected_len);
    fclose(fp);
    return false;
  }
  
  // Read buffer data
  size_t read = fread(buf, 1, buf_len, fp);
  fclose(fp);
  
  if (read != buf_len)
  {
    ESP_LOGE(TAG, "Failed to read buffer: %lu bytes", (unsigned long)read);
    return false;
  }
  
  ESP_LOGI(TAG, "Buffer restored: %lu bytes", (unsigned long)buf_len);
  return true;
}

void U8g2RLCDRenderer::reset()
{
  ESP_LOGI(TAG, "Resetting display");
  clear_screen();
  flush_display();
}

// U8g2 custom callbacks for ST7305 display
static uint8_t u8x8_byte_custom(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  (void)u8x8;

  switch (msg)
  {
  case U8X8_MSG_BYTE_START_TRANSFER:
    gpio_set_level(RLCD_PIN_CS, 0);
    return 1;
  case U8X8_MSG_BYTE_END_TRANSFER:
    gpio_set_level(RLCD_PIN_CS, 1);
    return 1;
  case U8X8_MSG_BYTE_SET_DC:
    gpio_set_level(RLCD_PIN_DC, arg_int ? 1 : 0);
    return 1;
  case U8X8_MSG_BYTE_SEND:
  {
    spi_transaction_t t = {};
    t.length = arg_int * 8;
    t.tx_buffer = arg_ptr;
    spi_device_polling_transmit(s_spi, &t);
    return 1;
  }
  default:
    break;
  }

  return 1;
}

static uint8_t u8x8_d_st7305_custom(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  (void)arg_int;
  
  switch (msg)
  {
  case U8X8_MSG_DISPLAY_SETUP_MEMORY:
    u8x8_d_helper_display_setup_memory(u8x8, &st7305_display_info);
    break;
    
  case U8X8_MSG_DISPLAY_INIT:
    // Reset sequence
    gpio_set_level(RLCD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(RLCD_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(RLCD_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // ST7305 initialization commands
    {
      const uint8_t d6[] = {0x17, 0x02};
      const uint8_t d1[] = {0x01};
      const uint8_t c0[] = {0x11, 0x04};
      const uint8_t c1[] = {0x69, 0x69, 0x69, 0x69};
      const uint8_t c2[] = {0x19, 0x19, 0x19, 0x19};
      const uint8_t c4[] = {0x4B, 0x4B, 0x4B, 0x4B};
      const uint8_t d8[] = {0x80, 0xE9};
      const uint8_t b2[] = {0x02};
      const uint8_t b3[] = {0xE5, 0xF6, 0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45};
      const uint8_t b4[] = {0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45};
      const uint8_t g_timing[] = {0x32, 0x03, 0x1F};
      const uint8_t b7[] = {0x13};
      const uint8_t b0[] = {0x64};
      const uint8_t c9[] = {0x00};
      const uint8_t m36[] = {0x48};
      const uint8_t m3a[] = {0x11};
      const uint8_t b9[] = {0x20};
      const uint8_t b8[] = {0x29};
      const uint8_t win_a[] = {0x12, 0x2A};
      const uint8_t win_b[] = {0x00, 0xC7};
      const uint8_t m35[] = {0x00};
      const uint8_t d0[] = {0xFF};
      
      // Helper lambda for sending command + data
      auto send_cmd_data = [](uint8_t cmd, const uint8_t *data, size_t len) {
        gpio_set_level(RLCD_PIN_CS, 0);
        gpio_set_level(RLCD_PIN_DC, 0);
        spi_transaction_t t = {};
        t.length = 8;
        t.tx_buffer = &cmd;
        spi_device_polling_transmit(s_spi, &t);
        if (len > 0)
        {
          gpio_set_level(RLCD_PIN_DC, 1);
          t.length = len * 8;
          t.tx_buffer = data;
          spi_device_polling_transmit(s_spi, &t);
        }
        gpio_set_level(RLCD_PIN_CS, 1);
      };
      
      send_cmd_data(0xD6, d6, sizeof(d6));
      send_cmd_data(0xD1, d1, sizeof(d1));
      send_cmd_data(0xC0, c0, sizeof(c0));
      send_cmd_data(0xC1, c1, sizeof(c1));
      send_cmd_data(0xC2, c2, sizeof(c2));
      send_cmd_data(0xC4, c4, sizeof(c4));
      send_cmd_data(0xC5, c2, sizeof(c2));
      send_cmd_data(0xD8, d8, sizeof(d8));
      send_cmd_data(0xB2, b2, sizeof(b2));
      send_cmd_data(0xB3, b3, sizeof(b3));
      send_cmd_data(0xB4, b4, sizeof(b4));
      send_cmd_data(0x62, g_timing, sizeof(g_timing));
      send_cmd_data(0xB7, b7, sizeof(b7));
      send_cmd_data(0xB0, b0, sizeof(b0));
      
      // Sleep out
      send_cmd_data(0x11, nullptr, 0);
      vTaskDelay(pdMS_TO_TICKS(120));
      
      send_cmd_data(0xC9, c9, sizeof(c9));
      send_cmd_data(0x36, m36, sizeof(m36));
      send_cmd_data(0x3A, m3a, sizeof(m3a));
      send_cmd_data(0xB9, b9, sizeof(b9));
      send_cmd_data(0xB8, b8, sizeof(b8));
      
      // Display inversion off (black text on white background)
      // send_cmd_data(0x21, nullptr, 0);
      
      send_cmd_data(0x2A, win_a, sizeof(win_a));
      send_cmd_data(0x2B, win_b, sizeof(win_b));
      send_cmd_data(0x35, m35, sizeof(m35));
      send_cmd_data(0xD0, d0, sizeof(d0));
      
      // Idle mode off
      send_cmd_data(0x38, nullptr, 0);
      // Display on
      send_cmd_data(0x29, nullptr, 0);
    }
    break;
    
  case U8X8_MSG_DISPLAY_DRAW_TILE:
  {
    u8x8_tile_t *tile = (u8x8_tile_t *)arg_ptr;
    uint8_t cnt = tile->cnt;
    uint8_t y_pos = tile->y_pos;
    uint8_t x_pos = tile->x_pos;
    
    int first_col = x_pos * 8;
    int last_col = (x_pos + cnt) * 8 - 1;
    if (last_col >= 300) last_col = 299;
    
    int addr_start = 0x12 + first_col / 12;
    int addr_end = 0x12 + last_col / 12;
    int send_start = (addr_start - 0x12) * 3;
    int send_cnt = (addr_end - addr_start + 1) * 3;
    
    int addr_first_col = (addr_start - 0x12) * 12;
    int addr_last_col = (addr_end - 0x12) * 12 + 11;
    if (addr_last_col >= 300) addr_last_col = 299;
    
    uint8_t *row_base = tile->tile_ptr - ((uint16_t)x_pos * 8U);
    
    uint8_t col_bounds[] = {(uint8_t)(0x3C - addr_end), (uint8_t)(0x3C - addr_start)};
    uint8_t row_bounds[] = {(uint8_t)(y_pos * 4), (uint8_t)(y_pos * 4 + 3)};
    
    static const uint8_t st_lut[4][4] = {
      {0x00, 0x80, 0x40, 0xC0},
      {0x00, 0x20, 0x10, 0x30},
      {0x00, 0x08, 0x04, 0x0C},
      {0x00, 0x02, 0x01, 0x03},
    };
    
    uint8_t all_rows[300] = {0};
    for (int sr = 0; sr < 4; sr++)
    {
      int shift = sr * 2;
      int base_off = sr * send_cnt;
      int idx = base_off + (addr_first_col >> 2) - send_start;
      
      for (int col = addr_first_col; col <= addr_last_col; col += 4, idx++)
      {
        all_rows[idx] = st_lut[0][(row_base[col] >> shift) & 3]
                       | st_lut[1][(row_base[col + 1] >> shift) & 3]
                       | st_lut[2][(row_base[col + 2] >> shift) & 3]
                       | st_lut[3][(row_base[col + 3] >> shift) & 3];
      }
    }
    
    // Send column address set
    gpio_set_level(RLCD_PIN_CS, 0);
    gpio_set_level(RLCD_PIN_DC, 0);
    {
      spi_transaction_t t = {};
      t.length = 8;
      uint8_t cmd = 0x2A;
      t.tx_buffer = &cmd;
      spi_device_polling_transmit(s_spi, &t);
    }
    gpio_set_level(RLCD_PIN_DC, 1);
    {
      spi_transaction_t t = {};
      t.length = 16;
      t.tx_buffer = col_bounds;
      spi_device_polling_transmit(s_spi, &t);
    }
    
    // Send row address set
    gpio_set_level(RLCD_PIN_DC, 0);
    {
      spi_transaction_t t = {};
      t.length = 8;
      uint8_t cmd = 0x2B;
      t.tx_buffer = &cmd;
      spi_device_polling_transmit(s_spi, &t);
    }
    gpio_set_level(RLCD_PIN_DC, 1);
    {
      spi_transaction_t t = {};
      t.length = 16;
      t.tx_buffer = row_bounds;
      spi_device_polling_transmit(s_spi, &t);
    }
    
    // Send pixel data
    gpio_set_level(RLCD_PIN_DC, 0);
    {
      spi_transaction_t t = {};
      t.length = 8;
      uint8_t cmd = 0x2C;
      t.tx_buffer = &cmd;
      spi_device_polling_transmit(s_spi, &t);
    }
    gpio_set_level(RLCD_PIN_DC, 1);
    {
      spi_transaction_t t = {};
      t.length = send_cnt * 4 * 8;
      t.tx_buffer = all_rows;
      spi_device_polling_transmit(s_spi, &t);
    }
    gpio_set_level(RLCD_PIN_CS, 1);
    break;
  }
    
  default:
    return 0;
  }
  
  return 1;
}
