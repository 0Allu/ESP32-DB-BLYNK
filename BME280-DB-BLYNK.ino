// Aleksi Räisänen @0Allu
// Using 2x Adafruit BME280 sensors
// Sending data to Blynk servers, Database and also displaying it on an LCD

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BLYNK_TEMPLATE_ID "TMPLFaaBHhXd" // Blynk pass
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "5p1zpiGn3BGjht4vHgrCDtJLrWImcb2o"
#define BLYNK_PRINT Serial

#define SCREEN_WIDTH 128 // LCD definitions
#define SCREEN_HEIGHT 64
#define SEALEVELPRESSURE_HPA (1013.25) // sea level pressure for BME280 sensor

Adafruit_BME280 bme, bme1; //sensor 1 and 2

char ssid[] = "KMRA"; // WiFi name & password
char password[] = "2674700304130";

const char* serverName = "https://192.168.1.224/insert_temperature.php"; // DB insert url
const char* serverName1 = "https://192.168.1.224/insert_temperature1.php"; // DB 2nd insert url

const int numReadings = 30;  //getting average
int Sensor1readings[numReadings];
int Sensor2readings[numReadings];
int readIndex = 0;
int readIndex1 = 0;
long total = 0;
long total1 = 0;
// float aisVal = 0; // -- Uncomment for debugging!! --
// float aisVal1 = 0; // -- Uncomment for debugging!! --

unsigned long lastMillisSend, lastMillisAvg, lastMillisScreen;

float temp, hum, pre, temp1, hum1, pre1; // Temperature, Humidity and Pressure values

BlynkTimer timer;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // LCD display

void sendSensorData() // Setting up for Blynk
{
  // Assign values
  hum = bme.readHumidity();   temp = bme.readTemperature();   pre = bme.readPressure() / 100.0F;
  hum1 = bme1.readHumidity(); temp1 = bme1.readTemperature(); pre1 = bme1.readPressure() / 100.0F;

  if (isnan(hum) || isnan(temp) || isnan(pre) || isnan(hum1) || isnan(temp1) || isnan(pre1))
  { // Checking if any value is "Not-A-Number" (NaN)
    Serial.println("Failed to read from BME sensor!");
    return;
  }
  // Setting datastreams to catch values
  Blynk.virtualWrite(V4, hum); Blynk.virtualWrite(V5, temp); Blynk.virtualWrite(V6, pre);
  Blynk.virtualWrite(V7, hum1); Blynk.virtualWrite(V8, temp1); Blynk.virtualWrite(V9, pre1);
}

void setup()
{
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D); // initialize LCD with I2C address 0x3C (128x64)
  display.display();
  display.clearDisplay();
  display.display();
  display.setTextSize(1.2);
  display.setTextColor(WHITE);

  WiFi.begin(ssid, password); // Connecting to WiFi
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password); // Blynk connection start

  timer.setInterval(1000L, sendSensorData);

  bool status = bme.begin(0x76); // Specify BME280 with address 0x76
  bool status1 = bme1.begin(0x77); // BME280 with default address 0x77

  if (!status || !status1) // Checking if connected
  {
    Serial.println("Could not find valid BME280 sensors. Check wiring!");
    while (1);
  }
}

void loop() 
{ /*              -- Uncomment for debugging!! --
  //aisVal = bme.readTemperature(); aisVal1 = bme1.readTemperature();  
  Serial.print(F("ais val "));  Serial.println(aisVal);  Serial.print(F("ais avg : "));  Serial.println(smooth());
  Serial.print(F("ais val1 ")); Serial.println(aisVal1); Serial.print(F("ais avg1 : ")); Serial.println(smooth1());*/
  Blynk.run(); // Run blynk
  timer.run();

  if (millis() - lastMillisSend >= 60000UL) //wait 60 seconds
  {
    lastMillisSend = millis(); //reset timer
    sendData(); // sendData function runs every 60 seconds
  }

  if (millis() - lastMillisScreen >= 1000UL) //wait 60 seconds
  {
    lastMillisScreen = millis(); //reset timer
    updateLCD(); // Update LCD every second
  }
  

  if (millis() - lastMillisAvg >= 500UL) //wait 0.5 seconds
  {
    lastMillisAvg = millis(); //reset timer
    smooth(); smooth1(); // Update average 
  }
}

long smooth() { // smoothing for average value
  long average; //Perform average on sensor readings
  total = total - Sensor1readings[readIndex]; // subtract the last reading
  Sensor1readings[readIndex] = bme.readTemperature(); // read the sensor
  total = total + Sensor1readings[readIndex]; // add value to total
  readIndex = readIndex + 1; // handle index
  if (readIndex >= numReadings) {
    readIndex = 0;
  }
  average = total / numReadings; // calculate the average
  return average;
}

long smooth1() { // smoothing for average value in second sensor
  long average1; //Perform average on sensor readings
  total1 = total1 - Sensor2readings[readIndex1]; // subtract the last reading
  Sensor2readings[readIndex1] = bme1.readTemperature(); // read the sensor
  total1 = total1 + Sensor2readings[readIndex1]; // add value to total
  readIndex1 = readIndex1 + 1; // handle index
  if (readIndex1 >= numReadings) {
    readIndex1 = 0;
  }
  average1 = total1 / numReadings; // calculate the average
  return average1;
}

void sendData() //Send an HTTP POST request every 60 seconds
{
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClientSecure *client = new WiFiClientSecure;
    client->setInsecure(); //don't use SSL certificate
    HTTPClient https;

    https.begin(*client, serverName); // IP address with path
    https.addHeader("Content-Type", "application/x-www-form-urlencoded"); // Specify content-type header
    // Prepare your HTTP POST request data 
    String httpRequestData = "api_key=tPmAT5Ab3j7F9&sensor=BME280#1&location=Outside&value1=" + String(smooth())
                             + "&value2=" + String(bme.readHumidity()) + "&value3=" + String(bme.readPressure() / 100.0F) + "";

    String httpRequestData1 = "api_key=tPmAT5Ab3j7F9&sensor=BME280&location=Inside&value1=" + String(smooth1())
                              + "&value2=" + String(bme1.readHumidity()) + "&value3=" + String(bme1.readPressure() / 100.0F) + "";

    int httpResponseCode = https.POST(httpRequestData);
    int httpResponseCode1 = https.POST(httpRequestData1);
    https.end(); // Free resources
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
}

void updateLCD() // Refreshes the LCD every second!
{
  display.setCursor(0, 0); 
  display.clearDisplay();
  display.println("- Sensor #1");

  display.print("Temperature: "); display.print(bme.readTemperature());       display.println(" *C");
  display.print("Pressure: ");    display.print(bme.readPressure() / 100.0F); display.println(" hPa");
  display.print("Humidity: ");    display.print(bme.readHumidity());          display.println(" %");

  display.println("- Sensor #2");

  display.print("Temperature: "); display.print(bme1.readTemperature());       display.println(" *C");
  display.print("Pressure: ");    display.print(bme1.readPressure() / 100.0F); display.println(" hPa");
  display.print("Humidity: ");    display.print(bme1.readHumidity());          display.println(" %");

  display.display();
}