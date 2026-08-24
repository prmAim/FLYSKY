#include <EnableInterrupt.h>
#include <Joystick.h>

const bool DEBUG = true;  // true = выводить сырые значения каналов в Serial Monitor (115200)

// Подключение сигнальных проводов приёмника FS-IA6B к Leonardo
// (порядок — для Mode 2, стандартная раскладка FS-i6)
const uint8_t CH1_PIN = 2;   // Aileron  (CH1) -> Roll
const uint8_t CH2_PIN = 3;   // Elevator (CH2) -> Pitch
const uint8_t CH3_PIN = 7;   // Throttle (CH3)
const uint8_t CH4_PIN = 8;   // Rudder   (CH4) -> Yaw
const uint8_t CH5_PIN = 9;   // Switch   (CH5)
const uint8_t CH6_PIN = 10;  // Switch   (CH6)

const uint8_t CHANNELS = 6;
const uint8_t CH_PINS[CHANNELS] = {CH1_PIN, CH2_PIN, CH3_PIN, CH4_PIN, CH5_PIN, CH6_PIN};

volatile uint32_t riseTime[CHANNELS] = {0};
volatile uint32_t pulseWidth[CHANNELS] = {1500, 1500, 1000, 1500, 1000, 1000};

const int32_t PULSE_MIN = 1000;
const int32_t PULSE_MAX = 2000;
const int32_t PULSE_CENTER = 1500;
const int32_t DEADBAND = 15;

Joystick_ joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
  2, 0,                 // 2 кнопки, 0 hat-переключателей
  true, true, true,     // оси X, Y, Z
  false, false, false,  // без Rx/Ry/Rz
  false, true,          // без руля, есть throttle
  false, false, false); // без accelerator/brake/steering

void onChange(uint8_t idx) {
  uint8_t pin = CH_PINS[idx];
  uint32_t now = micros();
  if (digitalRead(pin) == HIGH) {
    riseTime[idx] = now;
  } else if (riseTime[idx] != 0) {
    uint32_t w = now - riseTime[idx];
    if (w >= 800 && w <= 2200) pulseWidth[idx] = w;
  }
}

void ch1ISR() { onChange(0); }
void ch2ISR() { onChange(1); }
void ch3ISR() { onChange(2); }
void ch4ISR() { onChange(3); }
void ch5ISR() { onChange(4); }
void ch6ISR() { onChange(5); }

int32_t mapCentered(uint32_t pw) {
  int32_t v = (int32_t)pw - PULSE_CENTER;
  v = constrain(v, (int32_t)(PULSE_MIN - PULSE_CENTER), (int32_t)(PULSE_MAX - PULSE_CENTER));
  if (v >= -DEADBAND && v <= DEADBAND) v = 0;
  return v * 2;
}

int32_t mapThrottle(uint32_t pw) {
  int32_t v = (int32_t)pw - PULSE_MIN;
  return constrain(v, 0, (int32_t)(PULSE_MAX - PULSE_MIN));
}

void setup() {
  if (DEBUG) Serial.begin(115200);

  for (uint8_t i = 0; i < CHANNELS; i++) {
    pinMode(CH_PINS[i], INPUT);
  }

  enableInterrupt(CH1_PIN, ch1ISR, CHANGE);
  enableInterrupt(CH2_PIN, ch2ISR, CHANGE);
  enableInterrupt(CH3_PIN, ch3ISR, CHANGE);
  enableInterrupt(CH4_PIN, ch4ISR, CHANGE);
  enableInterrupt(CH5_PIN, ch5ISR, CHANGE);
  enableInterrupt(CH6_PIN, ch6ISR, CHANGE);

  joystick.setXAxisRange(-1000, 1000);
  joystick.setYAxisRange(-1000, 1000);
  joystick.setZAxisRange(-1000, 1000);
  joystick.setThrottleRange(0, 1000);

  joystick.begin(false);
}

void loop() {
  uint32_t pw[CHANNELS];
  noInterrupts();
  for (uint8_t i = 0; i < CHANNELS; i++) pw[i] = pulseWidth[i];
  interrupts();

  joystick.setXAxis(mapCentered(pw[0]));      // Roll
  joystick.setYAxis(mapCentered(pw[1]));      // Pitch
  joystick.setThrottle(mapThrottle(pw[2]));   // Throttle
  joystick.setZAxis(mapCentered(pw[3]));      // Yaw
  joystick.setButton(0, pw[4] > 1600);        // CH5 вверх
  joystick.setButton(1, pw[5] > 1600);        // CH6 вверх
  joystick.sendState();

  static uint32_t lastPrint = 0;
  if (DEBUG && Serial && millis() - lastPrint >= 200) {
    lastPrint = millis();
    Serial.print("CH1..CH6: ");
    for (uint8_t i = 0; i < CHANNELS; i++) {
      Serial.print(pw[i]);
      Serial.print(" ");
    }
    Serial.print("| B1=");
    Serial.print(pw[4] > 1600);
    Serial.print(" B2=");
    Serial.println(pw[5] > 1600);
  }

  delay(10);
}
