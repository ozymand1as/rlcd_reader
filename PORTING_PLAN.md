# EPUB Reader Port to Waveshare ESP32-S3-RLCD-4.2

## Executive Summary

Port the `diy-esp32-epub-reader` (EPD-based) to the Waveshare ESP32-S3-RLCD-4.2 (Reflective LCD).

**Key challenge**: The original reader uses EPD (e-paper) technology with the EPDIY library. The RLCD is a reflective LCD that behaves like a standard LCD but without backlight. The rendering approach differs significantly:
- EPD: 4-bit grayscale framebuffer (16 shades), slow refresh (1-2s), ghosting management
- RLCD: 1-bit monochrome framebuffer (black/white only), fast refresh (~60ms), no ghosting

---

## Architecture Comparison

| Aspect | Original (EPD) | Target (RLCD) |
|--------|----------------|---------------|
| Display Type | E-paper (ED047TC2) | Reflective LCD (SPI) |
| Resolution | 400x300 | 300x400 (portrait) |
| Color Depth | 4-bit grayscale | 1-bit monochrome |
| Refresh Speed | 1-2 seconds | ~60ms |
| Interface | Parallel (EPD) | SPI |
| MCU | ESP32 | ESP32-S3 |
| RAM | 4MB PSRAM | 8MB PSRAM |
| Flash | - | 16MB |

---

## Implementation Plan

### Phase 1: Project Setup (Day 1)

#### 1.1 Create New PlatformIO Project
```bash
# Create new project structure
mkdir -p rlcd_reader/src/boards
mkdir -p rlcd_reader/lib/Epub
mkdir -p rlcd_reader/lib/Fonts
```

#### 1.2 Configure `platformio.ini`
```ini
[env:rlcd_42]
platform = espressif32
board = esp32-s3-devkitc-1
framework = espidf
monitor_speed = 115200

build_flags =
  -DBOARD_TYPE_RLCD_42
  -DRLCD_WIDTH=300
  -DRLCD_HEIGHT=400
  -DBOARD_HAS_PSRAM
  -DSD_CARD_PIN_NUM_MISO=GPIO_NUM_13
  -DSD_CARD_PIN_NUM_MOSI=GPIO_NUM_11
  -DSD_CARD_PIN_NUM_CLK=GPIO_NUM_12
  -DSD_CARD_PIN_NUM_CS=GPIO_NUM_10
  -DBUTTON_UP_GPIO=GPIO_NUM_0
  -DBUTTON_DOWN_GPIO=GPIO_NUM_21
  -DBUTTON_SELECT_GPIO=GPIO_NUM_47
  
lib_deps =
  https://github.com/leethomason/tinyxml2.git
  
board_build.partitions = partitions.csv
```

### Phase 2: Display Driver (Day 1-2)

#### 2.1 Create `RLCDRenderer` Class
**New file**: `lib/Epub/Renderer/RLCDRenderer.h`

```cpp
#pragma once
#include "Renderer.h"
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>

class RLCDRenderer : public Renderer {
private:
  uint8_t* m_frame_buffer;
  int m_width;
  int m_height;
  spi_device_handle_t m_spi;
  
  // Pixel packing for 1-bit monochrome
  // Each byte = 8 pixels (1bpp)
  // Layout: column-major for RLCD hardware
  
  void set_pixel(int x, int y, bool black);
  bool get_pixel(int x, int y);
  
public:
  RLCDRenderer(int width, int height);
  ~RLCDRenderer();
  
  // Renderer interface
  void draw_pixel(int x, int y, uint8_t color) override;
  void draw_text(int x, int y, const char* text, 
                 bool bold = false, bool italic = false) override;
  void draw_rect(int x, int y, int w, int h, uint8_t color) override;
  void fill_rect(int x, int y, int w, int h, uint8_t color) override;
  void clear_screen() override;
  void flush_display() override;
  
  int get_page_width() override;
  int get_page_height() override;
  int get_space_width() override;
  int get_line_height() override;
};
```

