#include <ESPAsyncWebServer.h>
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPAsyncTCP.h>

// DHT22 settings
#define DHTPIN D6  // GPIO12 (D6 pin)
#define DHTTYPE DHT11
#define LDRPIN A0  // Pin for LDR (Analog Pin)
#define LEDPIN D7  // GPIO2 (D4 pin for LED control)

WiFiClient wifiClient;
AsyncWebServer server(80);
DHT dht(DHTPIN, DHTTYPE); // DHT object

String wifi_name = "admin", pass = "password";
// Wi-Fi credentials
const char* ssid = "vivo V21e 5G";
const char* password = "123456789";

// OpenWeather API and ThingSpeak settings
const String apiKey = "4317ba8f763c13b5e4c13118a8c8d9fd";
const String cityName = "Anand,IN";  // Use city and country code for India
const String cloudPlatformURL = "http://api.thingspeak.com/update?api_key=MEA32GARX71N0SCS";

const String flaskServerURL = "http://192.168.149.102:5000/predict";  // Example: http://192.168.1.4:5000/predict

float temperature = 0, humidity = 0, dewPoint = 0, sunshine = 0;
float pressure = 0, cloud = 0, windspeed = 0, winddirection = 0;
float lastTemperature = 0, lastHumidity = 0;
float probability_of_rainfall = 0 ;
int prediction = 0;
String summary = "No Rainfall";
bool ledState = false; // Store the state of the LED

// Function to calculate dew point
float calculateDewPoint(float temp, float hum) {
  float a = 17.27;
  float b = 237.7;
  float gamma = (a * temp) / (b + temp) + log(hum / 100.0);
  return (b * gamma) / (a - gamma);
}

// Function to simulate sunshine value from LDR
float readSunshine() {
  int ldrValue = analogRead(LDRPIN);  // Read value from LDR (0-1024)
  float sunshinePercentage = map(ldrValue, 0, 1024, 0, 100);  // Map to 0-100%
  return sunshinePercentage;
}

// Function to fetch OpenWeather data
void fetchOpenWeatherData() {
  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" + cityName + "&appid=" + apiKey + "&units=metric";
  
  http.begin(wifiClient, url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, payload);
    
    pressure = doc["main"]["pressure"];
    cloud = doc["clouds"]["all"];
    windspeed = doc["wind"]["speed"];
    winddirection = doc["wind"]["deg"];
  } else {
    Serial.println("Error fetching weather data");
  }
  http.end();
}

// Function to upload data to ThingSpeak
void uploadToThingSpeak() {
  HTTPClient http;
  String url = cloudPlatformURL
               + "&field1=" + String(temperature)  // Field 1: Temperature
               + "&field2=" + String(humidity)     // Field 2: Humidity
               + "&field3=" + String(pressure)     // Field 3: Pressure
               + "&field4=" + String(dewPoint)     // Field 4: Dew Point (Replaces maxtemp)
               + "&field5=" + String(cloud)        // Field 5: Cloud
               + "&field6=" + String(windspeed)    // Field 6: Wind Speed
               + "&field7=" + String(winddirection)// Field 7: Wind Direction
               + "&field8=" + String(sunshine);    // Field 8: Sunshine (Replaces mintemp)

  http.begin(wifiClient, url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.println("ThingSpeak Data uploaded: " + http.getString());
  } else {
    Serial.println("Error uploading data to ThingSpeak");
  }
  http.end();
}

// Function to upload data to Flask server
void uploadToFlaskServer() {
  HTTPClient http;
  http.begin(wifiClient, flaskServerURL);
  http.addHeader("Content-Type", "application/json");

  // Create JSON payload
  StaticJsonDocument<256> jsonDoc;
  jsonDoc["temperature"] = temperature;
  jsonDoc["humidity"] = humidity;
  jsonDoc["pressure"] = pressure;
  jsonDoc["dewPoint"] = dewPoint;
  jsonDoc["cloud"] = cloud;
  jsonDoc["windspeed"] = windspeed;
  jsonDoc["winddirection"] = winddirection;
  jsonDoc["sunshine"] = sunshine;

  String payload;
  Serial.println(payload);
  serializeJson(jsonDoc, payload);
  Serial.println(payload);
  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    Serial.println("Flask Server Data uploaded: " + http.getString());
    String payload = http.getString();
    Serial.println(payload);
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, payload);
    
    prediction = doc["prediction"];
    if (prediction ==0){
      summary = "No Rainfall Today";
    }else{
      summary = "There is High Probability of Rainfall Today";
    }
    probability_of_rainfall = doc["probability_of_rainfall"];
  } else {
    Serial.println("Error uploading data to Flask server" + String(httpCode) + http.getString());
  }
  http.end();
}

