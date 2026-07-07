# Waveshare ESP32-S3-RLCD-4.2 Hardware Specification

Reference document for porting projects to the Waveshare ESP32-S3-RLCD-4.2 development board.

## Overview

Fully reflective screen AIoT development board based on ESP32-S3. Features a 4.2-inch reflective LCD (no backlight required), audio codec with dual-microphone array, RTC, temperature/humidity sensor, TF card slot, and 18650 battery holder.

- **SKU**: 33298 (with battery) / 33507 (without battery)
- **Documentation**: https://docs.waveshare.com/ESP32-S3-RLCD-4.2
- **Examples**: https://github.com/waveshareteam/ESP32-S3-RLCD-4.2
- **Schematic**: https://files.waveshare.com/wiki/ESP32-S3-RLCD-4.2/ESP32-S3-RLCD-4.2-schematic.pdf

---

## MCU

| Parameter | Value |
|-----------|-------|
| Chip | ESP32-S3-WROOM-1-N16R8 |
| Core | Dual-core Xtensa LX7 @ 240 MHz |
| SRAM | 512 KB |
| ROM | 384 KB |
| Flash | 16 MB (QD SPI) |
| PSRAM | 8 MB (Octal SPI) |
| Wi-Fi | 2.4 GHz 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| USB | USB-OTG (Type-C) |

---

## Display

| Parameter | Value |
|-----------|-------|
| Type | Reflective LCD (RLCD) |
| Controller | ST7305 |
| Size | 4.2 inches |
| Resolution | 300 x 400 pixels |
| Color Depth | 1-bit monochrome (black/white) |
| Backlight | None (reflective, ambient light only) |
| Interface | SPI |
| Active Area | 56.19 x 74.92 mm |
| Pixel Pitch | 0.1873 x 0.1873 mm |

### Display SPI Pins

| Signal | GPIO | Notes |
|--------|------|-------|
| MOSI | 11 | SPI data |
| SCK | 12 | SPI clock |
| DC | 5 | Data/Command select |
| CS | 40 | Chip select (active low) |
| RST | 41 | Hardware reset (active low) |
| MISO | 13 | Not used for display (shared with SD card) |

### Display SPI Configuration

| Parameter | Value |
|-----------|-------|
| SPI Host | SPI3_HOST (HSPI) |
| Clock Speed | 24 MHz |
| SPI Mode | Mode 0 (CPOL=0, CPHA=0) |
| Bit Order | MSB first |

### Display Memory Layout

The ST7305 uses a tile-based addressing scheme:
- Tile width: 38 tiles (38 × 8 = 304 pixels, 300 used)
- Tile height: 50 tiles (50 × 8 = 400 pixels)
- Buffer size: 38 × 8 × 50 = 15,200 bytes (use PSRAM)

### Display Initialization Sequence

```
0xD6 [0x17, 0x02]  // NVM Load Control
0xD1 [0x01]        // Booster Enable
0xC0 [0x11, 0x04]  // Gate Voltage Control
0xC1 [0x69, 0x69, 0x69, 0x69]  // VSHP Setting
0xC2 [0x19, 0x19, 0x19, 0x19]
0xC4 [0x4B, 0x4B, 0x4B, 0x4B]
0xC5 [0x19, 0x19, 0x19, 0x19]
0xD8 [0x80, 0xE9]
0xB2 [0x02]
0xB3 [0xE5, 0xF6, 0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45]
0xB4 [0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45]
0x62 [0x32, 0x03, 0x1F]  // Gate Timing
0xB7 [0x13]
0xB0 [0x64]
0x11              // Sleep Out (wait 120ms)
0xC9 [0x00]
0x36 [0x48]       // Memory Access Control (portrait, top-left)
0x3A [0x11]       // Pixel Format (16-bit interface, 1-bit data)
0xB9 [0x20]
0xB8 [0x29]
0x21              // Display Inversion On
0x2A [0x00, 0x00, 0x01, 0x2B]  // Column Address (0-299)
0x2B [0x00, 0x00, 0x01, 0x8F]  // Row Address (0-399)
0x35 [0x00]       // Tearing Effect Line On
0xD0 [0xFF]
0x38              // Idle Mode Off
0x29              // Display On
```

