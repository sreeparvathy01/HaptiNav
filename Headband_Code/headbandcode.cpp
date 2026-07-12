//headband
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WebServer.h>

// config stuff
bool DEBUG_MODE = true;

// pins
const int trigLeft = 4,   echoLeft = 5;
const int trigCenter = 18, echoCenter = 19;
const int trigRight = 21,  echoRight = 22;
const int trigDown = 25,   echoDown = 26;

uint8_t gloveAddress[] = {0xE0, 0x8C, 0xFE, 0x32, 0xFC, 0xBC};

struct SensorPacket {
  long leftDistance;
  long centerDistance;
  long rightDistance;
  long downDistance;
  bool obstacleAbove;
  bool obstacleBelow;
};

SensorPacket data;

// geometry stuff - see notes for derivation
float sensorHeightCm = 150.0;
float sensorTiltDegrees = 48.6; // 150/sin(48.6) ~ 200cm expected floor distance
float waistHeightCm = 100.0;
float obstacleMargin = 10.0;
float dropOffMargin = 15.0;

// dashboard stuff
const char* hotspotName = "TapNav-Headband";
const char* hotspotPassword = "12345678";
WebServer server(80);

const int alertHistorySize = 5;
String alertHistory[alertHistorySize];
int alertHistoryIndex = 0;

// debounce
int obstacleAboveStreak = 0;
int obstacleBelowStreak = 0;
int streakNeededToConfirm = 3;

// filtering
long recentDownReadings[5] = {200, 200, 200, 200, 200};
int recentDownIndex = 0;

const int validityWindowSize = 8;
bool recentReadingsValid[validityWindowSize];
int validityIndex = 0;
bool validityWindowFilled = false;
float minValidRatioForDropOff = 0.5;

void logAlert(String message) {
  alertHistory[alertHistoryIndex] = message;
  alertHistoryIndex = (alertHistoryIndex + 1) % alertHistorySize;
}

String buildDashboardHtml() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta http-equiv='refresh' content='1'>";
  html += "<title>TapNav Live Dashboard</title>";
  html += "<style>";
  html += "body{font-family:Arial;background:#111;color:#eee;padding:20px;}";
  html += "h1{color:#4ade80;}";
  html += ".card{background:#222;border-radius:10px;padding:15px;margin:10px 0;}";
  html += ".ok{color:#4ade80;} .warn{color:#f87171;font-weight:bold;}";
  html += "table{width:100%;border-collapse:collapse;} td{padding:8px;border-bottom:1px solid #333;}";
  html += "</style></head><body>";

  html += "<h1>TapNav Live Dashboard</h1>";
  html += "<div class='card'><table>";
  html += "<tr><td>Left</td><td>" + String(dataToSend.leftDistance) + " cm</td></tr>";
  html += "<tr><td>Center</td><td>" + String(dataToSend.centerDistance) + " cm</td></tr>";
  html += "<tr><td>Right</td><td>" + String(dataToSend.rightDistance) + " cm</td></tr>";
  html += "<tr><td>Ground</td><td>" + String(dataToSend.downDistance) + " cm</td></tr>";
  html += "</table></div>";

  html += "<div class='card'>";
  html += "<p>Raised obstacle: <span class='" + String(dataToSend.obstacleAbove ? "warn" : "ok") + "'>";
  html += String(dataToSend.obstacleAbove ? "DETECTED" : "clear") + "</span></p>";
  html += "<p>Drop-off ahead: <span class='" + String(dataToSend.obstacleBelow ? "warn" : "ok") + "'>";
  html += String(dataToSend.obstacleBelow ? "DETECTED" : "clear (disabled)") + "</span></p>";
  html += "</div>";

  html += "<div class='card'><b>Recent alerts:</b><ul>";
  for (int i = 0; i < alertHistorySize; i++) {
    if (alertHistory[i].length() > 0) {
      html += "<li>" + alertHistory[i] + "</li>";
    }
  }
  html += "</ul></div></body></html>";
  return html;
}

long getDistanceCm(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long travelTime = pulseIn(echoPin, HIGH, 30000);
  if (travelTime == 0) return 200; // no echo back in time, just assume clear path

  return travelTime * 0.034 / 2;
}

