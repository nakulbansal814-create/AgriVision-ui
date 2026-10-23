#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Servo.h>

// =====================================================
//                    WIFI
// =====================================================

#define WIFI_SSID       "NOWHERE1"
#define WIFI_PASSWORD   "12345678"

// =====================================================
//                 FIREBASE
// =====================================================

#define FIREBASE_HOST   "https://agri-vision-4a713-default-rtdb.firebaseio.com"

// =====================================================
//                 SERVO PINS
// =====================================================

// D1 = GPIO5
// D2 = GPIO4

#define SERVO1_PIN 5
#define SERVO2_PIN 4

// =====================================================
//                 SERVO ANGLES
// =====================================================

#define SERVO_ON_ANGLE   90
#define SERVO_OFF_ANGLE   0

// =====================================================
//                    OBJECTS
// =====================================================

Servo servo1;
Servo servo2;

// =====================================================
//                    STATES
// =====================================================

bool servo1State = false;
bool servo2State = false;

// =====================================================
//                 TIMING
// =====================================================

unsigned long lastFirebaseCheck = 0;

const unsigned long FIREBASE_INTERVAL = 1000;

// =====================================================
//              FIREBASE HTTPS CLIENT
// =====================================================

WiFiClientSecure firebaseClient;

// =====================================================
//                    WIFI
// =====================================================

void connectWiFi() {

  Serial.println();
  Serial.println("================================");
  Serial.println("       WIFI CONNECTION");
  Serial.println("================================");

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println("WiFi connected!");

  Serial.print("IP Address: ");

  Serial.println(
    WiFi.localIP()
  );
}

// =====================================================
//               SET SERVO 1
// =====================================================

void setServo1(bool state) {

  servo1State = state;

  if (state) {

    servo1.write(
      SERVO_ON_ANGLE
    );

    Serial.println(
      "Servo 1 -> ON (90 degrees)"
    );

  } else {

    servo1.write(
      SERVO_OFF_ANGLE
    );

    Serial.println(
      "Servo 1 -> OFF (0 degrees)"
    );
  }
}

// =====================================================
//               SET SERVO 2
// =====================================================

void setServo2(bool state) {

  servo2State = state;

  if (state) {

    servo2.write(
      SERVO_ON_ANGLE
    );

    Serial.println(
      "Servo 2 -> ON (90 degrees)"
    );

  } else {

    servo2.write(
      SERVO_OFF_ANGLE
    );

    Serial.println(
      "Servo 2 -> OFF (0 degrees)"
    );
  }
}

// =====================================================
//             SEND STATUS TO FIREBASE
// =====================================================

void sendServoStatus() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  WiFiClientSecure client;

  client.setInsecure();

  HTTPClient https;

  String url =
    String(FIREBASE_HOST) +
    "/servo_status.json";

  String payload = "{";

  payload += "\"device\":\"ESP8266\",";

  payload += "\"servo1\":";

  payload +=
    servo1State
      ? "true"
      : "false";

  payload += ",\"servo2\":";

  payload +=
    servo2State
      ? "true"
      : "false";

  payload += "}";

  Serial.println();
  Serial.println("Updating Firebase status...");

  if (
    https.begin(
      client,
      url
    )
  ) {

    https.addHeader(
      "Content-Type",
      "application/json"
    );

    int httpCode =
      https.PUT(payload);

    Serial.print(
      "Firebase status HTTP code: "
    );

    Serial.println(
      httpCode
    );

    if (httpCode > 0) {

      Serial.println(
        "Servo status uploaded!"
      );

      Serial.println(
        https.getString()
      );

    } else {

      Serial.println(
        "Firebase status upload failed!"
      );
    }

    https.end();

  } else {

    Serial.println(
      "HTTPS connection failed!"
    );
  }
}

// =====================================================
//        READ SERVO 1 COMMAND FROM FIREBASE
// =====================================================

bool readServo1Command(
  bool &state
) {

  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  WiFiClientSecure client;

  client.setInsecure();

  HTTPClient https;

  String url =
    String(FIREBASE_HOST) +
    "/servo_control/servo1.json";

  if (
    !https.begin(
      client,
      url
    )
  ) {

    Serial.println(
      "Servo 1 HTTPS connection failed!"
    );

    return false;
  }

  int httpCode =
    https.GET();

  if (httpCode == HTTP_CODE_OK) {

    String response =
      https.getString();

    response.trim();

    Serial.print(
      "Servo 1 Firebase value: "
    );

    Serial.println(
      response
    );

    if (
      response == "true"
    ) {

      state = true;

      https.end();

      return true;

    }

    if (
      response == "false"
    ) {

      state = false;

      https.end();

      return true;
    }
  }

  else {

    Serial.print(
      "Servo 1 GET failed: "
    );

    Serial.println(
      httpCode
    );
  }

  https.end();

  return false;
}

