# User guide

## Start-up

1. Close the lid before powering the prototype.
2. Apply power and wait for the start-up message.
3. Confirm the displayed time is correct.
4. The second LCD line shows the next configured dose.

The default daily reminders are **08:00**, **15:00**, and **21:00**.

## Configure reminder times

1. Press **Select** to enter settings.
2. The display shows the active alarm and whether its hour or minute is selected.
3. Press **Increment** to change that value.
4. Press **Select** again to move to the next field.
5. Continue until all six fields have been reviewed, or wait 12 seconds to exit automatically.

Settings are not retained after power is removed in this firmware version.

## Take a scheduled dose

1. At the configured time, the servo unlocks the lid, the green LED turns on, and the buzzer pulses.
2. Open the lid. The sound stops and the LCD confirms that the compartment was accessed.
3. Close the lid. The servo relocks automatically and the controller returns to the clock screen.

Press **Reset** to cancel an active reminder and relock the lid, provided the lid is closed.

## Respond to a tamper warning

Opening the lid outside a scheduled reminder activates the red LED, continuous buzzer, and warning screen.

1. Close the lid.
2. Press **Reset**.
3. Confirm that the system returns to the normal clock screen.

Repeated or unexplained alerts indicate a switch, wiring, or enclosure-alignment problem that should be corrected before further use.

## Troubleshooting

| Symptom | Check |
|---|---|
| `RTC ERROR` | RTC power, SDA/SCL wiring, module address, and coin-cell orientation |
| Blank LCD | Contrast potentiometer, power, address `0x27`, and I²C wiring |
| Lid state is reversed | Switch wiring or `LID_CLOSED_LEVEL` in the firmware |
| Board resets when servo moves | Use a separate regulated servo supply with a common ground |
| Wrong time after power loss | Set the RTC using an RTClib clock-adjustment example |
| Alarm time resets after restart | Expected: alarm edits currently use RAM only |
