#include <WiFi.h>
#include <WebServer.h>
#include <TinyGPS++.h>

// Wi-Fi credentials
const char* ssid = "Leap of Faith"; // Replace with your Wi-Fi SSID
const char* password = "20010816akd"; // Replace with your Wi-Fi password

// Google Maps API Key
const char* googleMapsApiKey = "********"; // Replace with your API key
const char* mapId = "*******"; // Replace with your Map ID

// Default coordinates
const double DEFAULT_LAT = 23.798157743491537;
const double DEFAULT_LON = 90.44976364044352;

// Initialize WebServer on port 80
WebServer server(80);

// Initialize TinyGPS++ object
TinyGPSPlus gps;

// GPS data variables
double latitude = DEFAULT_LAT;
double longitude = DEFAULT_LON;
float speedKmph = 0.0;
bool gpsFix = false;

// Buzzer pin
#define BUZZER_PIN 18 // GPIO18 for buzzer (positive leg to D18, negative to GND)

// Serial2 for GPS (RX2: GPIO16, TX2: GPIO17)
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17

// Buzzer control variables
bool buzzerActive = false;
unsigned long buzzerStartTime = 0;
const unsigned long BUZZER_DURATION = 5000; // 5 seconds

// Emergency alerts storage
struct Alert {
  unsigned long id;
  double lat;
  double lng;
  String localTime; // Store formatted local time
};
Alert alerts[50]; // Max 50 alerts
int alertCount = 0;
unsigned long nextAlertId = 1;

// Function to get local time (UTC+6)
String getLocalTime() {
  if (gps.time.isValid() && gps.date.isValid()) {
    // Use GPS time if available
    int hour = gps.time.hour();
    int minute = gps.time.minute();
    int second = gps.time.second();
    // Adjust for UTC+6
    hour = (hour + 6) % 24;
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %s, %04d-%02d-%02d",
            hour > 12 ? hour - 12 : hour, minute, second, hour >= 12 ? "PM" : "AM",
            gps.date.year(), gps.date.month(), gps.date.day());
    return String(timeStr);
  } else {
    // Fallback to system time (approximate, assuming millis() starts at boot)
    unsigned long seconds = millis() / 1000;
    int hour = (seconds / 3600 + 6) % 24; // UTC+6
    int minute = (seconds % 3600) / 60;
    int second = seconds % 60;
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %s", 
            hour > 12 ? hour - 12 : hour, minute, second, hour >= 12 ? "PM" : "AM");
    return String(timeStr);
  }
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Buzzer off (LOW) at startup

  Serial.begin(115200);
  while (!Serial);

  Serial2.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  int wifiTimeout = 30;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout > 0) {
    delay(1000);
    Serial.print(".");
    wifiTimeout--;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi.");
    return;
  }

  server.on("/", handleRoot);
  server.on("/getLocation", handleGetLocation);
  server.on("/triggerBuzzer", handleTriggerBuzzer);
  server.on("/resolveAlert", handleResolveAlert);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  // Read GPS data
  unsigned long startTime = millis();
  bool dataReceived = false;
  String nmeaBuffer = "";

  while (Serial2.available() && millis() - startTime < 2000) {
    char c = Serial2.read();
    nmeaBuffer += c;
    if (gps.encode(c)) {
      dataReceived = true;
      if (gps.location.isValid()) {
        latitude = gps.location.lat();
        longitude = gps.location.lng();
        gpsFix = true;
        Serial.print("GPS Fix: Lat=");
        Serial.print(latitude, 6);
        Serial.print(", Lon=");
        Serial.println(longitude, 6);
      } else {
        gpsFix = false;
        latitude = DEFAULT_LAT;
        longitude = DEFAULT_LON;
        Serial.println("No GPS fix, using default coordinates");
      }
      if (gps.speed.isValid()) {
        speedKmph = gps.speed.kmph();
        Serial.print("Speed: ");
        Serial.print(speedKmph);
        Serial.println(" km/h");
      } else {
        speedKmph = 0.0;
        Serial.println("No valid speed data");
      }
      Serial.print("Satellites: ");
      Serial.println(gps.satellites.value());
      Serial.print("HDOP: ");
      Serial.println(gps.hdop.isValid() ? gps.hdop.value() / 100.0 : -1);
      Serial.println("Raw NMEA (last 100 chars): ");
      int startIndex = max(0, (int)(nmeaBuffer.length() - 100));
      Serial.println(nmeaBuffer.substring(startIndex));
    }
  }

  if (!dataReceived && millis() - startTime >= 2000) {
    Serial.println("No GPS data received. Check connections.");
    Serial.println("Raw NMEA (last 100 chars): ");
    int startIndex = max(0, (int)(nmeaBuffer.length() - 100));
    Serial.println(nmeaBuffer.substring(startIndex));
  }

  // Control buzzer
  if (buzzerActive && (millis() - buzzerStartTime >= BUZZER_DURATION)) {
    digitalWrite(BUZZER_PIN, LOW); // Turn buzzer off
    buzzerActive = false;
    Serial.println("Buzzer turned off");
  }

  server.handleClient();
}