long getSmoothedDownDistance(long newReading) {
  recentDownReadings[recentDownIndex] = newReading;
  recentDownIndex = (recentDownIndex + 1) % 5;

  long sorted[5];
  for (int i = 0; i < 5; i++) sorted[i] = recentDownReadings[i];

  // bubble sort, only 5 elements so who cares about efficiency
  for (int i = 0; i < 5; i++) {
    for (int j = i + 1; j < 5; j++) {
      if (sorted[j] < sorted[i]) {
        long temp = sorted[i];
        sorted[i] = sorted[j];
        sorted[j] = temp;
      }
    }
  }
  return sorted[2]; // median
}

float updateAndGetValidRatio(long rawReading) {
  bool isValid = rawReading < 195;
  recentReadingsValid[validityIndex] = isValid;
  validityIndex = (validityIndex + 1) % validityWindowSize;
  if (validityIndex == 0) validityWindowFilled = true;

  int validCount = 0;
  int totalToCheck = validityWindowFilled ? validityWindowSize : validityIndex;
  if (totalToCheck == 0) return 0.0;

  for (int i = 0; i < totalToCheck; i++) {
    if (recentReadingsValid[i]) validCount++;
  }
  return (float)validCount / (float)totalToCheck;
}

bool isObstacleAbove(long downDistance) {
  float expectedFloor = sensorHeightCm / sin(sensorTiltDegrees * PI / 180.0);
  float heightOfObject = sensorHeightCm - (downDistance * sin(sensorTiltDegrees * PI / 180.0));

  bool closerThanFloor = downDistance < (expectedFloor - obstacleMargin);
  bool belowWaist = heightOfObject > 0 && heightOfObject < waistHeightCm;

  if (DEBUG_MODE) {
    Serial.printf("Floor:%f | Measured:%ld | ObjHeight:%f | Triggered:%d\n",
                  expectedFloor, downDistance, heightOfObject, (closerThanFloor && belowWaist));
  }
  return closerThanFloor && belowWaist;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(trigLeft, OUTPUT);   pinMode(echoLeft, INPUT);
  pinMode(trigCenter, OUTPUT); pinMode(echoCenter, INPUT);
  pinMode(trigRight, OUTPUT);  pinMode(echoRight, INPUT);
  pinMode(trigDown, OUTPUT);   pinMode(echoDown, INPUT);

  memset(recentReadingsValid, 0, sizeof(recentReadingsValid));

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(hotspotName, hotspotPassword);

  Serial.print("Dashboard IP: ");
  Serial.println(WiFi.softAPIP());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_peer_info_t gloveInfo = {};
  memcpy(gloveInfo.peer_addr, gloveAddress, 6);
  gloveInfo.channel = 0;
  gloveInfo.encrypt = false;
  esp_now_add_peer(&gloveInfo);

  server.on("/", []() {
    server.send(200, "text/html", buildDashboardHtml());
  });
  server.begin();
}

void loop() {
  server.handleClient();

  dataToSend.leftDistance = getDistanceCm(trigLeft, echoLeft);
  dataToSend.centerDistance = getDistanceCm(trigCenter, echoCenter);
  dataToSend.rightDistance = getDistanceCm(trigRight, echoRight);

  long rawDownReading = getDistanceCm(trigDown, echoDown);
  dataToSend.downDistance = getSmoothedDownDistance(rawDownReading);

  // raised obstacle check, debounced so one noisy reading doesn't trigger it
  bool aboveDetected = isObstacleAbove(dataToSend.downDistance);
  bool wasAboveBefore = dataToSend.obstacleAbove;

  if (aboveDetected) obstacleAboveStreak++;
  else obstacleAboveStreak = 0;

  dataToSend.obstacleAbove = (obstacleAboveStreak >= streakNeededToConfirm);

  // drop-off detection - not doing this for now
  dataToSend.obstacleBelow = false;

  if (dataToSend.obstacleAbove && !wasAboveBefore) {
    logAlert("Obstacle hit at " + String(millis() / 1000) + "s");
  }

  esp_now_send(gloveAddress, (uint8_t *)&dataToSend, sizeof(dataToSend));

  Serial.printf("L:%ld C:%ld R:%ld D:%ld Alert:%d\n",
    dataToSend.leftDistance, dataToSend.centerDistance,
    dataToSend.rightDistance, dataToSend.downDistance, dataToSend.obstacleAbove);

  delay(100);
}