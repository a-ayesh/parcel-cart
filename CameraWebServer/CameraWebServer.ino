#include <WiFi.h>
#include <MQTT.h>
#include "esp_camera.h"

// Camera pin definitions
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define MQTT_BROKER_IP "192.168.137.24" // Your PC's IP address
#define MQTT_BROKER_PORT 1883
#define ESP32CAM_PUBLISH_TOPIC   "esp32/cam_0"

const char *ssid = "*";       // Your WiFi SSID
const char *password = "*";  // Your WiFi Password

WiFiClient net;
MQTTClient client;

void connectMQTT()
{
  Serial.print("Connecting to MQTT broker at ");
  Serial.print(MQTT_BROKER_IP);
  Serial.print(":");
  Serial.println(MQTT_BROKER_PORT);

  client.begin(MQTT_BROKER_IP, MQTT_BROKER_PORT, net);

  while (!client.connect("")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nConnected to MQTT broker");
}

void cameraInit(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA; // 640x480
  config.jpeg_quality = 10;
  config.fb_count = 2;

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
    return;
  }
}

void grabImage(){
  camera_fb_t * fb = esp_camera_fb_get();
  if(fb != NULL && fb->format == PIXFORMAT_JPEG && fb->len < 1000000){
    Serial.print("Image Length: ");
    Serial.print(fb->len);
    Serial.print("\t Publish Image: ");
    bool result = client.publish(ESP32CAM_PUBLISH_TOPIC, (const char*)fb->buf, fb->len);
    Serial.println(result ? "Success" : "Fail");

    if(!result){
      Serial.println("Publish failed, reconnecting...");
      client.disconnect();
      connectMQTT();
    }
  }
  esp_camera_fb_return(fb);
  // delay(1000); // Publish every second; adjust as needed
}

void setup() {
  Serial.begin(115200);
  cameraInit();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected");

  // Display the ESP32's IP address
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // Connect to MQTT Broker
  client.begin(MQTT_BROKER_IP, MQTT_BROKER_PORT, net);
  connectMQTT();
}

void loop() {
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();
  grabImage();
}