// Handle root URL
void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html lang='en'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>UIU Shuttle Bus Tracker - Location of Bus-1</title>";
  html += "<script src='https://cdn.tailwindcss.com'></script>";
  html += "</head>";
  html += "<body class='bg-gray-100 font-sans'>";
  html += "<div class='min-h-screen flex flex-col items-center p-4'>";
  html += "<h1 class='text-3xl font-bold text-gray-800 mb-6'>UIU Shuttle Bus Tracker - Location of Bus-1</h1>";
  html += "<div class='w-full max-w-4xl flex flex-col md:flex-row gap-4'>";
  html += "<div class='flex-1'>";
  html += "<div id='map' class='w-full h-96 rounded-lg shadow-lg mb-4'></div>";
  html += "<div class='w-full bg-white p-6 rounded-lg shadow-md'>";
  html += "<p id='status' class='text-gray-600 text-center mb-4'>Waiting for GPS fix...</p>";
  html += "<p id='speed' class='text-gray-600 text-center mb-4'>Speed: 0.00 km/h</p>";
  html += "<p id='error' class='text-red-500 text-center hidden'>Failed to load map. Check API key, Map ID, or internet connection.</p>";
  html += "<button id='emergencyBtn' onclick='triggerBuzzer()' class='w-full bg-red-500 text-white py-2 px-4 rounded-lg hover:bg-red-600 transition duration-300'>Emergency Alert</button>";
  html += "</div>";
  html += "</div>";
  html += "<div class='flex-1'>";
  html += "<h2 class='text-2xl font-semibold text-gray-800 mb-4'>Emergency Alerts</h2>";
  html += "<div id='emergencyMessages' class='w-full bg-white p-6 rounded-lg shadow-md'>";
  for (int i = alertCount - 1; i >= 0; i--) {
    html += "<div id='message-" + String(alerts[i].id) + "' class='mb-4 p-4 bg-red-100 text-red-700 rounded-lg'>";
    html += "<p>Emergency Alert Triggered at " + alerts[i].localTime + "! Location: " + String(alerts[i].lat, 6) + ", " + String(alerts[i].lng, 6) + "</p>";
    html += "<button onclick='resolveMessage(" + String(alerts[i].id) + ")' class='mt-2 bg-green-500 text-white py-1 px-3 rounded hover:bg-green-600'>Resolved</button>";
    html += "</div>";
  }
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "<script>";
  html += "function initMap() {";
  html += "  try {";
  html += "    const map = new google.maps.Map(document.getElementById('map'), {";
  html += "      zoom: 15,";
  html += "      center: { lat: " + String(DEFAULT_LAT, 6) + ", lng: " + String(DEFAULT_LON, 6) + " },";
  html += "      mapId: '" + String(mapId) + "'";
  html += "    });";
  html += "    let marker = new google.maps.marker.AdvancedMarkerElement({";
  html += "      map: map,";
  html += "      position: { lat: " + String(DEFAULT_LAT, 6) + ", lng: " + String(DEFAULT_LON, 6) + " }";
  html += "    });";
  html += "    document.getElementById('error').classList.add('hidden');";
  html += "    function updateLocation() {";
  html += "      fetch('/getLocation')";
  html += "        .then(response => response.json())";
  html += "        .then(data => {";
  html += "          const latlng = { lat: data.lat, lng: data.lng };";
  html += "          map.setCenter(latlng);";
  html += "          marker.position = latlng;";
  html += "          document.getElementById('status').innerText = `GPS Location: ${data.lat.toFixed(6)}, ${data.lng.toFixed(6)}`;";
  html += "          document.getElementById('speed').innerText = `Speed: ${data.speed.toFixed(2)} km/h`;";
  html += "        })";
  html += "        .catch(error => {";
  html += "          console.error('Error fetching location:', error);";
  html += "          document.getElementById('status').innerText = 'Error fetching location';";
  html += "          document.getElementById('error').classList.remove('hidden');";
  html += "        });";
  html += "    }";
  html += "    updateLocation();";
  html += "    setInterval(updateLocation, 5000);";
  html += "    window.triggerBuzzer = function() {";
  html += "      fetch('/triggerBuzzer')";
  html += "        .then(response => response.json())";
  html += "        .then(data => {";
  html += "          if (data.status === 'success') {";
  html += "            document.getElementById('emergencyBtn').disabled = true;";
  html += "            document.getElementById('emergencyBtn').innerText = 'Alert Triggered (5s)';";
  html += "            setTimeout(() => {";
  html += "              document.getElementById('emergencyBtn').disabled = false;";
  html += "              document.getElementById('emergencyBtn').innerText = 'Emergency Alert';";
  html += "            }, 5000);";
  html += "            const messagesContainer = document.getElementById('emergencyMessages');";
  html += "            const messageDiv = document.createElement('div');";
  html += "            messageDiv.id = `message-${data.id}`;";
  html += "            messageDiv.className = 'mb-4 p-4 bg-red-100 text-red-700 rounded-lg';";
  html += "            messageDiv.innerHTML = `";
  html += "              <p>Emergency Alert Triggered at ${data.localTime}! Location: ${data.lat.toFixed(6)}, ${data.lng.toFixed(6)}</p>";
  html += "              <button onclick='resolveMessage(${data.id})' class='mt-2 bg-green-500 text-white py-1 px-3 rounded hover:bg-green-600'>Resolved</button>";
  html += "            `;";
  html += "            messagesContainer.prepend(messageDiv);";
  html += "          }";
  html += "        })";
  html += "        .catch(error => console.error('Error triggering buzzer:', error));";
  html += "    };";
  html += "    window.resolveMessage = function(id) {";
  html += "      fetch('/resolveAlert?id=' + id)";
  html += "        .then(response => response.json())";
  html += "        .then(data => {";
  html += "          if (data.status === 'success') {";
  html += "            const messageDiv = document.getElementById(`message-${id}`);";
  html += "            if (messageDiv) messageDiv.remove();";
  html += "          }";
  html += "        })";
  html += "        .catch(error => console.error('Error resolving alert:', error));";
  html += "    };";
  html += "  } catch (e) {";
  html += "    console.error('Map initialization failed:', e);";
  html += "    document.getElementById('map').classList.add('hidden');";
  html += "    document.getElementById('error').classList.remove('hidden');";
  html += "  }";
  html += "}";
  html += "</script>";
  html += "<script src='https://maps.googleapis.com/maps/api/js?key=" + String(googleMapsApiKey) + "&libraries=marker&callback=initMap&loading=async' async></script>";
  html += "</body>";
  html += "</html>";

  server.send(200, "text/html", html);
}

