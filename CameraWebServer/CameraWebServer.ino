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
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 20;
  config.fb_count = 2;

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  if (s != NULL) {
    // Example “make it brighter / more visible” adjustments.
    // Tweak the -2..2 or 0..2 parameters as you see fit.
    s->set_brightness(s, 1);       // -2 to 2
    // s->set_contrast(s, 1);         // -2 to 2
    s->set_saturation(s, 2);       // -2 to 2
    // s->set_special_effect(s, 0);   // 0 = no effect
    // s->set_whitebal(s, 1);         // enable/disable
    // s->set_awb_gain(s, 1);         // enable/disable AWB gain
    // s->set_wb_mode(s, 0);          // 0: Auto; 1: Sunny; 2: Cloudy; 3: Office; 4: Home
    s->set_exposure_ctrl(s, 1);    // enable auto-exposure
    // s->set_aec2(s, 1);             // new AEC algorithm
    s->set_ae_level(s, 2);         // -2 to +2
    // If you want a bit longer/slower exposure (brighter in low light):
    // s->set_aec_value(s, 500);    // 0 to 1200 (only if auto-exposure is disabled)
    // s->set_gain_ctrl(s, 1);        // enable auto-gain
    // s->set_agc_gain(s, 15);      // set manual gain if auto-gain disabled
    // s->set_gainceiling(s, (gainceiling_t)0);
    // s->set_bpc(s, 0);              // black pixel correction
    // s->set_wpc(s, 1);              // white pixel correction
    // s->set_raw_gma(s, 1);
    // s->set_lenc(s, 1);             // lens correction
    // s->set_hmirror(s, 0);
    // s->set_vflip(s, 0);
    // s->set_dcw(s, 1);
    // s->set_colorbar(s, 0);
  }
  else {
    Serial.println("Failed to get sensor_t object.");
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
  delay(100); // Publish every 100 ms
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