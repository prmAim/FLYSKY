# FlySky FS-i6 → Arduino Leonardo → PC (USB Joystick)

Turn a **FlySky FS-i6** transmitter with an **FS-IA6B** receiver into a low-latency USB joystick for PC flight simulators, using an **Arduino Leonardo**.

The receiver exposes all 6 channels as a single digital iBUS frame (~7 ms refresh). The Leonardo parses that stream and presents it as a standard HID joystick — no extra drivers required.

## How it works

1. The FS-i6 transmitter sends a signal to the FS-IA6B receiver.
2. The receiver outputs all 6 channels as one digital iBUS packet over serial at 115200 baud, updated every ~7 ms.
3. The Leonardo reads this stream on `Serial1` (pin 0 / RX) and decodes the channel values.
4. The Leonardo presents itself as a USB gamepad (Joystick library): X/Y/Z axes + Throttle + 2 buttons.
5. The simulator sees a regular joystick.

> Why iBUS instead of PWM: PWM outputs update every 20 ms (50 Hz); iBUS updates every ~7 ms. This removes the main source of control latency.

## Channel mapping (Mode 2, FS-i6 default)

| Channel | Function | Control         | Joystick mapping |
|---------|----------|-----------------|------------------|
| CH1     | Aileron  | right stick ←/→ | X axis (Roll)    |
| CH2     | Elevator | right stick ↑/↓ | Y axis (Pitch)   |
| CH3     | Throttle | left stick ↑/↓  | Throttle         |
| CH4     | Rudder   | left stick ←/→  | Z axis (Yaw)     |
| CH5     | Switch SWA | toggle       | Button 1         |
| CH6     | Switch SWB | toggle       | Button 2         |

## Hardware required

- FlySky FS-i6 transmitter
- FS-IA6B receiver
- Arduino Leonardo
- One dupont (female-to-female) jumper wire for the iBUS signal
- Windows PC with a flight simulator

## Wiring

| FS-IA6B receiver              | Arduino Leonardo |
|-------------------------------|------------------|
| GND (`-` pin)                 | GND              |
| VCC (`+` pin, 5V)             | 5V               |
| `SERVO` port signal pin (iBUS)| D0 (RX)          |

- The iBUS port is the **3 horizontal pins on the side of the board**, labelled **`SERVO`**. Do not confuse it with the CH1–CH6 channel rows or the `SENS` port (sensors/telemetry — not needed).
- Only the **signal** pin is required (the outer pin; the middle is +5V). The receiver already gets GND and 5V on separate wires.
- Power the receiver from the Leonardo's 5V rail (~40 mA — fine over USB).
- The FS-i6 transmitter needs no configuration: iBUS on the `SERVO` port is enabled by default after binding.

Wiring diagram: [docs/pinout.png](docs/pinout.png).

## Installation

### Arduino libraries

In Arduino IDE: **Sketch → Include Library → Manage Libraries**, install:

1. **Joystick** (Matthew Heironimus)

### Transmitter setup (FS-i6)

1. Verify **Mode 2** (default).
2. **OK (hold 1 s) → Functions setup → Aux. channels**:
   - **Ch5 = SWA**
   - **Ch6 = SWB**
   - (CH5/CH6 often default to knobs VrA/VrB — replace them with the switches)
3. Verify: **OK → Functions setup → Display** — flicking SWA/SWB should move bars 5 and 6.

### Receiver binding

1. On the transmitter, hold the **BIND** button (bottom left) and power it on.
2. On the FS-IA6B, jumper the **B/VCC** (bind) pins while applying power.
3. After a successful bind the receiver LED stays solid. Remove the jumper.

### Flashing the Leonardo

1. **Tools → Board → Arduino Leonardo**, select the COM port.
2. Open `FLYSKY_v1/FLYSKY_v1.ino` and click **Upload**.
3. If it hangs at `Connecting to programmer: .`, press **Reset** on the board and retry.

## Usage

Verify in Windows:

1. Win+R → `joy.cpl` → Properties.
2. Move the sticks: crosshair (X/Y), slider Z (left stick ←/→).
3. Switches SWA/SWB light up Button 1 and Button 2.
4. Throttle is **not shown** in the standard `joy.cpl` window — that's normal; check it in-game.

Configure the simulator (e.g. World of Aircraft):

1. Connect the Leonardo **before** starting the game.
2. Open **Options / Settings → Controls / Input**.
3. Select the device — usually shown as **Arduino Leonardo** or **Gamepad / Joy 1**.
4. Bind axes by clicking the action and moving the corresponding stick to its limit.
5. If an axis reacts backwards, enable **Invert** for it.
6. Use **Calibrate / Deadzone** if available.
7. If latency remains, reduce input smoothing/filtering in the game.

## Debugging

The sketch has a `DEBUG` flag at the top:

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

If `interval` is well above 10 ms or `frames/s` well below 100, there is an iBUS line problem.

| Symptom | Cause |
|---------|-------|
| No output / large `interval` | iBUS wire not on the `SERVO` port signal pin or not on D0 |
| Channel value doesn't change | Wrong channel bound in-game / stick not calibrated on the transmitter |
| CH5/CH6 don't react to switches | Ch5/Ch6 not assigned to SWA/SWB (they're VrA/VrB) |
| Axis inverted in-game | Invert it in the game settings (not in code) |
| Latency remains | Reduce smoothing/deadzone in the game's input settings |

## Repository structure

```
.
├── FLYSKY_v1/                  # Arduino Leonardo sketch
│   └── FLYSKY_v1.ino
├── docs/
│   ├── INSTRUCTIONS.md         # Detailed instructions (English)
│   ├── INSTRUCTIONS.ru.md      # Detailed instructions (Russian)
│   ├── Manual-WoA_GliderSimulator_en.pdf
│   └── pinout.png              # Wiring diagram
├── LICENSE
└── README.md
```

Full instructions: [English](docs/INSTRUCTIONS.md) · [Русский](docs/INSTRUCTIONS.ru.md).

## License

[MIT](LICENSE) © Igor Ankudinov
