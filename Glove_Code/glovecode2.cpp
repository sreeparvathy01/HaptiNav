#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>

Servo leftServo;
Servo centerServo;
Servo rightServo;
Servo downServo;

Servo* servos[4];

struct SensorPacket {
  long left;
  long center;
  long right;
  long down;
  bool lowObstacle;
};

SensorPacket data;

long distance[4] = {200, 200, 200, 200};

bool enabled[4] = {true, true, true, false};
bool servoState[4] = {false, false, false, false};
unsigned long lastMove[4] = {0, 0, 0, 0};

const int minDist = 30;
const int maxDist = 100;

const int servoUp = 90;
const int servoDown = 0;

const int minDelay = 150;
const int maxDelay = 800;

void receiveData(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&data, incomingData, sizeof(data));

  distance[0] = data.left;
  distance[1] = data.center;
  distance[2] = data.right;
  distance[3] = data.down;

  enabled[3] = data.lowObstacle;
}

void setup() {
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  leftServo.attach(13);
  centerServo.attach(12);
  rightServo.attach(14);
  downServo.attach(27);

  servos[0] = &leftServo;
  servos[1] = &centerServo;
  servos[2] = &rightServo;
  servos[3] = &downServo;

  for (int i = 0; i < 4; i++) {
    servos[i]->write(servoDown);
  }

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_register_recv_cb(receiveData);
}

int getTapDelay(long dist) {
  dist = constrain(dist, minDist, maxDist);
  return map(dist, minDist, maxDist, minDelay, maxDelay);
}

void updateServo(int index, unsigned long currentTime) {

  if (!enabled[index] || distance[index] > maxDist) {
    servos[index]->write(servoDown);
    servoState[index] = false;
    lastMove[index] = currentTime;
    return;
  }

  int delayTime = getTapDelay(distance[index]);

  if (currentTime - lastMove[index] >= delayTime) {
    servoState[index] = !servoState[index];

    if (servoState[index]) {
      servos[index]->write(servoUp);
    } else {
      servos[index]->write(servoDown);
    }

    lastMove[index] = currentTime;
  }
}

void loop() {
  unsigned long currentTime = millis();

  for (int i = 0; i < 4; i++) {
    updateServo(i, currentTime);
  }
}