# Quick Start Guide

## Key Differences: EPD vs RLCD

| | EPD (Original) | RLCD (Target) |
|--|----------------|---------------|
| **Display** | E-paper (slow, grayscale) | Reflective LCD (fast, 1-bit) |
| **Refresh** | 1-2 seconds | ~60ms |
| **Colors** | 16 grayscale | Black/White only |
| **Power** | Zero when static | Always on |

## What Needs to Change

### 1. Display Driver
- **Remove**: EPDIY library dependency
- **Add**: SPI-based RLCD driver (from `ESP32-S3-RLCD-4.2/02_Example/Arduino/08_LVGL_V8_Test/display_bsp.cpp`)

### 2. Renderer
- **Original**: `EpdiyRenderer` (16 grayscale, 4-bit buffer)
- **New**: `RLCDRenderer` (1-bit, 1bpp buffer)

### 3. Fonts
- **Original**: EPD fonts (anti-aliased)
- **New**: 1-bit fonts (Adafruit GFX or similar)

### 4. Images
- **Original**: Grayscale PNG/JPEG
- **New**: Convert to 1-bit at render time

## Files to Create

1. `lib/Epub/Renderer/RLCDRenderer.h` - Main renderer
2. `src/boards/RLCD_Board.h` - Board abstraction
3. `src/boards/RLCD_Board.cpp` - Board implementation
4. `src/boards/controls/RLCDButtonControls.h` - Button handling

## Files to Modify

1. `platformio.ini` - Add RLCD environment
2. `src/boards/Board.cpp` - Add RLCD to factory

## Testing Commands

```bash
# Build for RLCD
pio run -e rlcd_42

# Monitor serial output
pio device monitor

# Upload firmware
pio run -e rlcd_42 --target upload
```

## Display Pin Mapping (RLCD 4.2)

| Function | GPIO |
|----------|------|
| MOSI | 11 |
| SCK | 12 |
| DC | 5 |
| CS | 40 |
| RST | 41 |
| MISO | 13 |

## Button Pin Mapping (estimated)

| Function | GPIO |
|----------|------|
| UP | 0 |
| DOWN | 21 |
| SELECT | 47 |

*Note: Verify actual button pins from board schematic*

## Memory Usage

- **Frame buffer**: 15KB (300×400 ÷ 8)
- **PSRAM available**: 8MB
- **Internal RAM**: ~512KB (ESP32-S3)

## Performance Expectations

- **Text rendering**: ~10ms per page
- **Full screen refresh**: ~60ms
- **EPUB page turn**: ~100ms total

## Common Issues

### 1. Display Not Updating
- Check SPI pin configuration
- Verify power supply
- Check reset timing

### 2. Garbled Display
- Verify pixel packing order (column-major)
- Check buffer size (must be 15,000 bytes)

### 3. Slow Performance
- Use PSRAM for framebuffer
- Optimize SPI clock (10MHz recommended)

## Resources

- [Original EPUB Reader](https://github.com/atomic14/diy-esp32-epub-reader)
- [RLCD Examples](https://github.com/waveshareteam/ESP32-S3-RLCD-4.2)
- [ESP-IDF SPI Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/spi_master.html)