### Display Pixel Format

The ST7305 uses a packed pixel format where each byte contains 4 pixels (2 bits per pixel for 4-gray levels, but only 1-bit is used for monochrome):

```
Byte layout (portrait mode):
Bit 7: pixel 0, bit 1
Bit 6: pixel 0, bit 0
Bit 5: pixel 1, bit 1
Bit 4: pixel 1, bit 0
Bit 3: pixel 2, bit 1
Bit 2: pixel 2, bit 0
Bit 1: pixel 3, bit 1
Bit 0: pixel 3, bit 0
```

For monochrome: bit 1 = pixel on/off, bit 0 = 0.

---

## Buttons

| Button | GPIO | Active Level | Pull-up | Function |
|--------|------|-------------|---------|----------|
| BOOT | 0 | Low (0) | Internal | Download mode / Back |
| KEY | 18 | Low (0) | Internal | User-configurable |
| PREV | 17 | Low (0) | Internal | Previous page / Back to list |
| FWD | 20 | Low (0) | Internal | Next page / Select |
| PWR | N/A | N/A | N/A | Hardware power management (not GPIO) |

### Button Usage Notes
- **BOOT**: Hold during power-on to enter download mode. Single press = previous page, long press = back to list.
- **KEY**: General-purpose user button. Single click, double click, and long press can be detected.
- **PREV (GP17)**: Single press = previous page, long press = back to list.
- **FWD (GP20)**: Single press = next page, long press = select/confirm.
- **PWR**: Long press = power off, single click = power on. Controls PMIC hardware directly.
- All GPIO buttons use **internal pull-up resistors** (GPIO_PULLUP_ENABLE).
- Buttons are **active-low** (pressed = LOW, released = HIGH).

---

## SD Card (TF Card)

| Signal | GPIO | Notes |
|--------|------|-------|
| MOSI | 11 | Shared with display SPI |
| MISO | 13 | Dedicated to SD card |
| SCK | 12 | Shared with display SPI |
| CS | 10 | Chip select (active low) |

### SD Card Configuration

| Parameter | Value |
|-----------|-------|
| Interface | SPI (SDSPI) |
| Max Frequency | Default (25 MHz) |
| Mount Point | `/fs` |
| File System | FAT32 |
| DMA | Recommended |

### SD Card Notes
- Uses SPI interface (not SDMMC)
- CS pin (GPIO 10) is separate from display CS (GPIO 40)
- MISO (GPIO 13) is dedicated to SD card (display is write-only)
- MOSI and SCK are shared with display SPI bus

---

## Audio

### Audio Codec (ES8311)

| Parameter | Value |
|-----------|-------|
| Type | Low-power audio codec |
| ADC | 24-bit Sigma-Delta |
| DAC | 24-bit Sigma-Delta |
| Interface | I2S |
| Sample Rate | Up to 96 kHz |

### Audio ADC (ES7210)

| Parameter | Value |
|-----------|-------|
| Type | 4-channel ADC with echo cancellation |
| Interface | I2S |
| Microphones | 2x on-board MEMS microphones |
| Features | Noise reduction, echo cancellation |

### Audio Connections

| Function | GPIO | Notes |
|----------|------|-------|
| I2S BCK | 14 | Bit clock |
| I2S WS | 15 | Word select (LRCK) |
| I2S SDO | 16 | Data out (to DAC) |
| I2S SDI | 48 | Data in (from ADC) |
| I2S MCLK | 0 | Master clock (shared with BOOT button) |

### Speaker Interface
- MX1.25 2-pin connector
- Requires external speaker

---

## Sensors

### SHTC3 Temperature/Humidity Sensor

| Parameter | Value |
|-----------|-------|
| Type | Digital temperature and humidity |
| Interface | I2C |
| Address | 0x70 |
| Temperature Range | -40°C to +125°C |
| Humidity Range | 0% to 100% RH |
| Accuracy | ±0.2°C, ±2% RH |

### PCF85063 RTC (Real-Time Clock)

| Parameter | Value |
|-----------|-------|
| Type | CMOS RTC |
| Interface | I2C |
| Address | 0x51 |
| Backup Battery | PH1.0 rechargeable (not included) |
| Features | Alarm, timer, clock output |

