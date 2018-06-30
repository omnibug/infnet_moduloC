/**********************************************
 * Catalin Batrinu bcatalin@gmail.com 
 * Read temperature and pressure from BMP280
 * and send it to thingspeaks.com
**********************************************/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>

#define A0  17
#define D0  16
#define D1   5
#define D2   4
#define D3   0
#define D4   2
#define D5  14
#define D6  12
#define D7  13
#define D8  15
#define D9   3
#define D10  1

#define BMP_SCK 13
#define BMP_MISO 12
#define BMP_MOSI 11 
#define BMP_CS 10

Adafruit_BMP280 bme; // I2C

const char* server = "192.168.1.XXX";

// Uncomment one of the lines below for whatever DHT sensor type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// DHT Sensor - Humidity
#define DHTPIN D5 // What digital pin we're connected to
// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

// input for rain sensor
#define rainAPIN A0 // What analog pin we're connected to
#define LDRAPIN A0 // What analog pin we're connected to
#define rainDPIN D6 // What digital pin we're connected to
int rainAValue = 0;  // variable to store the value coming from rain Sensor A
int rainDValue = 0;  // variable to store the value coming from rain Sensor D

int enable1 = 15;      // enable reading LDR
int enable2 = 13;      // enable reading Rain sensor

const char* ssid = "XXXXXXXXXXXX";
const char* password = "XXXXXXXX";

String data;
float sealevelpressure = 1020;

float temperature, pressure, altitude, humidity, drytemperature, heatindex;
int rain, rainindex, lightindex;

void(* resetFunc) (void) = 0;//declare reset function at address 0

WiFiClient client;

/**************************  
 *   S E T U P
 **************************/
void setup() {
  // Initializing serial port for debugging purposes
  Serial.begin(115200);
  Serial.println("");
  blinkLED(1);
  Serial.println("");
  Serial.println("Setup  - Initialization sequence");
  // declare the enable and ledPin as an OUTPUT:
  pinMode(enable1, OUTPUT);
  pinMode(enable2, OUTPUT);
  blinkLED(2);
  Serial.println("DTH11  - Initializing Humidity/Temperature Sensor");
  dht.begin();
  delay(2000);

  blinkLED(3);
  Serial.println("BME280 - Initializing Pressure/Temperature Sensor");
  if (!bme.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
//    while (1);
  }
  
  blinkLED(4);
  Serial.println("MHRD   - Initializing Rain Drop Sensor");

  // Connecting to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  blinkLED(5);
  WiFi.begin(ssid, password);

  int countSecConnect = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    countSecConnect += 1;
    if (countSecConnect > 40) {
       resetFunc(); //call reset 
    }
  }
  Serial.println("");
  // print the SSID of the network you're attached to:
  Serial.println("WiFi connected");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("My IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  blinkLED(6);
}

  /**************************  
 *  L O O P
 **************************/
void loop() {
  temperature = 0;
  humidity = 0;
  pressure = 0;
  altitude = 0;
  drytemperature = 0;
  heatindex = 0;
  rain = 0;
  rainindex = 0;

    blinkLED(1);
    Serial.println("\n");
    Serial.println("Data Collection");
    Serial.print("T=");
    temperature = bme.readTemperature();
    Serial.print(temperature);
    Serial.print(" *C");
    
    Serial.print(" P=");
    pressure = bme.readPressure()/100;
    Serial.print(pressure);
    Serial.print(" hPa");

    Serial.print(" A= ");
    altitude = bme.readAltitude(sealevelpressure);
    Serial.print(pressure); // this should be adjusted to your local forcase
    Serial.println(" m");

    // Wait a few seconds between measurements.
    delay(2000);
    Serial.print("H= ");
    humidity = dht.readHumidity();
    Serial.print(humidity);
    Serial.print(" %");

    Serial.print(" DT= ");
    drytemperature = dht.readTemperature();
    Serial.print(drytemperature); 
    Serial.print(" *C");
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(drytemperature)) {
       Serial.println("Failed to read from DHT sensor!");
    }

    Serial.print(" HIC= ");
    heatindex = dht.computeHeatIndex(drytemperature, humidity, false);       
    Serial.print(heatindex); 
    Serial.println(" *C");
    //--------------------------LDR-------------------------
    digitalWrite(enable1, HIGH); 
    lightindex = analogRead(LDRAPIN);
