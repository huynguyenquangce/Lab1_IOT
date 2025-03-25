#include "../include/global.hpp"

// put function declarations here:

// MQTT
WiFiClient espClient;
PubSubClient client(espClient);
void reconnectMQTT()
{
  while (!client.connected())
  {
    Serial.print("Connecting to MQTT...");

    // String clientId = "ESP32Client" + String(random(0, 1000));
    String clientId = "pole_1";
    // if (client.connect(clientId.c_str(), user.c_str(), password.c_str()))
    if (client.connect(clientId.c_str(), TOKEN_GATEWAY, ""))
    {
      Serial.print("MQTT Connected");

// Subscribe to topic put in here
#ifdef _ESP_NUMBER_ONE_
      client.subscribe(MQTT_REQUEST_TOPIC);
      printlnData("Successfully subscribe topic");
#endif

      Serial.println("Start");
    }
    else
    {
      Serial.print("MQTT connection failed, rc=");
      Serial.print(String(client.state()));
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

void taskMQTT(void *pvParameters)
{
  // Wait connecting Wifi
  while (WiFi.status() != WL_CONNECTED)
  {
    vTaskDelay(delay_connect / portTICK_PERIOD_MS);
  }
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setKeepAlive(30);
  // client.setCallback(callback);
#ifdef _ESP_NUMBER_ONE_
  client.subscribe(MQTT_REQUEST_TOPIC);
#endif

  while (true)
  {
    if (!client.connected())
    {
      reconnectMQTT();
    }

    client.loop();
    vTaskDelay(delay_mqtt / portTICK_PERIOD_MS);
  }
}

bool publishData(String feedName, String message)
{
  String topic = feedName;
#ifdef ADAFRUIT
  String topic = user + "/feeds/" + feedName;
#endif
  Serial.print("Publishing to topic: ");
  Serial.print(feedName);
  Serial.print("Status: ");

  if (client.publish(topic.c_str(), message.c_str(), 1))
  {
    Serial.printf("Success!");
    return 1;
  }
  Serial.printf("Failed!");
  return 0;
}
// Wifi
void taskWifi(void *pvParameters)
{
  WiFi.mode(WIFI_STA);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int wifiRetryCount = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetryCount < 20)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
    wifiRetryCount++;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP.restart();
  }

  Serial.print("[INFO] WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.print(String(WiFi.localIP()));

  while (true)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.print("[WARN] Lost WiFi! Attempting to reconnect...");
      int retryCount = 0;

      while (WiFi.status() != WL_CONNECTED && retryCount < 20)
      {
        vTaskDelay(pdMS_TO_TICKS(1000));
        retryCount++;
      }

      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.print("[INFO] WiFi Reconnected!");
      }
      else
      {
        Serial.print("[ERROR] WiFi reconnect failed. Restarting in 5s...");
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP.restart();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
int flag = 0;
void ledToggle(void *pvParam)
{
  while (true)
  {
    if (flag)
    {
      digitalWrite(A0, 1);
      Serial.println("ON");
      flag = 0;
    }
    else
    {
      digitalWrite(A0, 0);
      Serial.println("OFF");
      flag = 1;
    }
    vTaskDelay(pdMS_TO_TICKS(delay_toggle_led));
  }
}

void sensorLight(void *pvParam)
{
  while (true)
  {
    uint32_t val = analogRead(A1);
    Serial.println(val);
    vTaskDelay(pdMS_TO_TICKS(delay_read_adc));
  }
}

DHT20 dht20;
StaticJsonDocument<300> doc;
void readDHT20(void *pvParam)
{
  vTaskDelay(pdMS_TO_TICKS(1000));
  DHT20 *dht = (DHT20 *)pvParam;
  uint8_t count = 0;
  while (true)
  {
    // READ DATA
    int status = dht->read();
    if (count % 5 == 0)
    {
      count = 0;
      Serial.print('\n');
      Serial.println("Type\tHumidity (%)\tTemp (°C)\tStatus");
    }
    count++;
    //
    // Print to Serial
    Serial.print("DHT20 \t");
    //  DISPLAY DATA, sensor has only one decimal.
    Serial.print(dht->getHumidity(), 1);
    String a(dht->getHumidity(), 1);
    Serial.print("\t\t");
    Serial.print(dht->getTemperature(), 1);
    Serial.print("\t\t");
    switch (status)
    {
    case DHT20_OK:
      Serial.print("OK");
      break;
    case DHT20_ERROR_CHECKSUM:
      Serial.print("Checksum error");
      break;
    case DHT20_ERROR_CONNECT:
      Serial.print("Connect error");
      break;
    case DHT20_MISSING_BYTES:
      Serial.print("Missing bytes");
      break;
    case DHT20_ERROR_BYTES_ALL_ZERO:
      Serial.print("All bytes read zero");
      break;
    case DHT20_ERROR_READ_TIMEOUT:
      Serial.print("Read time out");
      break;
    case DHT20_ERROR_LASTREAD:
      Serial.print("Error read too fast");
      break;
    default:
      Serial.print("Unknown error");
      break;
    }
    Serial.println();

    doc["humidity"] = dht->getHumidity();
    doc["temperature"] = dht->getTemperature();

    char buffer[256];
    serializeJson(doc, buffer);

    // Publish the data to MQTT
    publishData(MQTT_SENDING_VALUE, buffer);
    // }
    vTaskDelay(pdMS_TO_TICKS(delay_DHT20_read));
  }
}
void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(A0, OUTPUT); // For LED
  // pinMode(A1, INPUT); // For DHT11 ...
  Wire.begin(21, 22); // For I2C DHT20
  DHT20 *dht = new DHT20(&Wire);
  if (!dht->begin())
  {
    Serial.println("Failed to initialize DHT20 sensor!");
    while (1)
      ;
  }
  Serial.println("DHT20 initialized successfully.");
  delay(2000);

  // Add task
  xTaskCreate(readDHT20, "dddd", 4096, dht, 1, nullptr);
  // xTaskCreate(readDHT20, "Read DHT20", 1024*4, nullptr, 1, nullptr);
  // xTaskCreate(sensorLight, "Read Light Sensor", 4096, nullptr, 1, nullptr);
  xTaskCreate(taskWifi, "Task Wifi", 4096, nullptr, 0, nullptr);
  // xTaskCreate(ledToggle, "Blinky Led", 4096, nullptr, 0, nullptr);
  xTaskCreate(taskMQTT, "Task MQTT", 4096, nullptr, 1, nullptr);
}

// uint8_t count = 0;
// DHT20 DHT;
// void loop()
// {
//   if (millis() - DHT.lastRead() >= 1000)
//   {
//     //  READ DATA
//     uint32_t start = micros();
//     int status = DHT.read();
//     uint32_t stop = micros();

//     if ((count % 10) == 0)
//     {
//       count = 0;
//       Serial.println();
//       Serial.println("Type\tHumidity (%)\tTemp (°C)\tTime (µs)\tStatus");
//     }
//     count++;

//     Serial.print("DHT20 \t");
//     //  DISPLAY DATA, sensor has only one decimal.
//     Serial.print(DHT.getHumidity(), 1);
//     Serial.print("\t\t");
//     Serial.print(DHT.getTemperature(), 1);
//     Serial.print("\t\t");
//     Serial.print(stop - start);
//     Serial.print("\t\t");
//     switch (status)
//     {
//       case DHT20_OK:
//         Serial.print("OK");
//         break;
//       case DHT20_ERROR_CHECKSUM:
//         Serial.print("Checksum error");
//         break;
//       case DHT20_ERROR_CONNECT:
//         Serial.print("Connect error");
//         break;
//       case DHT20_MISSING_BYTES:
//         Serial.print("Missing bytes");
//         break;
//       case DHT20_ERROR_BYTES_ALL_ZERO:
//         Serial.print("All bytes read zero");
//         break;
//       case DHT20_ERROR_READ_TIMEOUT:
//         Serial.print("Read time out");
//         break;
//       case DHT20_ERROR_LASTREAD:
//         Serial.print("Error read too fast");
//         break;
//       default:
//         Serial.print("Unknown error");
//         break;
//     }
//     Serial.print("\n");
//   }
// }

void loop()
{
  // do nothing
}