// Function to send HTML for webpage with IP display
String sendHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">";
  html += "<meta http-equiv=\"refresh\" content=\"60\">";  // Auto-refresh every 60 seconds
  html += "<title>ESP8266 Weather Report</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background-color: #f4f4f4; margin: 0; padding: 20px; }";
  html += "h1 { color: #333; }";
  html += "p { margin: 10px 0; padding: 10px; background: #fff; border-radius: 5px; }";
  html += "table { width: 100%; border-collapse: collapse; margin: 20px 0; }";
  html += "th, td { border: 1px solid #ddd; padding: 8px; text-align: center; }";
  html += "th { background-color: #4CAF50; color: white; }";
  html += "h1 { text-align: center; color: #333; }";
  html += "</style>";
  html += "</head><body>";

  html += "<h1>ESP8266 Weather Report</h1>";
  
  html += "<table>";
  html += "<tr><th>Parameter</th><th>Value</th></tr>";
  html += "<tr><td>Temperature</td><td>" + String(temperature) + " °C</td></tr>";
  html += "<tr><td>Humidity</td><td>" + String(humidity) + " %</td></tr>";
  html += "<tr><td>Dew Point</td><td>" + String(dewPoint) + " °C</td></tr>";
  html += "<tr><td>Pressure</td><td>" + String(pressure) + " hPa</td></tr>";
  html += "<tr><td>Cloud</td><td>" + String(cloud) + " %</td></tr>";
  html += "<tr><td>Sunshine</td><td>" + String(sunshine) + " %</td></tr>";
  html += "<tr><td>Wind Speed</td><td>" + String(windspeed) + " m/s</td></tr>";
  html += "<tr><td>Wind Direction</td><td>" + String(winddirection) + "°</td></tr>";
  html += "<tr><td>Probability of Rainfall</td><td>" + String(probability_of_rainfall) + "%</td></tr>";
  html += "<tr><td>Prediction</td><td>" + summary + "</td></tr>";
  html += "</table>";

  // Add the switch for LED control
  html += "<h2>LED Control</h2>";
  html += "<label for='ledSwitch'>Turn LED:</label>";
  html += "<input type='checkbox' id='ledSwitch' onchange='toggleLED()' " + String(ledState ? "checked" : "") + "> ON/OFF";

  html += "<script>";
  html += "function toggleLED() {";
  html += "  var ledSwitch = document.getElementById('ledSwitch');";
  html += "  var ledState = ledSwitch.checked ? 1 : 0;";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/toggleLED?state=' + ledState, true);";
  html += "  xhr.send();";
  html += "}";
  html += "</script>";

  html += "</body></html>";
  return html;
}
// Refine WiFi input for SSID and password
void inputWiFiCredentials() {
  Serial.println(F("Enter WiFi SSID: "));
  while (!Serial.available());  // Wait for input
  wifi_name = Serial.readStringUntil('\n');  // Read the SSID as a string

  Serial.println(F("Enter WiFi Password: "));
  while (!Serial.available());  // Wait for input
  pass = Serial.readStringUntil('\n');  // Read the password as a string

  // Trim any trailing newlines or spaces
  wifi_name.trim();
  pass.trim();

  WiFi.begin(wifi_name.c_str(), pass.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

// Setup function
void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Start the web server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", sendHTML());
  });

  // Route to handle LED control
  server.on("/toggleLED", HTTP_GET, [](AsyncWebServerRequest *request) {
    String state = request->getParam("state")->value();
    ledState = (state == "1") ? true : false;
    digitalWrite(LEDPIN, ledState ? HIGH : LOW);
    request->send(200, "text/plain", "OK");
  });

  server.begin();
}

// Loop function
void loop() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  dewPoint = calculateDewPoint(temperature, humidity);
  sunshine = readSunshine();

  if (WiFi.status() == WL_CONNECTED) {
    fetchOpenWeatherData();
    uploadToThingSpeak();
    uploadToFlaskServer();
  }/else{
    /**Serial.println(F("Enter WiFi SSID : "));
    while (!Serial.available());
    while (Serial.available()) 
    {
        wifi_name = Serial.read();
    }
    Serial.println(F("Enter WiFi Password : "));
    while (!Serial.available());
    while (Serial.available()) 
    {
        pass = Serial.read();
    }
    WiFi.begin(wifi_name, pass);
    while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");}*/
    Serial.println("WiFi not connected, trying to reconnect...");
    inputWiFiCredentials();
  
  }
  
  delay(60000); // Wait for 1 minute before repeating
}