// Handle /getLocation URL
void handleGetLocation() {
  String json = "{";
  json += "\"fix\":" + String(gpsFix ? "true" : "false") + ",";
  json += "\"lat\":" + String(latitude, 6) + ",";
  json += "\"lng\":" + String(longitude, 6) + ",";
  json += "\"speed\":" + String(speedKmph, 2);
  json += "}";
  server.send(200, "application/json", json);
}

// Handle /triggerBuzzer URL
void handleTriggerBuzzer() {
  if (!buzzerActive) {
    digitalWrite(BUZZER_PIN, HIGH); // Turn buzzer on
    buzzerActive = true;
    buzzerStartTime = millis();
    Serial.println("Buzzer turned on");
    if (alertCount < 50) {
      alerts[alertCount].id = nextAlertId++;
      alerts[alertCount].lat = latitude;
      alerts[alertCount].lng = longitude;
      alerts[alertCount].localTime = getLocalTime();
      alertCount++;
    }
    String json = "{";
    json += "\"status\":\"success\",";
    json += "\"id\":" + String(alerts[alertCount - 1].id) + ",";
    json += "\"lat\":" + String(latitude, 6) + ",";
    json += "\"lng\":" + String(longitude, 6) + ",";
    json += "\"localTime\":\"" + alerts[alertCount - 1].localTime + "\"";
    json += "}";
    server.send(200, "application/json", json);
  } else {
    server.send(200, "application/json", "{\"status\":\"already_active\"}");
  }
}

// Handle /resolveAlert URL
void handleResolveAlert() {
  if (server.hasArg("id")) {
    unsigned long id = server.arg("id").toInt();
    for (int i = 0; i < alertCount; i++) {
      if (alerts[i].id == id) {
        for (int j = i; j < alertCount - 1; j++) {
          alerts[j] = alerts[j + 1];
        }
        alertCount--;
        server.send(200, "application/json", "{\"status\":\"success\"}");
        return;
      }
    }
  }
  server.send(400, "application/json", "{\"status\":\"invalid_id\"}");
}
