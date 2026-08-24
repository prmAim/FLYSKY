#include <Joystick.h>

const bool DEBUG = true;  // true = вывод значений каналов и частоты обновления в Serial Monitor (115200)

// Приёмник FS-IA6B -> Arduino Leonardo через iBUS:
// сигнальный провод с порта «SERVO» (боковые 3 горизонтальных пина) -> пин 0 (RX) Leonardo.
// Питание GND/5V приёмника остаётся как раньше. Пульт FS-i6 настраивать не нужно — iBUS включён по умолчанию.
const uint8_t IBUS_RX_PIN = 0;

// Раскладка каналов (Mode 2, FS-i6):
// CH1 -> Aileron (Roll)   -> ось X
// CH2 -> Elevator (Pitch) -> ось Y
// CH3 -> Throttle         -> ось Throttle
// CH4 -> Rudder (Yaw)     -> ось Z
// CH5 -> тумблер SWA      -> Кнопка 1
// CH6 -> тумблер SWB      -> Кнопка 2

const int32_t PULSE_MIN = 1000;
const int32_t PULSE_MAX = 2000;
const int32_t PULSE_CENTER = 1500;
const int32_t DEADBAND = 5;   // мёртвая зона около центра (мкс)

// Формат кадра iBUS: 32 байта, заголовок 0x20 0x40, 14 каналов по uint16 (little-endian),
// затем 2 байта контрольной суммы = 0xFFFF - сумма байт [0..29]. Обновление ~7 мс.
const uint8_t IBUS_LENGTH = 32;
const uint8_t IBUS_HEADER0 = 0x20;
const uint8_t IBUS_HEADER1 = 0x40;

Joystick_ joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
  2, 0,                 // 2 кнопки, 0 hat-переключателей
  true, true, true,     // оси X, Y, Z
  false, false, false,  // без Rx/Ry/Rz
  false, true,          // без руля, есть throttle
  false, false, false); // без accelerator/brake/steering

uint8_t ibusBuffer[IBUS_LENGTH];
uint8_t ibusIndex = 0;

// Последние валидные значения каналов (при потере кадра держим их).
uint16_t channels[6] = {1500, 1500, 1000, 1500, 1000, 1000};

// Диагностика частоты обновления
uint32_t frameCount = 0;
uint32_t lastFrameMicros = 0;
uint32_t frameIntervalMicros = 0;

// Читает кадры iBUS из Serial1. Возвращает true, если получен хотя бы один валидный кадр.
bool readIBUS() {
  bool gotFrame = false;

  while (Serial1.available() > 0) {
    uint8_t b = Serial1.read();

    if (ibusIndex == 0 && b != IBUS_HEADER0) continue;
    if (ibusIndex == 1 && b != IBUS_HEADER1) {
      ibusIndex = (b == IBUS_HEADER0) ? 1 : 0;
      continue;
    }

    ibusBuffer[ibusIndex++] = b;
    if (ibusIndex < IBUS_LENGTH) continue;
    ibusIndex = 0;

    // Контрольная сумма: 0xFFFF - сумма байт [0..29]
    uint16_t sum = 0;
    for (uint8_t i = 0; i < 30; i++) sum += ibusBuffer[i];
    uint16_t checksum = (uint16_t)ibusBuffer[30] | ((uint16_t)ibusBuffer[31] << 8);
    if ((uint16_t)(0xFFFF - sum) != checksum) continue;  // битый кадр — пропускаем

    for (uint8_t i = 0; i < 6; i++) {
      uint16_t v = (uint16_t)ibusBuffer[2 + i * 2] | ((uint16_t)ibusBuffer[3 + i * 2] << 8);
      if (v >= PULSE_MIN && v <= PULSE_MAX) channels[i] = v;
    }

    uint32_t now = micros();
    if (lastFrameMicros != 0) frameIntervalMicros = now - lastFrameMicros;
    lastFrameMicros = now;
    frameCount++;
    gotFrame = true;
  }

  return gotFrame;
}

int32_t mapCentered(uint16_t pw) {
  int32_t v = (int32_t)pw - PULSE_CENTER;
  if (v >= -DEADBAND && v <= DEADBAND) v = 0;
  return v * 2;
}

int32_t mapThrottle(uint16_t pw) {
  int32_t v = (int32_t)pw - PULSE_MIN;
  return constrain(v, 0, (int32_t)(PULSE_MAX - PULSE_MIN));
}

void setup() {
  if (DEBUG) Serial.begin(115200);
  Serial1.begin(115200);  // iBUS

  joystick.setXAxisRange(-1000, 1000);
  joystick.setYAxisRange(-1000, 1000);
  joystick.setZAxisRange(-1000, 1000);
  joystick.setThrottleRange(0, 1000);

  joystick.begin(false);
}

void loop() {
  readIBUS();

  int32_t x = mapCentered(channels[0]);      // Roll
  int32_t y = mapCentered(channels[1]);      // Pitch
  int32_t throttle = mapThrottle(channels[2]);
  int32_t z = mapCentered(channels[3]);      // Yaw
  bool b1 = channels[4] > 1600;
  bool b2 = channels[5] > 1600;

  // Отправляем HID-отчёт сразу, как только что-то изменилось (без delay, минимум задержки).
  static int32_t lastX = -9999, lastY = -9999, lastZ = -9999, lastThrottle = -9999;
  static bool lastB1 = false, lastB2 = false;
  if (x != lastX || y != lastY || z != lastZ || throttle != lastThrottle || b1 != lastB1 || b2 != lastB2) {
    joystick.setXAxis(x);
    joystick.setYAxis(y);
    joystick.setZAxis(z);
    joystick.setThrottle(throttle);
    joystick.setButton(0, b1);
    joystick.setButton(1, b2);
    joystick.sendState();

    lastX = x; lastY = y; lastZ = z; lastThrottle = throttle;
    lastB1 = b1; lastB2 = b2;
  }

  if (DEBUG && Serial) {
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint >= 500) {
      uint32_t dt = millis() - lastPrint;
      lastPrint = millis();
      float fps = frameCount * 1000.0f / dt;

      Serial.print("CH1..CH6: ");
      for (uint8_t i = 0; i < 6; i++) { Serial.print(channels[i]); Serial.print(" "); }
      Serial.print("| frames=");
      Serial.print(frameCount);
      Serial.print(" interval=");
      Serial.print(frameIntervalMicros / 1000.0f, 1);
      Serial.print("ms (");
      Serial.print(fps, 0);
      Serial.print("/s) | B1=");
      Serial.print(b1);
      Serial.print(" B2=");
      Serial.println(b2);

      frameCount = 0;
    }
  }
}