//    lightindex = constrain(lightindex, 300, 850); 
//    lightindex = map(lightindex, 300, 850, 0, 1023); 
    Serial.print("Light intensity:  ");
    Serial.println(lightindex);
    digitalWrite(enable1, LOW);
    delay(100);
    //--------------------------Rain Sensor-------------------------
    digitalWrite(enable2, HIGH); 
    delay(500);
    Serial.print("Ra= ");
    rainindex = analogRead(rainAPIN);
    Serial.print(rainindex); 
    Serial.print(" idx");
    //sensorValue2 = constrain(sensorValue2, 150, 440); 
    //sensorValue2 = map(sensorValue2, 150, 440, 1023, 0); 
    delay(500);
    Serial.print(" Rd= ");
    rain = digitalRead(rainDPIN);
    Serial.print(rain); 
    Serial.println(" ");
    digitalWrite(enable2, LOW);
    //sensorValue2 = constrain(sensorValue2, 150, 440); 
    //sensorValue2 = map(sensorValue2, 150, 440, 1023, 0); 
    
    //--------------------------Upload Data-------------------------
    uploadData(temperature, pressure, altitude, sealevelpressure, humidity, drytemperature, heatindex, rain, rainindex, lightindex);
    //every 60 sec   
    blinkLED(2);
   delay(60000);

}
void blinkLED(int repeats){
  pinMode(D4, OUTPUT); 
  for (int i=1; i <= repeats; i++){
    digitalWrite(D4, LOW);   // Turn the LED on by making the voltage LOW
    delay(500);            // Wait for a quarter second
    digitalWrite(D4, HIGH);
    delay(500);            // Wait for a quarter second
   }
   delay(500);            // Wait for a quarter second
}
void uploadData(float ptemperature, float pressure, float altitude, float sealevelpressure, float humidity, float drytemperature, float heatindex, float rain, float rainlevel, float lightindex){
  //temperature, pressure, altitude, humidity, rain, rainlevel, lightindex
  if (client.connect(server,80))  //www.XXXX.XXX.br "184.XXX.XXX.XXX" or api.thingspeak.com
  {
  
  Serial.println("\n");
  Serial.println("connected to Web server");
  
  String data = "temperature=" + (String)temperature 
    + "&pressure=" + (String)pressure
    + "&altitude=" + (String)altitude
    + "&sealevelpressure=" + (String)sealevelpressure
    + "&humidity=" + (String)humidity
    + "&drytemperature=" + (String)drytemperature
    + "&heatindex=" + (String)heatindex
    + "&rain=" + (String)rain
    + "&rainlevel=" + (String)rainlevel
    + "&lightindex=" + (String)lightindex +  "&";
  
   //change URL below if using your Sub-Domain
   //client.println("POST /store-weather.php HTTP/1.1"); 
   //client.println("GET /storedata.php HTTP/1.1"); 

  client.print("GET /storedata.php?");
  client.print(data);
  client.println(" HTTP/1.1");
  client.println("Host: 192.168.1.XXX");
//  client.print("Host: " + (String)server + "\n");

  client.println();
   
   //change URL below if using your Domain
   //client.print("Host: " + (String)server + "\n");
   //client.println("User-Agent: ESP8266/1.0");
   //client.println("Connection: close"); 
   //client.println("Content-Type: application/x-www-form-urlencoded");
   //client.print("Content-Length: ");
   //client.print(data.length());
   //client.print("\n\n");
   //client.print(data);
   client.stop(); 
   
   Serial.println("My data string im POSTing looks like this: ");
   Serial.println(data);
   Serial.println("And it is this many bytes: ");
   Serial.println(data.length());             
  }
  
}  

