#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

// Pin configuration
const int devicePins[4] = {5, 18, 19, 21}; // You can change as needed

// Access Point credentials
const char* ssid = "HomeAuto_AP";
const char* password = "12345678";  // Min. 8 characters

// DNS and WebServer setup
DNSServer dnsServer;
WebServer server(80);

const byte DNS_PORT = 53;

String getDeviceStatusJSON() {
  String json = "{";
  for (int i = 0; i < 4; i++) {
    json += "\"device" + String(i + 1) + "\":" + String(digitalRead(devicePins[i]));
    if (i < 3) json += ",";
  }
  json += "}";
  return json;
}


// HTML page
String getHTML() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Home Automation</title>
    <style>
      body {
        margin: 0;
        padding: 0;
        background: linear-gradient(to bottom right, #0f2027, #203a43, #2c5364);
        font-family: 'Segoe UI', sans-serif;
        text-align: center;
        color: white;
      }

      h1 {
        font-size: 32px;
        margin-top: 30px;
        margin-bottom: 10px;
        text-shadow: 2px 2px 4px #000;
      }

      h2 {
        font-size: 16px;
        font-style: italic;
        color: #ccc;
        margin-bottom: 30px;
      }

      .device-card {
        background-color: #ffffff;
        border-radius: 20px;
        padding: 25px 20px;
        margin: 25px auto;
        width: 85%;
        max-width: 360px;
        box-shadow: 0 6px 18px rgba(0, 0, 0, 0.4);
        color: #000;
        position: relative;
        transition: transform 0.2s ease;
      }

      .device-card:hover {
        transform: scale(1.01);
      }

      .device-label {
        font-size: 22px;
        font-weight: bold;
        margin-bottom: 20px;
      }

      .toggle-btn {
        width: 100%;
        padding: 14px 0;
        font-size: 18px;
        font-weight: bold;
        color: white;
        border: none;
        border-radius: 30px;
        cursor: pointer;
        transition: background-color 0.4s ease, transform 0.2s;
        box-shadow: 0 4px 10px rgba(0, 0, 0, 0.2);
      }

      .toggle-btn:active {
        transform: scale(0.98);
      }

      .on {
        background-color: #28a745;
      }

      .off {
        background-color: #dc3545;
      }
    </style>
  <script>
  function updateUI(status) {
    for (let i = 1; i <= 4; i++) {
      const btn = document.getElementById('btn' + i);
      const isOn = status['device' + i] === 1;

      btn.classList.toggle('on', isOn);
      btn.classList.toggle('off', !isOn);
      btn.innerText = isOn ? 'TURN OFF' : 'TURN ON';
    }
  }

  function fetchStatus() {
    fetch('/status')
      .then(res => res.json())
      .then(data => updateUI(data))
      .catch(err => console.log('Error fetching status:', err));
  }

  function toggleDevice(deviceId) {
    const btn = document.getElementById('btn' + deviceId);
    const isOn = btn.classList.contains('on');
    const command = isOn ? '/off' + deviceId : '/on' + deviceId;

    fetch(command)
      .then(() => fetchStatus())  // Refresh UI after toggle
      .catch(err => console.log('Toggle error:', err));
  }

  window.onload = () => {
    fetchStatus();                    // Initial fetch
    setInterval(fetchStatus, 2000);  // Auto-sync every 2 seconds
  };  // Sync UI on page load
  </script>
  </head>
  <body>
    <h1>HOME AUTOMATION</h1>
    <h2>Make Your House Smart!</h2>

    <div class='device-card'>
      <div class='device-label'>DEVICE 1</div>
      <button id='btn1' class='toggle-btn off' onclick='toggleDevice(1)'>TURN ON</button>
    </div>

    <div class='device-card'>
      <div class='device-label'>DEVICE 2</div>
      <button id='btn2' class='toggle-btn off' onclick='toggleDevice(2)'>TURN ON</button>
    </div>

    <div class='device-card'>
      <div class='device-label'>DEVICE 3</div>
      <button id='btn3' class='toggle-btn off' onclick='toggleDevice(3)'>TURN ON</button>
    </div>

    <div class='device-card'>
      <div class='device-label'>DEVICE 4</div>
      <button id='btn4' class='toggle-btn off' onclick='toggleDevice(4)'>TURN ON</button>
    </div>

  </body>
  </html>
  )rawliteral";
  return html;
}





// Handle root page
void handleRoot() {
  server.send(200, "text/html", getHTML());
}

// Handle 8 endpoints for ON/OFF
void handleDevice(int device, bool state) {
  if (device >= 1 && device <= 4) {
    digitalWrite(devicePins[device - 1], state ? HIGH : LOW);
  }
  server.send(200, "text/html", getHTML());
}

void setup() {
  // Set pins as outputs
  for (int i = 0; i < 4; i++) {
    pinMode(devicePins[i], OUTPUT);
    digitalWrite(devicePins[i], LOW); // All OFF initially
  }

  server.on("/status", []() {
  server.send(200, "application/json", getDeviceStatusJSON());
});


  Serial.begin(115200);

  // Start WiFi in AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // Start DNS server
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Route DNS to our root
  
  server.on("/", handleRoot);

// Captive portal detection URLs
server.on("/generate_204", handleRoot);        // Android
server.on("/gen_204", handleRoot);             // Android
server.on("/redirect", handleRoot);            // Android
server.on("/hotspot-detect.html", handleRoot); // iOS/macOS
server.on("/ncsi.txt", handleRoot);            // Windows

server.onNotFound(handleRoot);  // Redirect all unknown URLs to root


  // Create handlers for ON/OFF
  for (int i = 1; i <= 4; i++) {
    server.on(("/on" + String(i)).c_str(), [i]() {
      handleDevice(i, true);
    });
    server.on(("/off" + String(i)).c_str(), [i]() {
      handleDevice(i, false);
    });
  }

  // Start web server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
