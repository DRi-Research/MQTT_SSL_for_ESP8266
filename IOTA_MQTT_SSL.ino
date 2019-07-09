/*
IoT device firmware for SmartHome Gateway v0.1 | 9/07/2019
Anushka Wijesundara | MIT licenced | IoT device firmware for SmartHome Gateway
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include "SSD1306Brzo.h" // Include OLED Library

SSD1306Brzo display(0x3c, 5, 4); // Initialize OLED display

int blueLed = LED_BUILTIN; // Blue LED is on GPIO 2 (LED_BUILTIN)
int greenLed = 16; // Green LED is on GPIO 16

int analogPin = A0; // Analog input is A0
int analogValue = 0;

volatile byte state = LOW;
String button = "Please wait…";
String WiFi_status_LCD_txt ="Connecting...";
String MQTT_status_LCD_txt ="Connecting...";
String Time_status_LCD_txt = "";
String Update_status_LCD_txt = "";

//Use ArduinoJson Version 6

////--------------------------////
    #define DHTPIN 12 
    #define DHTTYPE DHT22
    #define LED D0
    DHT dht(DHTPIN, DHTTYPE); 
    

    const char ssid[] = "YOUR WIFI SSID";
    const char pass[] = "YOUR WIFI PASSWORD";

    #define HOSTNAME "IOT DEVICE HOSTNAME"

    const char MQTT_HOST[] = "Smart-Home-Gateway.local";
    const int  MQTT_PORT = 8883;
    const char MQTT_USER[] = "MQTT USERNAME"; // leave blank if no credentials used
    const char MQTT_PASS[] = "MQTT PASSWORD"; // leave blank if no credentials used
    
    const char IOT_FW_VER[]  = "FIRMWARE VERSION"; //Every new version should add here and 
    const int FW_VERSION = FIRMWARE VERSION; // here...
    
    const char* fwUrlBase = "http://Smart-Home-Gateway.local/firmwares/";
    
    const char MQTT_SUB_TOPIC[] =  "IoT/Firmware_Update/in";
    const char MQTT_SUB_TOPIC_FW_UPDATE[] = "SH_Gateway/fw_update";
    const char MQTT_PUB_TOPIC[] =  HOSTNAME "/out";
    const char MQTT_PUB_TOPIC_FW[] =  HOSTNAME "/fw";
    
    #define humidity_topic  HOSTNAME "/humidity"
    #define temperature_celsius_topic  HOSTNAME "/temperature_c"
    #define temperature_fahrenheit_topic  HOSTNAME "/temperature_f"

/////////////// LOCAL ROOT CA - Manually Generated /////////////

static const char digicert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----

YOUR SSL PUBLIC CERTIFICATE IN TEXT

-----END CERTIFICATE-----
)EOF";

//////////////////////////////////////////////////////////////////


BearSSL::WiFiClientSecure net;
PubSubClient client(net);

time_t now;
unsigned long lastMillis = 0;

void checkForUpdates() {
  String mac = WiFi.macAddress();
  String fwURL = String( fwUrlBase );
  fwURL.concat( mac );
  String fwVersionURL = fwURL;
  fwVersionURL.concat( ".version" );

  Serial.println( "Checking for firmware updates." );
  Serial.print( "MAC address: " );
  Serial.println( mac );
  Serial.print( "Firmware version URL: " );
  Serial.println( fwVersionURL );

  HTTPClient httpClient;
  httpClient.begin( fwVersionURL );
  int httpCode = httpClient.GET();
  if( httpCode == 200 ) {
    String newFWVersion = httpClient.getString();
    display.clear(); // Clear OLED display
    LCD_txt_checking_for_update();
    drawText(); // Draw the text
    display.display(); // Write the buffer to the display
    Serial.print( "Current firmware version: " );
    Serial.println( FW_VERSION );
    Serial.print( "Available firmware version: " );
    Serial.println( newFWVersion );

    int newVersion = newFWVersion.toInt();

    if( newVersion > FW_VERSION ) {
      Serial.println( "Preparing to update" );

      display.clear(); // Clear OLED display
      LCD_txt_update_available();
      drawText(); // Draw the text
      display.display(); // Write the buffer to the display

      String fwImageURL = fwURL;
      fwImageURL.concat( ".bin" );
      t_httpUpdate_return ret = ESPhttpUpdate.update( fwImageURL );

      switch(ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          display.clear(); // Clear OLED display
          LCD_txt_update_error();
          drawText(); // Draw the text
          display.display(); // Write the buffer to the display
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("HTTP_UPDATE_NO_UPDATES");
          display.clear(); // Clear OLED display
          LCD_txt_update_error();
          drawText(); // Draw the text
          display.display(); // Write the buffer to the display
          break;
      }
    }
    else {
      Serial.println( "Already on latest version" );
      display.clear(); // Clear OLED display
      LCD_txt_NO_update();
      drawText(); // Draw the text
      display.display(); // Write the buffer to the display
    }
  }
  else {
    Serial.print( "Firmware version check failed, got HTTP response code " );
    Serial.println( httpCode );
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    display.clear(); // Clear OLED display
    LCD_txt_update_error();
    drawText(); // Draw the text
    display.display(); // Write the buffer to the display
  }
  httpClient.end();
}

 
void drawText() { // Fuction to draw the text
display.setTextAlignment(TEXT_ALIGN_LEFT);

display.setFont(ArialMT_Plain_10);
display.drawString(0, 0, "WiFi Status:");
display.setFont(ArialMT_Plain_10);
display.drawString(55, 0, WiFi_status_LCD_txt); // Print button press
display.drawString(0, 10, "MQTT Status:");
display.drawString(65, 10, MQTT_status_LCD_txt);
display.drawString(0, 20, "Last update:");
display.drawString(0, 30, Time_status_LCD_txt);
display.drawString(0, 40, "Firmware: ");
display.drawString(45, 40, Update_status_LCD_txt);
display.setFont(ArialMT_Plain_10);
analogValue = analogRead(analogPin);
display.drawString(0, 50, "FW Ver: " + String(FW_VERSION)); // Print current firmware version
}


void ICACHE_RAM_ATTR WIFI_OK() // Right
{
WiFi_status_LCD_txt = " [ OK ]";
}

void ICACHE_RAM_ATTR MQTT_WAITING() // Down
{
MQTT_status_LCD_txt = "Waiting...";
Time_status_LCD_txt ="";
//blink();
}

void ICACHE_RAM_ATTR MQTT_OK() // Down
{
MQTT_status_LCD_txt = " [ OK ]";
//blink();
}

void ICACHE_RAM_ATTR WIFI_CONNECTING() // Down
{
WiFi_status_LCD_txt = "Connecting...";
//blink();
}

void ICACHE_RAM_ATTR CURRENT_TIME() // Up
{
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Time_status_LCD_txt = asctime(&timeinfo);
//blink();
}

void ICACHE_RAM_ATTR interrupt14() // Push
{
button = "Connecting";
//blink();
}


void LCD_txt_checking_for_update() // Checking for update_LCD
{
Update_status_LCD_txt = " [ Checking ]";
//blink();
}

void LCD_txt_NO_update() // No_update_LCD
{
Update_status_LCD_txt = " [ Latest ]";
//blink();
}

void LCD_txt_update_available() // Update_available_LCD
{
Update_status_LCD_txt = " [ Preparing.. ]";
//blink();
}

void LCD_txt_update_error() // Updated_LCD
{
Update_status_LCD_txt = " [ Error ! ]";
//blink();
}

void blink() {
state = !state; // Reverse LED state
}


void mqtt_connect()
{
  while (!client.connected()) {
    Serial.print("Time: ");
    Serial.print(ctime(&now));
    Serial.print("MQTT connecting ... ");
    if (client.connect(HOSTNAME, MQTT_USER, MQTT_PASS)) {
      Serial.println("connected.");
      display.clear(); // Clear OLED display
      MQTT_OK(); // "Connecting" to OLED
      drawText(); // Draw the text
      display.display(); // Write the buffer to the display
      client.subscribe(MQTT_SUB_TOPIC);
      client.subscribe(MQTT_SUB_TOPIC_FW_UPDATE);
    } else {
      display.clear(); // Clear OLED display
      MQTT_WAITING();
      drawText(); // Draw the text
      display.display(); // Write the buffer to the display
      Serial.print("failed, status code =");
      Serial.print(client.state());
      Serial.println(". Try again in 5 seconds.");
      /* Wait 5 seconds before retrying */
      delay(5000);
    }
  }
}

