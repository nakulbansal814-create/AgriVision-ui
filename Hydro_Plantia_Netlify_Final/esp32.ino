#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include <DHT.h>

// =====================================================
//                    WIFI SETTINGS
// =====================================================

#define WIFI_SSID       "NOWHERE1"
#define WIFI_PASSWORD   "12345678"

// =====================================================
//              FIREBASE SETTINGS
// =====================================================

#define FIREBASE_URL \
"https://agri-vision-4a713-default-rtdb.firebaseio.com"

// Firebase data location
#define FIREBASE_PATH "/esp32.json"

// =====================================================
//                    ESP32 PINOUT
// =====================================================

// KEEPING YOUR UPLOADED CODE EXACTLY
#define DHT_PIN         14
#define DHT_TYPE        DHT11

// Soil moisture sensor
#define SOIL_PIN        34

// PIR motion sensor
#define PIR_PIN         13

// Buzzer
#define BUZZER_PIN      12

// =====================================================
//                       OLED
// =====================================================

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1

Adafruit_SH1106G display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// =====================================================
//                       DHT
// =====================================================

DHT dht(
  DHT_PIN,
  DHT_TYPE
);

// =====================================================
//                 SENSOR VARIABLES
// =====================================================

float temperature = 0.0;
float humidity = 0.0;

int soilRaw = 0;
int soilMoisture = 0;

bool motionActive = false;

String dhtStatus = "ERROR";

// =====================================================
//                SOIL CALIBRATION
// =====================================================

const int SOIL_DRY = 2800;
const int SOIL_WET = 1300;

// =====================================================
//                MOTION SYSTEM
// =====================================================

bool previousMotionState = LOW;

int motionCount = 0;

unsigned long lastMotionTime = 0;

const unsigned long MOTION_GAP = 3000;

// =====================================================
//                 FIREBASE TIMING
// =====================================================

unsigned long lastFirebaseUpload = 0;

const unsigned long FIREBASE_INTERVAL = 5000;

// =====================================================
//                 WIFI CONNECTION
// =====================================================

void connectWiFi() {

  Serial.println();
  Serial.println("================================");
  Serial.println("       CONNECTING TO WIFI");
  Serial.println("================================");

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  int attempts = 0;

  while (
    WiFi.status() != WL_CONNECTED &&
    attempts < 40
  ) {

    delay(500);

    Serial.print(".");

    attempts++;
  }

  Serial.println();

  if (
    WiFi.status() == WL_CONNECTED
  ) {

    Serial.println("WiFi Connected!");

    Serial.print("IP Address: ");

    Serial.println(
      WiFi.localIP()
    );

    Serial.print("Signal RSSI: ");

    Serial.print(
      WiFi.RSSI()
    );

    Serial.println(" dBm");

  } else {

    Serial.println(
      "WiFi connection FAILED!"
    );
  }
}

// =====================================================
//                  READ SENSORS
// =====================================================

void readSensors() {

  // ---------------------------------------------------
  // DHT TEMPERATURE
  // ---------------------------------------------------

  float newTemperature =
    dht.readTemperature();

  // ---------------------------------------------------
  // DHT HUMIDITY
  // ---------------------------------------------------

  float newHumidity =
    dht.readHumidity();

  // ---------------------------------------------------
  // DHT STATUS
  // ---------------------------------------------------

  if (
    !isnan(newTemperature) &&
    !isnan(newHumidity)
  ) {

    temperature =
      newTemperature;

    humidity =
      newHumidity;

    dhtStatus =
      "OK";

  } else {

    dhtStatus =
      "ERROR";

    Serial.println(
      "DHT READ FAILED!"
    );
  }

  // ---------------------------------------------------
  // SOIL SENSOR
  // ---------------------------------------------------

  soilRaw =
    analogRead(
      SOIL_PIN
    );

  soilMoisture =
    map(
      soilRaw,
      SOIL_DRY,
      SOIL_WET,
      0,
      100
    );

  soilMoisture =
    constrain(
      soilMoisture,
      0,
      100
    );

  // ---------------------------------------------------
  // PIR MOTION
  // ---------------------------------------------------

  motionActive =
    digitalRead(
      PIR_PIN
    );

  // ---------------------------------------------------
  // SERIAL OUTPUT
  // ---------------------------------------------------

  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "          SENSOR DATA"
  );

  Serial.println(
    "================================"
  );

  Serial.print(
    "Temperature : "
  );

  if (dhtStatus == "OK") {

    Serial.print(
      temperature,
      1
    );

    Serial.println(
      " °C"
    );

  } else {

    Serial.println(
      "ERROR"
    );
  }

  Serial.print(
    "Humidity    : "
  );

  if (dhtStatus == "OK") {

    Serial.print(
      humidity,
      1
    );

    Serial.println(
      " %"
    );

  } else {

    Serial.println(
      "ERROR"
    );
  }

  Serial.print(
    "Soil Raw    : "
  );

  Serial.println(
    soilRaw
  );

  Serial.print(
    "Soil Moist. : "
  );

  Serial.print(
    soilMoisture
  );

  Serial.println(
    " %"
  );

  Serial.print(
    "Motion      : "
  );

  Serial.println(
    motionActive
      ? "DETECTED"
      : "NORMAL"
  );

  Serial.print(
    "Motion Count: "
  );

  Serial.println(
    motionCount
  );

  Serial.print(
    "DHT Status  : "
  );

  Serial.println(
    dhtStatus
  );

  Serial.println(
    "================================"
  );
}

