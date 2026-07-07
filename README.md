# RLCD EPUB Reader

> This project was built (modified from source) using MiMo AI

A port of the [diy-esp32-epub-reader](https://github.com/atomic14/diy-esp32-epub-reader) to the Waveshare ESP32-S3-RLCD-4.2 development board.

## Features

- Read EPUB files from SD card
- Navigate with 4 buttons (BOOT=back, KEY=select, FWD=next, PREV=previous)
- Battery level monitoring
- Deep sleep with state restoration
- Cyrillic font support (Russian, Ukrainian, etc.)
- Table of contents navigation

## Hardware

- **Board**: Waveshare ESP32-S3-RLCD-4.2
- **Display**: 4.2" Reflective LCD (300x400, 1-bit monochrome, ST7305)
- **MCU**: ESP32-S3-WROOM-1-N16R8 (16MB Flash, 8MB PSRAM)
- **Storage**: TF card slot for EPUB files (FAT32)
- **Power**: 18650 battery holder, USB-C charging
- **Sensors**: SHTC3 (temp/humidity), PCF85063 (RTC)

## Pin Configuration

### Display (SPI)
| Function | GPIO |
|----------|------|
| MOSI | 11 |
| SCK | 12 |
| DC | 5 |
| CS | 40 |
| RST | 41 |
| MISO | 13 |

### Buttons (Active Low, Internal Pull-up)
| Function | GPIO | Action |
|----------|------|--------|
| BOOT | 0 | Back / Cancel |
| KEY | 18 | Single=Down, Double=Up, Long=Select |
| FWD | 20 | Next page / Scroll down |
| PREV | 17 | Previous page / Scroll up |
| PWR | N/A | Hardware power (not GPIO) |

### SD Card (SPI)
| Function | GPIO |
|----------|------|
| MISO | 13 |
| MOSI | 11 |
| SCK | 12 |
| CS | 10 |

## Deep Sleep

The reader automatically enters deep sleep after 2 minutes of inactivity. State is preserved to survive sleep/wake cycles.

### How It Works

1. **State Storage**: UI state (selected book, page, section) is stored in RTC memory (`RTC_DATA_ATTR`), which survives deep sleep.

2. **Display Buffer**: The current screen content is saved to SD card (`/fs/front_buffer.z`) before sleep.

3. **Wake Sources**: Any button press (BOOT, KEY, FWD, PREV) wakes the device via GPIO interrupt.

4. **Restoration**: On wake, the device:
   - Restores UI state from RTC memory
   - Loads display buffer from SD card
   - Redraws the last screen instantly

### Sleep Behavior

```
┌─────────────────────────────────────────────────────────┐
│                                                         │
│  Idle 2 min ──→ Save state ──→ Save display ──→ Sleep   │
│                                          │              │
│                                          ▼              │
│                              ┌───────────────────┐      │
│                              │  Deep Sleep       │      │
│                              │  (10 μA)          │      │
│                              └───────────────────┘      │
│                                          │              │
│                                          ▼              │
│                              Button press (GPIO wake)   │
│                                          │              │
│                                          ▼              │
│                              Load display ──→ Resume    │
└─────────────────────────────────────────────────────────┘
```

### Battery Life

| Mode | Current Draw |
|------|-------------|
| Active (display on) | ~50 mA |
| Deep sleep | ~10 μA |
| Display retention (RLCD) | 0 mA (image persists) |

**Note**: Unlike e-paper, the RLCD display **retains the last image** without power. After deep sleep, the display still shows the last page even though the MCU is off. On wake, the buffer is restored to maintain consistency.

## Navigation

| Button | Short Press | Double Press | Long Press |
|--------|-------------|--------------|------------|
| BOOT | Previous page | - | Back to list |
| KEY | Next page | Previous page | Select/Confirm |
| FWD (GP20) | Next page | - | Select/Confirm |
| PREV (GP17) | Previous page | - | Back to list |

### UI Flow

```
┌──────────────────┐     KEY      ┌──────────────────┐     KEY      ┌──────────────────┐
│                  │ ──────────→  │                  │ ──────────→  │                  │
│   EPUB List      │              │ Table of Contents│              │    Reading       │
│                  │ ←──────────  │                  │ ←──────────  │                  │
└──────────────────┘   BOOT       └──────────────────┘   BOOT       └──────────────────┘
        ▲                                                          │
        │                        BOOT                              │
        └──────────────────────────────────────────────────────────┘
```

## Building

### Prerequisites

```bash
# Install PlatformIO
pip install platformio

# Or via Homebrew
brew install platformio
```

### Build and Flash

```bash
# Build
pio run

# Flash to device
pio run --target upload

# Monitor serial output
pio device monitor
```

## Project Structure

```
rlcd_reader/
├── platformio.ini              # PlatformIO configuration
├── CMakeLists.txt              # ESP-IDF CMake configuration
├── partitions.csv              # Flash partition table
├── HARDWARE_SPEC.md            # Detailed hardware reference
├── src/
│   ├── main.cpp                # Main application & UI state machine
│   ├── u8g2/                   # u8g2 font library (C sources)
│   └── boards/
│       ├── Board.h/cpp         # Board abstraction
│       ├── RLCD_Board.h/cpp    # RLCD board + SPI display init
│       ├── Renderer/
│       │   ├── U8g2RLCDRenderer.*  # u8g2-based renderer
│       │   └── RLCDRenderer.*      # Legacy renderer (unused)
│       ├── controls/
│       │   ├── ButtonControls.h     # Button interface
│       │   └── RLCDButtonControls.* # 4-button + multi-click
│       └── battery/
│           ├── Battery.h        # Battery interface
│           └── ADCBattery.*     # ADC battery monitor
└── lib/
    ├── Epub/
    │   ├── EpubList/
    │   │   ├── Epub.h/cpp           # EPUB parser (OPF, NCX, spine)
    │   │   ├── EpubList.h/cpp       # EPUB file browser
    │   │   ├── EpubReader.h/cpp     # Page-by-page renderer
    │   │   ├── EpubToc.h/cpp        # Table of contents
    │   │   └── State.h              # Persistent state structs
    │   ├── RubbishHtmlParser/
    │   │   ├── RubbishHtmlParser.*  # XHTML parser
    │   │   ├── blocks/              # TextBlock, ImageBlock
    │   │   ├── Page.h               # Page layout
    │   │   └── htmlEntities.*       # HTML entity decoder
    │   ├── ZipFile/
    │   │   └── ZipFile.*            # ZIP extraction (miniz)
    │   └── Renderer/
    │       ├── Renderer.h           # Base renderer interface
    │       └── Renderer.cpp
    ├── miniz-2.2.0/                 # ZIP compression library
    ├── sd_card/
    │   └── SDCard.*                 # SD card SPI driver
    └── u8g2/                        # u8g2 font library
```

## Implementation Status

- [x] Project structure
- [x] PlatformIO configuration
- [x] Board abstraction
- [x] RLCD display driver (ST7305)
- [x] u8g2 font rendering (Cyrillic support)
- [x] Button controls (4-button, dedicated nav + multi-click)
- [x] Battery monitoring (ADC)
- [x] SD card filesystem (SPI, FAT32)
- [x] EPUB parsing (OPF, NCX, spine, manifest)
- [x] HTML parsing (text, bold, images, line breaks)
- [x] EPUB rendering (word wrap, pagination)
- [x] Table of contents navigation
- [x] Deep sleep state restoration
- [ ] Image rendering (PNG/JPEG)
- [ ] Touch support
- [ ] Wi-Fi OTA updates

## Fonts

| Font | Size | Coverage |
|------|------|----------|
| `u8g2_font_7x13_t_cyrillic` | 7x13px | Latin + Cyrillic (default) |
| `u8g2_font_6x13B_t_cyrillic` | 6x13px | Bold |
| `u8g2_font_6x12_t_cyrillic` | 6x12px | Small |
| `u8g2_font_9x15_t_cyrillic` | 9x15px | Large |
| `u8g2_font_10x20_t_cyrillic` | 10x20px | Extra large |

## Differences from Original

| Feature | Original (EPD) | RLCD Port |
|---------|----------------|-----------|
| Display | E-paper | Reflective LCD |
| Colors | 16 grayscale | 1-bit B&W |
| Refresh | 1-2 seconds | ~60ms |
| Interface | Parallel | SPI |
| Fonts | EPD fonts (400KB) | u8g2 (4-7KB each) |
| Buttons | 3 (up/down/select) | 4 (back/select/fwd/prev) |
| Sleep wake | ULP coprocessor | GPIO interrupt |

## Hardware Spec

See [HARDWARE_SPEC.md](HARDWARE_SPEC.md) for detailed hardware documentation including GPIO mapping, SPI configuration, display init sequence, and troubleshooting.

## License

Based on [diy-esp32-epub-reader](https://github.com/atomic14/diy-esp32-epub-reader) by atomic14.

See [LICENSE](LICENSE) for details.