---

## I2C Bus

| Parameter | Value |
|-----------|-------|
| SDA | GPIO 8 |
| SCL | GPIO 9 |
| Frequency | 400 kHz (default) |

### I2C Device Addresses

| Device | Address |
|--------|---------|
| SHTC3 | 0x70 |
| PCF85063 | 0x51 |

---

## Power

### Power Sources

| Source | Description |
|--------|-------------|
| USB Type-C | 5V power input, battery charging |
| 18650 Battery | 3.7V Li-ion (holder included on SKU 33298) |
| RTC Battery | PH1.0 rechargeable (separate holder) |

### Power Management

| Parameter | Value |
|-----------|-------|
| Charging IC | TP4056 or similar |
| Charging Current | ~1A (USB) |
| Battery Voltage | 3.0V - 4.2V (nominal 3.7V) |
| Low Battery | ~3.0V cutoff |

### Battery Monitoring

| Parameter | Value |
|-----------|-------|
| ADC Channel | ADC1_CH3 (GPIO 3) |
| Voltage Divider | On-board |
| Measurement Range | 0 - 3.3V (scaled) |
| Resolution | 12-bit (0-4095) |

---

## GPIO Pin Summary

### Used Pins

| GPIO | Function | Direction | Notes |
|------|----------|-----------|-------|
| 0 | BOOT button / I2S MCLK | Input | Active-low, internal pull-up |
| 3 | Battery ADC | Analog input | ADC1_CH3 |
| 5 | LCD DC | Output | Data/Command select |
| 8 | I2C SDA | Bidirectional | Open-drain |
| 9 | I2C SCL | Output | Open-drain |
| 10 | SD Card CS | Output | Active-low |
| 11 | SPI MOSI | Output | Shared: LCD + SD |
| 12 | SPI SCK | Output | Shared: LCD + SD |
| 13 | SD Card MISO | Input | SD card only |
| 14 | I2S BCK | Output | Audio bit clock |
| 15 | I2S WS | Output | Audio word select |
| 16 | I2S SDO | Output | Audio data out |
| 18 | KEY button | Input | Active-low, internal pull-up |
| 17 | PREV button | Input | Active-low, internal pull-up |
| 20 | FWD button | Input | Active-low, internal pull-up |
| 40 | LCD CS | Output | Active-low |
| 41 | LCD RST | Output | Active-low reset |
| 48 | I2S SDI | Input | Audio data in |

### Available for User Use

| GPIO | Notes |
|------|-------|
| 1 | Free |
| 2 | Free |
| 4 | Free |
| 6 | Free |
| 7 | Free |
| 19 | Free |
| 21 | Free |
| 38 | Free |
| 39 | Free |
| 42 | Free |
| 43 | Free (UART TX) |
| 44 | Free (UART RX) |
| 45 | Free |
| 46 | Free |
| 47 | Free |

### Reserved Pins

| GPIO | Notes |
|------|-------|
| 26-32 | Flash/PSRAM (do not use) |
| 33-37 | PSRAM (do not use) |

---

## Expansion Headers

2 × 8-pin female headers (2.54mm pitch) breakout remaining GPIOs.

---

## Build System Configuration

### PlatformIO (ESP-IDF Framework)

```ini
[env:rlcd_42]
platform = espressif32 @ 6.5.0
framework = espidf
board = esp32-s3-devkitc-1

build_flags =
  -DBOARD_TYPE_RLCD_42
  -DRLCD_WIDTH=300
  -DRLCD_HEIGHT=400
  -DBOARD_HAS_PSRAM
  ; Display
  -DRLCD_PIN_MOSI=GPIO_NUM_11
  -DRLCD_PIN_SCK=GPIO_NUM_12
  -DRLCD_PIN_DC=GPIO_NUM_5
  -DRLCD_PIN_CS=GPIO_NUM_40
  -DRLCD_PIN_RST=GPIO_NUM_41
  -DRLCD_PIN_MISO=GPIO_NUM_13
  ; SD Card
  -DSD_CARD_PIN_NUM_MISO=GPIO_NUM_13
  -DSD_CARD_PIN_NUM_MOSI=GPIO_NUM_11
  -DSD_CARD_PIN_NUM_CLK=GPIO_NUM_12
  -DSD_CARD_PIN_NUM_CS=GPIO_NUM_10
  ; Buttons
  -DBUTTON_BACK_GPIO=GPIO_NUM_0
  -DBUTTON_KEY_GPIO=GPIO_NUM_18
  -DBUTTON_PREV_GPIO=GPIO_NUM_17
  -DBUTTON_FWD_GPIO=GPIO_NUM_20
  -DBUTTON_ACTIVE_LEVEL=0
  ; Battery
  -DBATTERY_ADC_CHANNEL=ADC1_CHANNEL_3

board_build.partitions = partitions.csv
board_build.flash_mode = dio
board_build.flash_size = 8MB
```