// =====================================================
//                MOTION DETECTION
// =====================================================

void checkMotion() {

  bool currentMotion =
    digitalRead(
      PIR_PIN
    );

  // ---------------------------------------------------
  // NEW MOTION EVENT
  // ---------------------------------------------------

  if (
    currentMotion == HIGH &&
    previousMotionState == LOW
  ) {

    unsigned long currentTime =
      millis();

    // Prevent duplicate triggering
    if (
      currentTime -
      lastMotionTime >=
      MOTION_GAP
    ) {

      motionCount++;

      lastMotionTime =
        currentTime;

      Serial.println();
      Serial.println(
        "******** MOTION EVENT ********"
      );

      Serial.print(
        "Motion detection count = "
      );

      Serial.println(
        motionCount
      );

      // ------------------------------------------------
      // FIRST MOTION
      // ------------------------------------------------

      if (
        motionCount == 1
      ) {

        Serial.println(
          "FIRST MOTION DETECTED!"
        );

        // Short beep
        digitalWrite(
          BUZZER_PIN,
          HIGH
        );

        delay(300);

        digitalWrite(
          BUZZER_PIN,
          LOW
        );
      }

      // ------------------------------------------------
      // SECOND MOTION
      // ------------------------------------------------

      if (
        motionCount >= 2
      ) {

        Serial.println(
          "SECOND MOTION DETECTED!"
        );

        Serial.println(
          "Motion count will reset."
        );

        // Reset after second event
        motionCount = 0;
      }

      Serial.println(
        "********************************"
      );
    }
  }

  previousMotionState =
    currentMotion;
}

// =====================================================
//                     OLED
// =====================================================

void updateDisplay() {

  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(
    SH110X_WHITE
  );

  // ---------------------------------------------------
  // TITLE
  // ---------------------------------------------------

  display.setCursor(
    0,
    0
  );

  display.println(
    "AGRI VISION"
  );

  display.drawLine(
    0,
    10,
    127,
    10,
    SH110X_WHITE
  );

  // ---------------------------------------------------
  // TEMPERATURE
  // ---------------------------------------------------

  display.setCursor(
    0,
    16
  );

  display.print(
    "Temp: "
  );

  if (dhtStatus == "OK") {

    display.print(
      temperature,
      1
    );

    display.println(
      " C"
    );

  } else {

    display.println(
      "ERROR"
    );
  }

  // ---------------------------------------------------
  // HUMIDITY
  // ---------------------------------------------------

  display.setCursor(
    0,
    28
  );

  display.print(
    "Humidity: "
  );

  if (dhtStatus == "OK") {

    display.print(
      humidity,
      1
    );

    display.println(
      " %"
    );

  } else {

    display.println(
      "ERROR"
    );
  }

  // ---------------------------------------------------
  // SOIL
  // ---------------------------------------------------

  display.setCursor(
    0,
    40
  );

  display.print(
    "Soil: "
  );

  display.print(
    soilMoisture
  );

  display.println(
    " %"
  );

  // ---------------------------------------------------
  // MOTION
  // ---------------------------------------------------

  display.setCursor(
    0,
    53
  );

  display.print(
    "Motion: "
  );

  display.print(
    motionActive
      ? "YES"
      : "NO"
  );

  display.display();
}

// =====================================================
//              FIREBASE UPLOAD
// =====================================================