void receivedCallback(char* topic, byte* payload, unsigned int length) {
//Only reading Json obects using Arduino 6
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length);
  const int fw_version = doc["fw_version"];
  const char* fw_url = doc["fw_url"];
  Serial.println(topic);
  Serial.println(fw_version);
  Serial.println(fw_url);
  if(fw_version>FW_VERSION){
    Serial.println("New version detected !");
    checkForUpdates(); // Check for update
    }
 
}

void setup()
{
  display.init();
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(ssid);
  Serial.print("  ");
  WiFi.hostname(HOSTNAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    display.clear(); // Clear OLED display
    WIFI_CONNECTING(); // "Connecting" to OLED
    drawText(); // Draw the text
    display.display(); // Write the buffer to the display
    Serial.print(".");
    delay(1000);
  }
  display.clear(); // Clear OLED display
  WIFI_OK(); // WiFi OK
  drawText(); // Draw the text
  display.display(); // Write the buffer to the display
  Serial.println("  CONNECTED !");
  Serial.print("IoT device IP -->  ");
  Serial.println(WiFi.localIP());
  Serial.println("");
  Serial.print("Firmware Version --> ");
  Serial.println(FW_VERSION);
  Serial.println("");
  Serial.print("IoT device MAC --> ");
  Serial.println(WiFi.macAddress());
  Serial.println("");
  Serial.print("Setting time using SNTP -->  ");
  //configTime(+9 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  configTime(+9 * 3600, 0, "Smart-Home-Gateway.local","Vendor.local");
  now = time(nullptr);
  while (now < 1510592825) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("  OK");
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
  pinMode(LED, OUTPUT);

///////// ROOT CA - Manual ////////////

    BearSSL::X509List cert(digicert);
    net.setTrustAnchors(&cert);
    
///////////////////////////////////////


  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(receivedCallback);
  mqtt_connect();
  checkForUpdates();
}