### Arduino IDE Board Settings

```
Board: ESP32S3 Dev Module
Flash Size: 16MB
PSRAM: OPI PSRAM
Upload Speed: 921600
```

---

## Display Rendering Notes

### Key Differences from E-Paper

| Feature | RLCD (ST7305) | E-Paper (EPD) |
|---------|---------------|---------------|
| Refresh Rate | ~60ms | 1-2 seconds |
| Color | 1-bit monochrome | 4-bit grayscale (16 shades) |
| Ghosting | None | Requires waveforms |
| Power (static) | Always on | Zero |
| Power (refresh) | ~50mA | ~20mA |
| Orientation | Portrait default | Varies |

### Rendering Approach

1. **Framebuffer**: Allocate 15,200 bytes (300 × 400 / 8) in PSRAM
2. **Drawing**: Write pixels to framebuffer using 1bpp packed format
3. **Flush**: Send entire buffer via SPI (0x2C command)
4. **Partial Update**: Can update specific tile regions for faster refresh

### u8g2 Integration

The ST7305 is supported by u8g2 library with custom setup:
- Use `u8x8_d_st7305_custom` display callback
- Tile width: 38, Tile height: 50
- Buffer: Full frame (15,200 bytes) or page-based

---

## Memory Allocation Guide

| Component | RAM Usage | Notes |
|-----------|-----------|-------|
| Display buffer | 15,200 bytes | Use PSRAM |
| EPUB parsing | ~50 KB | Temporary, freed after layout |
| HTML parser | ~20 KB | Per page |
| u8g2 fonts | 4-7 KB each | Selective loading |
| FreeRTOS stack | 32-65 KB | Main task |
| **Total recommended** | **~200 KB** | Fits in 320 KB SRAM + 8 MB PSRAM |

---

## Common Issues and Solutions

### Display Not Updating
- Verify SPI connections (MOSI=11, SCK=12, CS=40, DC=5, RST=41)
- Check reset timing (HIGH 50ms → LOW 20ms → HIGH 50ms)
- Ensure 24MHz SPI clock

### SD Card Not Mounting
- Verify CS pin (GPIO 10, not shared with display)
- Check MISO (GPIO 13) is connected
- Ensure FAT32 format
- Pull-up resistors may be needed on MISO

### Buttons Not Responding
- GPIO 0 (BOOT) may interfere with boot sequence
- GPIO 18 (KEY) should work normally
- Verify active-low configuration
- Check debounce timing (~50ms)

### Memory Issues
- Use PSRAM for display buffer (`MALLOC_CAP_SPIRAM`)
- Free EPUB data after rendering
- Reduce parser buffer size for large books

---

## References

- [ESP32-S3 Datasheet](https://documentation.espressif.com/esp32-s3_datasheet_en.pdf)
- [ST7305 Datasheet](https://files.waveshare.com/wiki/common/ST_7305_V0_2.pdf)
- [ES8311 Datasheet](https://files.waveshare.com/wiki/common/ES8311.DS.pdf)
- [SHTC3 Datasheet](https://files.waveshare.com/wiki/common/SHTC3_Datasheet.pdf)
- [PCF85063 Datasheet](https://files.waveshare.com/wiki/common/Pcf85063atl1118-NdPQpTGE-loeW6GbZ7.pdf)
- [u8g2 Library](https://github.com/olikraus/u8g2)
- [Waveshare RLCD Examples](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2)