void uploadToFirebase() {

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

    if (
      WiFi.status() != WL_CONNECTED
    ) {

      Serial.println(
        "Firebase upload skipped."
      );

      return;
    }
  }

  // ---------------------------------------------------
  // BUILD JSON
  // ---------------------------------------------------

  String payload = "{";

  payload +=
    "\"temperature\":";

  if (
    dhtStatus == "OK"
  ) {

    payload +=
      String(
        temperature,
        1
      );

  } else {

    payload +=
      "0.0";
  }

  payload +=
    ",\"humidity\":";

  if (
    dhtStatus == "OK"
  ) {

    payload +=
      String(
        humidity,
        1
      );

  } else {

    payload +=
      "0.0";
  }

  payload +=
    ",\"soil_raw\":" +
    String(
      soilRaw
    );

  payload +=
    ",\"soil_moisture\":" +
    String(
      soilMoisture
    );

  payload +=
    ",\"motion_active\":" +
    String(
      motionActive
        ? "true"
        : "false"
    );

  payload +=
    ",\"motion_count\":" +
    String(
      motionCount
    );

  payload +=
    ",\"device\":\"ESP32\"";

  payload +=
    ",\"dht_status\":\"" +
    dhtStatus +
    "\"";

  payload +=
    "}";

  // ---------------------------------------------------
  // FIREBASE URL
  // ---------------------------------------------------

  String firebaseURL =
    String(FIREBASE_URL) +
    FIREBASE_PATH;

  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "       FIREBASE UPLOAD"
  );

  Serial.println(
    "================================"
  );

  Serial.println(
    "Firebase → Hydro Plantia"
  );

  Serial.println(
    payload
  );

  // ---------------------------------------------------
  // HTTPS CLIENT
  // ---------------------------------------------------

  WiFiClientSecure client;

  // Firebase HTTPS certificate verification
  // is skipped for this ESP32 prototype.
  client.setInsecure();

  HTTPClient http;

  if (
    http.begin(
      client,
      firebaseURL
    )
  ) {

    http.addHeader(
      "Content-Type",
      "application/json"
    );

    // PUT replaces the /esp32 object
    int httpCode =
      http.PUT(
        payload
      );

    Serial.print(
      "Firebase HTTP code: "
    );

    Serial.println(
      httpCode
    );

    if (
      httpCode >= 200 &&
      httpCode < 300
    ) {

      Serial.println(
        "Firebase upload SUCCESS!"
      );

    } else {

      Serial.println(
        "Firebase upload FAILED!"
      );

      Serial.println(
        http.errorToString(
          httpCode
        )
      );
    }

    http.end();

  } else {

    Serial.println(
      "Firebase connection FAILED!"
    );
  }

  Serial.println(
    "================================"
  );
}

// =====================================================
//                       SETUP
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
    "       HYDRO PLANTIA"
  );

  Serial.println(
    "       ESP32 CONTROLLER"
  );

  Serial.println(
    "================================"
  );

  // ---------------------------------------------------
  // SENSOR PINS
  // ---------------------------------------------------

  pinMode(
    PIR_PIN,
    INPUT
  );

  pinMode(
    BUZZER_PIN,
    OUTPUT
  );

  digitalWrite(
    BUZZER_PIN,
    LOW
  );

  // ---------------------------------------------------
  // DHT
  // ---------------------------------------------------

  dht.begin();

  // Give DHT time to stabilize
  delay(2000);

  // ---------------------------------------------------
  // OLED
  // ---------------------------------------------------

  Wire.begin(
    21,
    22
  );

  delay(250);

  if (
    !display.begin(
      0x3C,
      true
    )
  ) {

    Serial.println(
      "OLED initialization FAILED!"
    );

  } else {

    Serial.println(
      "OLED initialized!"
    );
  }

  // ---------------------------------------------------
  // OLED STARTUP
  // ---------------------------------------------------

  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(
    SH110X_WHITE
  );

  display.setCursor(
    0,
    0
  );

  display.println(
    "AGRI VISION"
  );

  display.println();

  display.println(
    "Starting system..."
  );

  display.display();

  // ---------------------------------------------------
  // WIFI
  // ---------------------------------------------------

  connectWiFi();

  // ---------------------------------------------------
  // READY
  // ---------------------------------------------------

  display.clearDisplay();

  display.setCursor(
    0,
    0
  );

  display.println(
    "AGRI VISION"
  );

  display.println();

  display.println(
    "System Ready"
  );

  display.println();

  if (
    WiFi.status() ==
    WL_CONNECTED
  ) {

    display.println(
      "Firebase: ONLINE"
    );

  } else {

    display.println(
      "Firebase: OFFLINE"
    );
  }

  display.display();

  // ---------------------------------------------------
  // INITIAL SENSOR READ
  // ---------------------------------------------------

  readSensors();

  updateDisplay();

  // ---------------------------------------------------
  // INITIAL FIREBASE UPLOAD
  // ---------------------------------------------------

  uploadToFirebase();

  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "     HYDRO PLANTIA READY"
  );

  Serial.println(
    "================================"
  );
}

// =====================================================
//                       LOOP
// =====================================================

void loop() {

  // ---------------------------------------------------
  // WIFI CHECK
  // ---------------------------------------------------

  if (
    WiFi.status() != WL_CONNECTED
  ) {

    connectWiFi();
  }

  // ---------------------------------------------------
  // MOTION
  // ---------------------------------------------------

  motionActive =
    digitalRead(
      PIR_PIN
    );

  // Buzzer while motion is active
  if (
    motionActive
  ) {

    digitalWrite(
      BUZZER_PIN,
      HIGH
    );

  } else {

    digitalWrite(
      BUZZER_PIN,
      LOW
    );
  }

  // Detect motion events
  checkMotion();

  // ---------------------------------------------------
  // SENSOR + OLED + FIREBASE
  // ---------------------------------------------------

  if (
    millis() -
    lastFirebaseUpload >=
    FIREBASE_INTERVAL
  ) {

    lastFirebaseUpload =
      millis();

    // Read all sensors
    readSensors();

    // Update OLED
    updateDisplay();

    // Upload directly to Firebase
    uploadToFirebase();
  }

  delay(50);
}