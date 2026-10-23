# Hydro Plantia — Netlify + Firebase

## Firebase paths

ESP32 sensor data:
- `/esp32`

ESP8266 commands:
- `/servo_control/servo1`
- `/servo_control/servo2`

ESP8266 status:
- `/servo_status`

## Deploy to Netlify

Upload this folder to Netlify as a static site. No MQTT broker or VS Code process is required for the website.

## Important

The Realtime Database rules must allow the deployed website to read `/esp32` and `/servo_status`, and write `/servo_control`.

The ESP32 and ESP8266 firmware are separate from this website and should keep their existing pin mappings.
