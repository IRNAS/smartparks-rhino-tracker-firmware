#include "gtest/gtest.h"
#include "arduino-mock/Arduino.h"
#include "arduino-mock/Serial.h"

#include "../main.ino"

using ::testing::Return;
TEST(loop, pushed) {
  ArduinoMock* arduinoMock = arduinoMockInstance();
  SerialMock* serialMock = serialMockInstance();
  EXPECT_CALL(*arduinoMock, digitalWrite(1, HIGH));
  loop();
  releaseSerialMock();
  releaseArduinoMock();
}