#### 2.2 Display Initialization Sequence
**Reference**: `ESP32-S3-RLCD-4.2/02_Example/Arduino/08_LVGL_V8_Test/display_bsp.cpp`

Key differences from EPD:
- No waveform tables needed
- No temperature compensation
- Direct SPI transfer (10MHz)
- Simple command set (0x11, 0x29, 0x2A, 0x2B, 0x2C)

#### 2.3 Framebuffer Layout
```
RLCD 300x400 portrait:
- Total pixels: 120,000
- Buffer size: 15,000 bytes (120,000 / 8)
- Memory: Use PSRAM (heap_caps_malloc with MALLOC_CAP_SPIRAM)

Pixel packing (column-major, rotated):
Byte 0: [x=0,y=7] [x=0,y=6] ... [x=0,y=0]
Byte 1: [x=1,y=7] [x=1,y=6] ... [x=1,y=0]
...
```

### Phase 3: Font Rendering (Day 2)

#### 3.1 Font Conversion
The original uses EPD fonts (EpdFont). Need to:
1. Convert fonts to 1-bit format (no anti-aliasing)
2. Create font rendering functions that work with 1bpp buffer

**Option A**: Use existing font library (e.g., Adafruit GFX fonts)
**Option B**: Convert EPD fonts to 1-bit and create compatible renderer

**Recommended**: Use Adafruit GFX for simplicity:
```cpp
#include <Adafruit_GFX.h>
#include <Fonts/FreeMono9pt7b.h>
```

#### 3.2 Text Rendering
```cpp
void RLCDRenderer::draw_text(int x, int y, const char* text, 
                             bool bold, bool italic) {
  // Set font based on bold/italic
  // Render to framebuffer using Adafruit GFX or custom renderer
  // Handle word wrapping
}
```

### Phase 4: UI Controls (Day 2-3)

#### 4.1 Button Controls
**New file**: `src/boards/controls/RLCDButtonControls.h`

```cpp
class RLCDButtonControls : public ButtonControls {
private:
  gpio_num_t m_up_pin;
  gpio_num_t m_down_pin;
  gpio_num_t m_select_pin;
  QueueHandle_t m_ui_queue;
  
public:
  RLCDButtonControls(gpio_num_t up, gpio_num_t down, 
                     gpio_num_t select, QueueHandle_t queue);
  
  void setup() override;
  void poll() override;
  bool did_wake_from_deep_sleep() override;
};
```

#### 4.2 Button Mapping
- **UP**: Page up / Navigate up
- **DOWN**: Page down / Navigate down
- **SELECT**: Select item / Back to list

### Phase 5: Board Integration (Day 3)

#### 5.1 Create `RLCD_Board` Class
**New file**: `src/boards/RLCD_Board.h`

```cpp
class RLCD_Board : public Board {
public:
  void power_up() override;
  void prepare_to_sleep() override;
  Renderer* get_renderer() override;
  ButtonControls* get_button_controls(QueueHandle_t queue) override;
};
```

#### 5.2 Update Board Factory
**Edit**: `src/boards/Board.cpp`

```cpp
Board* Board::factory() {
  #ifdef BOARD_TYPE_LILIGO_T5_47
    return new Lilygo_t5_47();
  #endif
  #ifdef BOARD_TYPE_EPDIY
    return new Epdiy();
  #endif
  #ifdef BOARD_TYPE_M5_PAPER
    return new M5Paper();
  #endif
  #ifdef BOARD_TYPE_RLCD_42
    return new RLCD_Board();
  #endif
}
```

### Phase 6: EPUB Rendering Adaptation (Day 3-4)

#### 6.1 Grayscale to Monochrome
The original renders 16 grayscale levels. For RLCD:
```cpp
uint8_t RLCDRenderer::grayscale_to_mono(uint8_t gray) {
  // Dithering or threshold
  return (gray < 128) ? 0 : 255;  // Simple threshold
  // Or use Floyd-Steinberg dithering for better quality
}
```

