#include "RLCD_Board.h"
#include "Renderer/RLCDRenderer.h"
#include <esp_log.h>
#include <esp_heap_caps.h>

static const char *TAG = "RLCD_Board";

// SPI configuration for RLCD
#define RLCD_SPI_HOST SPI3_HOST
#define RLCD_SPI_FREQ_HZ (10 * 1000 * 1000)  // 10MHz

// RLCD display commands
#define RLCD_CMD睡眠    0x10
#define RLCD_CMD唤醒    0x11
#define RLCD_CMD显示    0x29
#define RLCD_CMD列地址  0x2A
#define RLCD_CMD行地址  0x2B
#define RLCD_CMD写内存  0x2C
#define RLCD_CMD像素格式 0x3A
#define RLCD_CMD方向    0x36

// Static SPI device for display commands
static spi_device_handle_t s_spi = nullptr;

RLCD_Board::RLCD_Board() : m_renderer(nullptr)
{
  init_spi();
}

RLCD_Board::~RLCD_Board()
{
}

void RLCD_Board::init_spi()
{
  ESP_LOGI(TAG, "Initializing SPI for RLCD");
  
  spi_bus_config_t bus_cfg = {};
  bus_cfg.mosi_io_num = GPIO_NUM_11;
  bus_cfg.sclk_io_num = GPIO_NUM_12;
  bus_cfg.miso_io_num = GPIO_NUM_13;
  bus_cfg.quadwp_io_num = -1;
  bus_cfg.quadhd_io_num = -1;
  bus_cfg.max_transfer_sz = RLCD_WIDTH * RLCD_HEIGHT / 8;
  
  esp_err_t ret = spi_bus_initialize(RLCD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
  ESP_ERROR_CHECK(ret);
  
  spi_device_interface_config_t dev_cfg = {};
  dev_cfg.clock_speed_hz = RLCD_SPI_FREQ_HZ;
  dev_cfg.mode = 0;
  dev_cfg.spics_io_num = GPIO_NUM_40;
  dev_cfg.queue_size = 10;
  
  ret = spi_bus_add_device(RLCD_SPI_HOST, &dev_cfg, &s_spi);
  ESP_ERROR_CHECK(ret);
  
  // Configure DC and RST pins
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL << GPIO_NUM_5) | (1ULL << GPIO_NUM_41);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);
  
  ESP_LOGI(TAG, "SPI initialized");
}

void RLCD_Board::send_command(uint8_t cmd)
{
  gpio_set_level(GPIO_NUM_5, 0);  // DC low = command
  
  spi_transaction_t t = {};
  t.length = 8;
  t.tx_buffer = &cmd;
  spi_device_polling_transmit(s_spi, &t);
}

void RLCD_Board::send_data(uint8_t data)
{
  gpio_set_level(GPIO_NUM_5, 1);  // DC high = data
  
  spi_transaction_t t = {};
  t.length = 8;
  t.tx_buffer = &data;
  spi_device_polling_transmit(s_spi, &t);
}

void RLCD_Board::send_data_bulk(uint8_t *data, int len)
{
  gpio_set_level(GPIO_NUM_5, 1);  // DC high = data
  
  spi_transaction_t t = {};
  t.length = len * 8;
  t.tx_buffer = data;
  spi_device_polling_transmit(s_spi, &t);
}

void RLCD_Board::init_display()
{
  ESP_LOGI(TAG, "Initializing RLCD display");
  
  // Reset sequence
  gpio_set_level(GPIO_NUM_41, 1);
  vTaskDelay(pdMS_TO_TICKS(50));
  gpio_set_level(GPIO_NUM_41, 0);
  vTaskDelay(pdMS_TO_TICKS(20));
  gpio_set_level(GPIO_NUM_41, 1);
  vTaskDelay(pdMS_TO_TICKS(50));
  
  // Initialize display using RLCD commands
  send_command(0xD6);  // NVM Load Control
  send_data(0x17);
  send_data(0x02);
  
  send_command(0xD1);  // Booster Enable
  send_data(0x01);
  
  send_command(0xC0);  // Gate Voltage Control
  send_data(0x11);
  send_data(0x04);
  
  send_command(0xC1);  // VSHP Setting
  send_data(0x69);
  send_data(0x69);
  send_data(0x69);
  send_data(0x69);
  
  send_command(0xC2);
  send_data(0x19);
  send_data(0x19);
  send_data(0x19);
  send_data(0x19);
  
  send_command(0xC4);
  send_data(0x4B);
  send_data(0x4B);
  send_data(0x4B);
  send_data(0x4B);
  
  send_command(0xC5);
  send_data(0x19);
  send_data(0x19);
  send_data(0x19);
  send_data(0x19);
  
  send_command(0xD8);
  send_data(0x80);
  send_data(0xE9);
  
  send_command(0xB2);
  send_data(0x02);
  
  send_command(0xB3);
  send_data(0xE5);
  send_data(0xF6);
  send_data(0x05);
  send_data(0x46);
  send_data(0x77);
  send_data(0x77);
  send_data(0x77);
  send_data(0x77);
  send_data(0x76);
  send_data(0x45);
  
  send_command(0xB4);
  send_data(0x05);
  send_data(0x46);
  send_data(0x77);
  send_data(0x77);
  send_data(0x77);
  send_data(0x77);
  send_data(0x76);
  send_data(0x45);
  
  send_command(0x62);
  send_data(0x32);
  send_data(0x03);
  send_data(0x1F);
  
  send_command(0xB7);
  send_data(0x13);
  
  send_command(0xB0);
  send_data(0x64);
  
  send_command(0x11);  // Sleep Out
  vTaskDelay(pdMS_TO_TICKS(200));
  
  send_command(0xC9);
  send_data(0x00);
  
  send_command(0x36);  // Memory Access Control
  send_data(0x48);  // Portrait, top-left origin
  
  send_command(0x3A);  // Pixel Format
  send_data(0x11);  // 16-bit (we'll use 1-bit packed)
  
  send_command(0xB9);
  send_data(0x20);
  
  send_command(0xB8);
  send_data(0x29);
  
  send_command(0x21);  // Display Inversion On
  
  // Set column address
  send_command(0x2A);
  send_data(0x00);
  send_data(0x00);
  send_data(0x01);  // 300-1 = 299 = 0x012B
  send_data(0x2B);
  
  // Set row address
  send_command(0x2B);
  send_data(0x00);
  send_data(0x00);
  send_data(0x01);  // 400-1 = 399 = 0x018F
  send_data(0x8F);
  
  send_command(0x35);  // Tearing Effect Line On
  send_data(0x00);
  
  send_command(0xD0);
  send_data(0xFF);
  
  send_command(0x38);  // Idle Mode Off
  send_command(0x29);  // Display On
  
  ESP_LOGI(TAG, "RLCD display initialized");
}

void RLCD_Board::power_up()
{
  ESP_LOGI(TAG, "Powering up RLCD board");
  init_display();
}

void RLCD_Board::prepare_to_sleep()
{
  ESP_LOGI(TAG, "Preparing RLCD board for sleep");
  // Turn off display
  send_command(0x28);  // Display Off
  send_command(0x10);  // Sleep In
}

Renderer *RLCD_Board::get_renderer()
{
  if (!m_renderer)
  {
    m_renderer = new RLCDRenderer(RLCD_WIDTH, RLCD_HEIGHT);
  }
  return m_renderer;
}

ButtonControls *RLCD_Board::get_button_controls(xQueueHandle ui_queue)
{
  // TODO: Implement button controls
  return nullptr;
}
