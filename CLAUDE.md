# CLAUDE.md

Guidance for Claude Code (claude.ai/code) when hacking on the **tstick** firmware that lives in this repo.

## Project Overview

tstick is a palm-sized pointing device that combines:

- Seeeduino XIAO BLE (nRF52840) as the only MCU
- Three direct keyboard switches (3×1 matrix)
- A PMW3610-based trackball for cursor control
- One EC11-style rotary encoder for volume/scroll shortcuts

Everything runs on ZMK, so this repo only contains the configuration that sits on top of upstream ZMK.

## Build Commands

Typical West invocations:

```bash
# clean build of main firmware
west build -p -b seeeduino_xiao_ble -- -DSHIELD=tstick

# build the standard settings reset image
west build -p -b seeeduino_xiao_ble -- -DSHIELD=settings_reset
```

## Architecture

### Hardware Layout

| Function | Pin | Notes |
| --- | --- | --- |
| Row0 | D1 (P0.03) | Matrix row |
| Row1 | D2 (P0.28) | Matrix row |
| Row2 | D3 (P0.29) | Matrix row |
| Col0 | D10 (P1.15) | Matrix column |
| RE_A | D9 (P1.14) | Rotary encoder phase A |
| RE_B | D8 (P1.13) | Rotary encoder phase B |
| PMW3610 SDIO | D4 (P0.04) | Shared MOSI/MISO |
| PMW3610 SCLK | D5 (P0.05) | SPI clock |
| PMW3610 CS | D7 (P1.10) | Chip select |
| PMW3610 MOTION | D0 (P0.02) | IRQ |

### Firmware Blocks

- `zmk,kscan-gpio-matrix` provides the 3×1 switch matrix with `diode-direction = col2row`.
- `alps,ec11` rotary sensor feeds `zmk,keymap-sensors` so `sensor-bindings` can use `&inc_dec_kp`.
- `pixart,pmw3610` driver (via `zmk-pmw3610-driver` module from `west.yml`) handles the trackball on SPI1.
- `zmk,input-listener` + `zip_*` processors toggle a scroll mode (layer 1) for the trackball when requested.

## Key Files

- `config/boards/shields/tstick/tstick.dtsi` – complete hardware description (matrix, encoder, PMW3610, listeners)
- `config/boards/shields/tstick/tstick.conf` – Zephyr/ZMK Kconfig enabling BLE, PMW3610, EC11, etc.
- `config/boards/shields/tstick/Kconfig.*` – shield registration and defaults
- `config/tstick.keymap` – two-layer keymap (default + scroll) plus rotary bindings
- `config/tstick.json` – layout metadata for ZMK Studio / keymap editors
- `build.yaml` – GitHub Actions matrix describing which shield builds to run
- `config/west.yml` – manifest bringing in ZMK and the PMW3610 module

## Keymap Layers

1. **default** – ESC, Space, and a momentary key that switches to the scroll layer; rotary encoder adjusts system volume.
2. **scroll** – Mouse buttons plus the same rotary bindings (since `sensor-bindings` live at the keymap root).

The PMW3610 drives cursor movement on every layer. When layer 1 is held, the `zip_xy_to_scroll_mapper` turns movement into scroll wheel events, giving "trackball scroll mode".

## Trackball + Rotary notes

- Trackball CPI is set to 400 by default; tweak `cpi` inside `trackball@0` if you need different sensitivity.
- The listener in `tstick.dtsi` already inverts Y to match the PCB orientation and scales scroll by 1/16.
- Rotary encoder resolution is set to 24 `triggers-per-rotation` with the expectation of a standard EC11 (detent every 15°). Adjust `steps`/`triggers-per-rotation` together if you swap encoders.

## Workflow Tips

- Edit key bindings in `config/tstick.keymap`; `sensor-bindings` follow the order of the sensors array defined in the DTS.
- Pin, behavior, or pointing tweaks should happen inside `config/boards/shields/tstick/tstick.dtsi`.
- Run `west build -p -b seeeduino_xiao_ble -- -DSHIELD=tstick` locally before pushing changes; flashing works with the usual `west flash`.