// =====================================================
//        READ SERVO 2 COMMAND FROM FIREBASE
// =====================================================

bool readServo2Command(
  bool &state
) {

  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  WiFiClientSecure client;

  client.setInsecure();

  HTTPClient https;

  String url =
    String(FIREBASE_HOST) +
    "/servo_control/servo2.json";

  if (
    !https.begin(
      client,
      url
    )
  ) {

    Serial.println(
      "Servo 2 HTTPS connection failed!"
    );

    return false;
  }

  int httpCode =
    https.GET();

  if (httpCode == HTTP_CODE_OK) {

    String response =
      https.getString();

    response.trim();

    Serial.print(
      "Servo 2 Firebase value: "
    );

    Serial.println(
      response
    );

    if (
      response == "true"
    ) {

      state = true;

      https.end();

      return true;

    }

    if (
      response == "false"
    ) {

      state = false;

      https.end();

      return true;
    }
  }

  else {

    Serial.print(
      "Servo 2 GET failed: "
    );

    Serial.println(
      httpCode
    );
  }

  https.end();

  return false;
}

// =====================================================
//             CHECK FIREBASE COMMANDS
// =====================================================

void checkFirebaseCommands() {

  bool newServo1State;
  bool newServo2State;

  // ---------------------------------------------------
  // SERVO 1
  // ---------------------------------------------------

  if (
    readServo1Command(
      newServo1State
    )
  ) {

    if (
      newServo1State != servo1State
    ) {

      Serial.println();
      Serial.println(
        "================================"
      );

      Serial.println(
        "   SERVO 1 COMMAND CHANGED"
      );

      Serial.println(
        "================================"
      );

      setServo1(
        newServo1State
      );

      sendServoStatus();
    }
  }

  // ---------------------------------------------------
  // SERVO 2
  // ---------------------------------------------------

  if (
    readServo2Command(
      newServo2State
    )
  ) {

    if (
      newServo2State != servo2State
    ) {

      Serial.println();
      Serial.println(
        "================================"
      );

      Serial.println(
        "   SERVO 2 COMMAND CHANGED"
      );

      Serial.println(
        "================================"
      );

      setServo2(
        newServo2State
      );

      sendServoStatus();
    }
  }
}

// =====================================================
//                    SETUP
// =====================================================

void setup() {

  Serial.begin(
    115200
  );

  delay(1000);

  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "     HYDRO PLANTIA"
  );

  Serial.println(
    "   ESP8266 SERVO CONTROLLER"
  );

  Serial.println(
    "================================"
  );

  // ---------------------------------------------------
  // ATTACH SERVO 1
  // ---------------------------------------------------

  servo1.attach(
    SERVO1_PIN
  );

  // ---------------------------------------------------
  // ATTACH SERVO 2
  // ---------------------------------------------------

  servo2.attach(
    SERVO2_PIN
  );

  // ---------------------------------------------------
  // INITIAL STATE = OFF
  // ---------------------------------------------------

  servo1State = false;
  servo2State = false;

  servo1.write(
    SERVO_OFF_ANGLE
  );

  servo2.write(
    SERVO_OFF_ANGLE
  );

  Serial.println(
    "Servo 1 initialized at 0 degrees"
  );

  Serial.println(
    "Servo 2 initialized at 0 degrees"
  );

  // ---------------------------------------------------
  // CONNECT WIFI
  // ---------------------------------------------------

  connectWiFi();

  // ---------------------------------------------------
  // HTTPS
  // ---------------------------------------------------

  firebaseClient.setInsecure();

  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "   FIREBASE SERVO CONTROL"
  );

  Serial.println(
    "================================"
  );

  Serial.println(
    "Firebase connected through HTTPS."
  );

  Serial.println(
    "Checking commands..."
  );

  // ---------------------------------------------------
  // SEND INITIAL STATUS
  // ---------------------------------------------------

  sendServoStatus();

  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "      SERVO SYSTEM READY"
  );

  Serial.println(
    "================================"
  );
}

// =====================================================
//                    LOOP
// =====================================================

void loop() {

  // ---------------------------------------------------
  // CHECK WIFI
  // ---------------------------------------------------

  if (
    WiFi.status() != WL_CONNECTED
  ) {

    Serial.println(
      "WiFi disconnected!"
    );

    connectWiFi();
  }

  // ---------------------------------------------------
  // CHECK FIREBASE
  // ---------------------------------------------------

  if (
    millis() -
    lastFirebaseCheck >=
    FIREBASE_INTERVAL
  ) {

    lastFirebaseCheck =
      millis();

    checkFirebaseCommands();
  }

  delay(10);
}