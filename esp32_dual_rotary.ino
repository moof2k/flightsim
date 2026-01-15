// ESP32-S3
// Two dual rotary encoders mapped to X-Plane over serial

#include <Arduino.h>

// FMS knob
const int ENC1_INNER_A = 1;
const int ENC1_INNER_B = 2;
const int ENC1_OUTER_A = 42;
const int ENC1_OUTER_B = 41;
const int BTN1_PUSH    = 40;

// COM knob
const int ENC2_INNER_A = 13;
const int ENC2_INNER_B = 12;
const int ENC2_OUTER_A = 10;
const int ENC2_OUTER_B = 11;
const int BTN2_PUSH    = 9;

int8_t counts[4] = {0, 0, 0, 0};
uint8_t states[4] = {0, 0, 0, 0};

void updateEnc(int pinA, int pinB, uint8_t &state, int8_t &counter) {
  state <<= 2;
  state |= (digitalRead(pinA) << 1) | digitalRead(pinB);
  state &= 0x0F;
  if (state == 0b0001 || state == 0b0111 || state == 0b1110 || state == 0b1000) counter++;
  if (state == 0b0010 || state == 0b1011 || state == 0b1101 || state == 0b0100) counter--;
}

void setup() {
  Serial.begin(115200);
  int allPins[] = {1, 2, 42, 41, 40, 9, 10, 11, 12, 13};
  for (int p : allPins) pinMode(p, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // Sampling
  static unsigned long lastSample = 0;
  if (micros() - lastSample >= 1000) {
    lastSample = micros();
    updateEnc(ENC1_INNER_A, ENC1_INNER_B, states[0], counts[0]);
    updateEnc(ENC1_OUTER_A, ENC1_OUTER_B, states[1], counts[1]);
    updateEnc(ENC2_INNER_A, ENC2_INNER_B, states[2], counts[2]);
    updateEnc(ENC2_OUTER_A, ENC2_OUTER_B, states[3], counts[3]);
  }

  // Process Unit 1: COM1 Radio
  if (counts[2] >= 2)  { Serial.println("sim/GPS/g1000n1_com_inner_up");   counts[2] = 0; }
  if (counts[2] <= -2) { Serial.println("sim/GPS/g1000n1_com_inner_down"); counts[2] = 0; }
  if (counts[3] >= 2)  { Serial.println("sim/GPS/g1000n1_com_outer_up"); counts[3] = 0; }
  if (counts[3] <= -2) { Serial.println("sim/GPS/g1000n1_com_outer_down"); counts[3] = 0; }

  // Process Unit 2: G1000 MFD FMS
  if (counts[0] >= 2)  { Serial.println("sim/GPS/g1000n3_fms_inner_up");   counts[0] = 0; }
  if (counts[0] <= -2) { Serial.println("sim/GPS/g1000n3_fms_inner_down"); counts[0] = 0; }
  if (counts[1] >= 2)  { Serial.println("sim/GPS/g1000n3_fms_outer_up");   counts[1] = 0; }
  if (counts[1] <= -2) { Serial.println("sim/GPS/g1000n3_fms_outer_down"); counts[1] = 0; }

  // Process Buttons
  static bool lastB1 = HIGH, lastB2 = HIGH;
  bool curB1 = digitalRead(BTN1_PUSH);
  bool curB2 = digitalRead(BTN2_PUSH);

  if (curB1 == LOW && lastB1 == HIGH) Serial.println("sim/GPS/g1000n3_cursor");
  if (curB2 == LOW && lastB2 == HIGH) Serial.println("sim/GPS/g1000n1_com12");

  lastB1 = curB1;
  lastB2 = curB2;
}
