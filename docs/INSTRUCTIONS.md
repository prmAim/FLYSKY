# FS-i6 → Arduino Leonardo → PC (simulator) — instructions

Goal: connect a **FlySky FS-i6** transmitter with an **FS-IA6B** receiver to a PC via an **Arduino Leonardo** and control a flight simulator (e.g. World of Aircraft) as a USB joystick with minimal latency.

## How it works

1. The FS-i6 transmitter sends a signal to the FS-IA6B receiver.
2. The receiver outputs all 6 channels as a single digital packet over the **iBUS** protocol (serial, 115200 baud), refreshed every ~7 ms.
3. The Leonardo reads this stream via `Serial1` (pin 0 / RX) and decodes the channel values.
4. The Leonardo presents itself as a USB gamepad (Joystick library): X/Y/Z axes + Throttle + 2 buttons.
5. The simulator sees a regular joystick.

Why iBUS instead of PWM: PWM outputs refresh every 20 ms (50 Hz), iBUS every ~7 ms. This removes the main source of latency.

Channel layout (Mode 2, FS-i6 standard):

| Channel | Function | Control          | Joystick mapping |
|---------|----------|------------------|------------------|
| CH1     | Aileron (roll)  | right stick ←/→ | X axis (Roll)    |
| CH2     | Elevator (pitch)| right stick ↑/↓ | Y axis (Pitch)   |
| CH3     | Throttle        | left stick ↑/↓  | Throttle         |
| CH4     | Rudder (yaw)    | left stick ←/→  | Z axis (Yaw)     |
| CH5     | Switch SWA      | toggle          | Button 1         |
| CH6     | Switch SWB      | toggle          | Button 2         |

## Components

- FlySky FS-i6 transmitter
- FS-IA6B receiver
- Arduino Leonardo
- Jumper wire (dupont female-to-female) — one signal wire for iBUS
- Windows PC with a flight simulator

## Wiring

| FS-IA6B receiver              | Arduino Leonardo |
|-------------------------------|------------------|
| GND (`-` pin)                 | GND              |
| VCC (`+` pin, 5V)             | 5V               |
| `SERVO` port (iBUS), signal pin | D0 (RX)        |

Important notes:

- The iBUS port is the **3 horizontal pins on the side of the board**, labelled **`SERVO`**. Do not confuse it with the two rows of channels CH1–CH6 or with the `SENS` port (sensors/telemetry — not needed).
- Of the three pins, only the **signal** pin is required (the outer one; the middle = +5V). The receiver already gets GND and 5V on separate wires.
- Power the receiver from the Leonardo's 5V (~40 mA, sufficient over USB).
- The FS-i6 transmitter needs no configuration: iBUS on the `SERVO` port is enabled by default after binding.

## Installing libraries

In Arduino IDE: **Sketch → Include Library → Manage Libraries**, find and install:

1. **Joystick** (Matthew Heironimus)

(The `EnableInterrupt` library is no longer needed — PWM interrupts were removed.)

## Transmitter setup (FS-i6)

1. Verify **Mode 2** (default).
2. **OK (hold 1 s) → Functions setup → Aux. channels**:
   - **Ch5 = SWA**
   - **Ch6 = SWB**
   - (by default CH5/CH6 often have knobs VrA/VrB — replace them with the switches)
3. Verify: **OK → Functions setup → Display** — flick SWA/SWB; bars 5 and 6 should move.

## Receiver binding

1. On the transmitter, hold the **BIND** button (bottom left) and power on.
2. On the FS-IA6B, jumper the **B/VCC** (bind) contacts while applying power.
3. After a successful bind the receiver LED stays solid. Remove the jumper.

## Flashing the Leonardo

1. **Tools → Board → Arduino Leonardo**, select the COM port.
2. Open `FLYSKY_v1.ino` and click **Upload**.
3. If it hangs at `Connecting to programmer: .` — press **Reset** on the board and retry.

## Verifying in Windows

1. Win+R → `joy.cpl` → Properties.
2. Move the sticks: crosshair (X/Y), slider Z (left stick ←/→).
3. Switches SWA/SWB light up Button 1 and Button 2.
4. Throttle is **not shown** in the standard `joy.cpl` window — that's normal; verify it in-game.

## Simulator setup (World of Aircraft)

1. Connect the Leonardo **before** starting the game (the gamepad must be detected — check in `joy.cpl`).
2. Start the game, open **Options / Settings → Controls / Input** (exact menu names may differ slightly).
3. In the device list, select the controller — usually shown as **Arduino Leonardo** or **Gamepad / Joy 1**.
4. Assign axes — click the action row and move the corresponding stick to its limit:
   - **Roll / Aileron** → right stick ←/→ (X axis)
   - **Pitch / Elevator** → right stick ↑/↓ (Y axis)
   - **Yaw / Rudder** → left stick ←/→ (Z axis)
   - **Throttle** → left stick ↑/↓ (Throttle axis)
   - **Buttons** (gear, flaps, etc.) → switches SWA/SWB (Button 1 / Button 2)
5. Check the response: moving the sticks should move the corresponding indicator/aircraft in-game.
6. If an axis reacts "backwards" — enable **Invert** for it.
7. If **Calibrate / Deadzone** is available — calibrate and set the deadzone.
8. If latency remains — reduce input smoothing/filtering, if the game offers it.

Notes:

- In Unity games an axis is often only assigned after a full stick travel — move it to the limit.
- **Throttle** is a separate "Throttle" axis; don't confuse it with the Z axis.
- If the game doesn't see the device — restart the game (the gamepad must be connected before launch).

## Diagnostics

The sketch has a `DEBUG` flag (at the top of the file):

```cpp
const bool DEBUG = true;  // true = print channel values and refresh rate to Serial Monitor
```

With `DEBUG = true`, **Tools → Serial Monitor** (115200) prints every ~0.5 s:

```
CH1..CH6: 1500 1500 1000 1500 1000 1000 | frames=68 interval=7.1ms (136/s) | B1=0 B2=0
```

- `CH1..CH6` — channel values (µs).
- `frames` — valid iBUS frames received over the interval.
- `interval` — time between adjacent frames (should be ~7 ms).
- `B1/B2` — button states.

If `interval` is noticeably above 10 ms or `frames/s` noticeably below 100 — there is a problem on the iBUS line.

Troubleshooting:

| Symptom | Cause |
|---------|-------|
| Serial Monitor shows nothing / large `interval` | iBUS wire not on the `SERVO` port signal pin or not on D0 |
| Channel value doesn't change when moving the stick | Wrong channel assigned in-game / stick not calibrated on the transmitter |
| CH5/CH6 don't react to the switches | Ch5/Ch6 not assigned to SWA/SWB on the transmitter (they're VrA/VrB) |
| Axis inverted in-game | Invert it in the game settings (not in code) |
| Latency remains in-game | Reduce smoothing/deadzone in the game's input settings |

## Project files

- `FLYSKY_v1.ino` — Leonardo firmware (iBUS joystick + diagnostics).
