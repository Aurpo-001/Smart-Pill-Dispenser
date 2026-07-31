# Hardware and wiring

## Pin map

The firmware uses the following Arduino Uno-compatible pin assignment.

| Device | Signal | Arduino pin | Notes |
|---|---|---:|---|
| Servo | Control | D9 | Servo power should not be drawn from a heavily loaded board regulator. |
| Buzzer | Signal | D11 | Firmware uses `tone()` for reminder and tamper patterns. |
| Green LED | Anode through resistor | D13 | Medicine-due indicator. |
| Red LED | Anode through resistor | D12 | Tamper indicator. |
| Lid switch | Digital input | D2 | `INPUT_PULLUP`; LOW = closed, HIGH = open. |
| Reset button | Digital input | D3 | `INPUT_PULLUP`; connect button to GND. |
| Select button | Digital input | D4 | `INPUT_PULLUP`; connect button to GND. |
| Increment button | Digital input | D5 | `INPUT_PULLUP`; connect button to GND. |
| DS3231 RTC | SDA / SCL | A4 / A5 | Shared I²C bus on Arduino Uno. |
| 16×2 LCD | SDA / SCL | A4 / A5 | Default address is `0x27`. |

## Power notes

- Use a stable regulated supply appropriate for the selected board and modules.
- Power the servo from a suitable external 5 V rail when possible; connect the external supply ground to Arduino GND.
- Add a 100–470 µF capacitor near the servo supply to reduce resets caused by current spikes.
- Fit current-limiting resistors (typically 220–330 Ω) in series with discrete LEDs.
- Verify every module's voltage requirements before connection. Some boards are 3.3 V only.
- Do not connect or rearrange wiring while the system is powered.

## Lid switch convention

The sketch assumes the switch connects D2 to GND while the lid is fully closed. This makes the input LOW when closed and HIGH when open. If your mechanism behaves in reverse, change:

```cpp
constexpr uint8_t LID_CLOSED_LEVEL = LOW;
```

## Servo calibration

The default positions are:

```cpp
constexpr uint8_t SERVO_LOCKED_ANGLE = 0;
constexpr uint8_t SERVO_UNLOCKED_ANGLE = 90;
```

Disconnect the lock linkage before initial testing. Confirm the servo direction and mechanical travel, then adjust these angles so the mechanism does not stall at either endpoint.

## I²C checks

Both the DS3231 and LCD share SDA/SCL. A typical DS3231 uses address `0x68`; the LCD backpack in this project is configured as `0x27`. Run an I²C scanner if the display remains blank.
