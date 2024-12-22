from fastapi import FastAPI, Response
from fastapi.responses import HTMLResponse
import paho.mqtt.client as mqtt
import threading

app = FastAPI()

# MQTT Broker Configuration
MQTT_BROKER = "localhost"  # Use "192.168.137.24" if running on a different machine
MQTT_PORT = 1883
MQTT_TOPIC = "esp32/cam_0"

# Variable to store the latest image
latest_image = None
image_lock = threading.Lock()

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker")
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    global latest_image
    with image_lock:
        latest_image = msg.payload
    print(f"Received image of size {len(msg.payload)} bytes")

def mqtt_loop():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_forever()

# Start MQTT client in a separate daemon thread
mqtt_thread = threading.Thread(target=mqtt_loop)
mqtt_thread.daemon = True
mqtt_thread.start()

# HTML frontend with JavaScript to refresh the image periodically
html_content = """
<!DOCTYPE html>
<html>
<head>
    <title>ESP32-CAM Live Stream</title>
    <style>
        body { text-align: center; }
        img { max-width: 100%; height: auto; }
    </style>
    <script>
        function updateImage() {
            var img = document.getElementById("cam_image");
            img.src = "/latest_image?" + new Date().getTime();
        }
        setInterval(updateImage, 1); // Update every second
    </script>
</head>
<body>
    <h1>ESP32-CAM Live Stream</h1>
    <img id="cam_image" src="/latest_image" alt="Camera Feed">
</body>
</html>
"""

@app.get("/", response_class=HTMLResponse)
def read_root():
    return html_content

@app.get("/latest_image")
def get_latest_image():
    with image_lock:
        if latest_image:
            return Response(content=latest_image, media_type="image/jpeg")
        else:
            return Response(content=b"", media_type="image/jpeg")