void loop()
{
        display.clear(); // Clear OLED display
      CURRENT_TIME(); // "Connecting" to OLED
      drawText(); // Draw the text
      display.display(); // Write the buffer to the display
  now = time(nullptr);
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Checking wifi");
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
      WiFi.begin(ssid, pass);
      Serial.print(".");
      delay(10);
    }
    Serial.println("connected");
  }
  else
  {
    if (!client.connected())
    {
      mqtt_connect();
    }
    else
    {
      client.loop();
    }
  }

  if (millis() - lastMillis > 5000) {
       lastMillis = millis();
       client.publish(MQTT_PUB_TOPIC,ctime(&now), false);
       client.publish(MQTT_PUB_TOPIC_FW,IOT_FW_VER, false);
      // Reading temperature or humidity takes about 250 milliseconds!
      // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
      float h = dht.readHumidity();
      // Read temperature as Celsius (the default)
      float t = dht.readTemperature();
      // Read temperature as Fahrenheit (isFahrenheit = true)
      float f = dht.readTemperature(true);
      
      // Check if any reads failed and exit early (to try again).
      if (isnan(h) || isnan(t) || isnan(f)) {
      //Serial.println("Failed to read from DHT sensor!");
      return;
      }
      
      // Compute heat index in Fahrenheit (the default)
      float hif = dht.computeHeatIndex(f, h);
      // Compute heat index in Celsius (isFahreheit = false)
      float hic = dht.computeHeatIndex(t, h, false);

      Serial.print("Temperature in Celsius:");
      Serial.println(String(t).c_str());
      client.publish(temperature_celsius_topic, String(t).c_str(), true);

      Serial.print("Temperature in Fahrenheit:");
      Serial.println(String(f).c_str());
      client.publish(temperature_fahrenheit_topic, String(f).c_str(), true);


      Serial.print("Humidity:");
      Serial.println(String(h).c_str());
      client.publish(humidity_topic, String(h).c_str(), true);
  }

      digitalWrite(LED, HIGH);
      delay(1000);
      digitalWrite(LED, LOW);
      delay(30);
}
