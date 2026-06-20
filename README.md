# RLCD EPUB Reader

A port of the [diy-esp32-epub-reader](https://github.com/atomic14/diy-esp32-epub-reader) to the Waveshare ESP32-S3-RLCD-4.2 development board.

## Features

- Read EPUB files from SD card
- Navigate with physical buttons (UP, DOWN, SELECT)
- Battery level monitoring
- Deep sleep support

## Hardware

- **Board**: Waveshare ESP32-S3-RLCD-4.2
- **Display**: 4.2" Reflective LCD (300x400, 1-bit monochrome)
- **MCU**: ESP32-S3-WROOM-1-N16R8 (16MB Flash, 8MB PSRAM)
- **Storage**: TF card slot for EPUB files
- **Power**: 18650 battery holder

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

### Buttons (Active Low)
| Function | GPIO |
|----------|------|
| UP | 0 |
| DOWN | 21 |
| SELECT | 47 |

### SD Card
| Function | GPIO |
|----------|------|
| MISO | 13 |
| MOSI | 11 |
| SCK | 12 |
| CS | 10 |

## Building

### Prerequisites

- PlatformIO installed
- ESP-IDF toolchain

### Build and Flash

```bash
# Build the project
pio run

# Flash to device
pio run --target upload

# Monitor serial output
pio device monitor
```

### Build for specific environment

```bash
# Build for RLCD 4.2
pio run -e rlcd_42
```

## Project Structure

```
rlcd_reader/
├── platformio.ini          # PlatformIO configuration
├── CMakeLists.txt          # ESP-IDF CMake configuration
├── partitions.csv          # Flash partition table
├── src/
│   ├── main.cpp           # Main application
│   └── boards/
│       ├── Board.h        # Board abstraction
│       ├── Board.cpp      # Board factory
│       ├── RLCD_Board.h   # RLCD board implementation
│       ├── RLCD_Board.cpp
│       ├── Renderer/
│       │   └── RLCDRenderer.*  # Display renderer
│       ├── controls/
│       │   └── RLCDButtonControls.*  # Button handling
│       └── battery/
│           ├── Battery.h      # Battery abstraction
│           └── ADCBattery.*   # ADC battery monitor
└── lib/
    └── Epub/
        └── Renderer/
            ├── Renderer.h     # Base renderer class
            └── Renderer.cpp
```

## Implementation Status

- [x] Project structure
- [x] PlatformIO configuration
- [x] Board abstraction
- [x] RLCD display driver
- [x] 1-bit framebuffer renderer
- [x] Button controls
- [x] Battery monitoring
- [ ] SD card filesystem
- [ ] EPUB parsing
- [ ] EPUB rendering
- [ ] Table of contents
- [ ] Deep sleep state restoration

## Differences from Original

| Feature | Original (EPD) | RLCD Port |
|---------|----------------|-----------|
| Display | E-paper | Reflective LCD |
| Colors | 16 grayscale | 1-bit B&W |
| Refresh | 1-2 seconds | ~60ms |
| Interface | Parallel | SPI |

## License

Based on [diy-esp32-epub-reader](https://github.com/atomic14/diy-esp32-epub-reader) by atomic14.

See [LICENSE](LICENSE) for details.