#### 6.2 Image Rendering
Original uses PNG/JPEG with grayscale. For RLCD:
- Convert images to 1-bit during rendering
- Use thresholding or dithering
- May need to resize images for 1-bit display

### Phase 7: Power Management (Day 4)

#### 7.1 Sleep Mode
RLCD doesn't have deep sleep like EPD. Options:
1. Turn off display (LCD power down)
2. Enter light sleep
3. Keep last image on screen (LCD retains state)

#### 7.2 Battery Monitoring
Board has 18650 battery holder. Implement ADC battery monitoring.

---

## File Structure

```
rlcd_reader/
├── platformio.ini
├── partitions.csv
├── src/
│   ├── main.cpp
│   └── boards/
│       ├── Board.h
│       ├── Board.cpp
│       ├── RLCD_Board.h
│       ├── RLCD_Board.cpp
│       └── controls/
│           ├── RLCDButtonControls.h
│           └── RLCDButtonControls.cpp
└── lib/
    ├── Epub/
    │   ├── Renderer/
    │   │   ├── Renderer.h
    │   │   ├── RLCDRenderer.h
    │   │   └── RLCDRenderer.cpp
    │   ├── EpubList/
    │   ├── RubbishHtmlParser/
    │   └── ZipFile/
    ├── Fonts/
    │   └── (converted 1-bit fonts)
    └── miniz-2.2.0/
```

---

## Key Technical Decisions

### 1. Rendering Approach
**Decision**: Direct framebuffer manipulation (not LVGL)
**Rationale**: 
- Simpler integration with existing epub reader code
- Less overhead than LVGL for text-heavy UI
- Full control over rendering pipeline

### 2. Font Format
**Decision**: Use Adafruit GFX compatible fonts
**Rationale**:
- Well-tested 1-bit font rendering
- Easy to find/generate fonts
- Good performance on ESP32-S3

### 3. Image Handling
**Decision**: Convert to 1-bit at render time
**Rationale**:
- Keep original PNG/JPEG support
- Use thresholding for simple conversion
- Consider dithering for better quality

### 4. Memory Management
**Decision**: Use PSRAM for framebuffer
**Rationale**:
- 15KB buffer fits easily in 8MB PSRAM
- Leaves internal RAM for stack/heap
- PSRAM is slower but sufficient for LCD refresh rate

---

## Testing Strategy

### Unit Tests
1. Pixel rendering (set/get pixel)
2. Text rendering (font metrics, wrapping)
3. Rect/triangle/circle drawing

### Integration Tests
1. Full screen refresh cycle
2. Button input handling
3. EPUB loading and rendering
4. Deep sleep/wake cycle

### Visual Tests
1. Compare rendering with original EPD version
2. Check text readability
3. Verify image quality (1-bit conversion)

---

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Font rendering quality | Medium | Use established font library, test early |
| Image conversion quality | Medium | Implement dithering option |
| SPI performance | Low | 10MHz SPI is sufficient for 300x400 |
| Memory pressure | Low | PSRAM provides ample space |
| Button debounce | Low | Use hardware debounce or timer |

---

## Timeline

- **Day 1**: Project setup, display driver basics
- **Day 2**: Font rendering, framebuffer management
- **Day 3**: Button controls, board integration
- **Day 4**: EPUB rendering, power management
- **Day 5**: Testing, bug fixes, documentation

**Total estimated time**: 5-7 days for working prototype

---

## Next Steps

1. Create PlatformIO project with correct configuration
2. Port display driver from RLCD examples
3. Implement RLCDRenderer class
4. Test basic rendering (text, shapes)
5. Integrate with existing EPUB library
6. Add button controls
7. Test with real EPUB files
8. Optimize performance
9. Add power management
10. Document usage
