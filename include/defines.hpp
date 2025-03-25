#ifdef ESP32

// Analog ports
#define A0 2
#define A1 4
#define A2 35
#define A3 34
#define A4 36
#define A5 39
#define A6 32

// Digital ports
#define A7 33
#define D2 26
#define D3 25
#define D4 7
#define D5 16
#define D6 17
#define D7 14
#define D8 12
#define D9 13
#define D10 5
#define D11 23
#define D12 19
#define D13 18

// Timing defines
#define delay_toggle_led 1000
#define delay_read_adc 1000
#define I2C_clockRate 400000
#define delay_DHT20_read 5000

// Wifi
#define WIFI_SSID "22-08"
#define WIFI_PASS "414414a2"
#define delay_connect 5000
// MQTT
#define TOKEN_GATEWAY "2nxcmp7pqy5qzyskj93n"
#define MQTT_SERVER "app.coreiot.io"
#define MQTT_PORT 1883
#define delay_mqtt 100
#define MQTT_SENDING_VALUE "v1/devices/me/telemetry"
#endif