#include <WiFi.h>
#include <DHT.h>
#include <ThingSpeak.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_SSD1306.h>

// Replace with your WiFi credentials
const char* ssid = "";
const char* password = "";

// Replace with your ThingSpeak Channel ID and Write API Key
unsigned long channelID = 123456;
const char* readAPIKey = "";
const char* writeAPIKey = "";

#define DHTPIN 19     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

#define SOIL_MOISTURE_PIN 34  // Analog pin connected to the soil moisture sensor
#define SOIL_MOISTURE_MIN 0   // Minimum analog value for dry soil
#define SOIL_MOISTURE_MAX 4095  // Maximum analog value for wet soil

Adafruit_BMP280 bmp; // I2C
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

WiFiClient client;

// Function to get WMO code description (using switch-case)
String getWmoDescription(int wmoCode) {
  switch (wmoCode) {
    case 0: return "Cloud dev. not observed";
    case 1: return "Clouds dissolving";
    case 2: return "State of sky unchanged";
    case 3: return "Clouds forming";
    case 51: return "Cont. drizzle (slight)";
    case 53: return "Cont. drizzle (moderate)";
    case 55: return "Cont. drizzle (heavy)";
    case 61: return "Cont. rain (slight)";
    case 63: return "Cont. rain (moderate)";
    case 65: return "Cont. rain (heavy)";
    default: return "Unknown code";  
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  if (!bmp.begin(0x76)) { // Initialize the BMP280 sensor
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Initialize the OLED display
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  ThingSpeak.begin(client);
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int soilMoistureRaw = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisture = map(soilMoistureRaw, SOIL_MOISTURE_MIN, SOIL_MOISTURE_MAX, 100, 0); // Map raw value to percentage
  float pressure = bmp.readPressure() / 100.0F; // Get pressure in hPa

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, Soil Moisture: ");
    Serial.print(soilMoisture);
    Serial.print(" %, Pressure: ");
    Serial.print(pressure);
    Serial.println(" hPa");

    // Set the fields with the values
    ThingSpeak.setField(1, temperature);
    ThingSpeak.setField(2, humidity);
    ThingSpeak.setField(4, soilMoisture);
    ThingSpeak.setField(3, pressure);

    // Write to the ThingSpeak channel
    int statusCode = ThingSpeak.writeFields(channelID, writeAPIKey);
    if (statusCode == 200) {
      Serial.println("Channel update successful");
    } else {
      Serial.println("Problem updating channel. HTTP error code " + String(statusCode));
    }
    
    long nextHourCode = ThingSpeak.readLongField(channelID, 5, readAPIKey);
    long nextDayCode = ThingSpeak.readLongField(channelID, 6, readAPIKey);

    if (nextHourCode != -1 && nextDayCode != -1) {
    String nextHourDescription = getWmoDescription(nextHourCode);
    String nextDayDescription = getWmoDescription(nextDayCode);

    // Display on OLED (with better arrangement)
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    // Sensor Readings (Top Half)
    display.setCursor(0, 0);
    display.println("Temperature: " + String(temperature) + " C");
    display.println("Humidity: " + String(humidity) + " %");
    display.println("Soil Moisture: " + String(soilMoisture) + " %");
    display.println("Pressure: " + String(pressure) + " hPa");

    // Predictions (Bottom Half)
    display.setCursor(0, 32);  // Move cursor to the middle vertically
    display.println("Next Hour: " + nextHourDescription);
    display.println("Next Day:  " + nextDayDescription); // Added extra space for alignment
  } else {
    // Handle read errors
    display.clearDisplay();
    display.println("Error reading forecast");
  }

  display.display();

}


  delay(60000);  // Wait 60 seconds before the next reading
}