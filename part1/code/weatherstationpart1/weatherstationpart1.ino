/*
 *  Basic sketch that reads temperature and humidity from dht devices and reports it to the binaryweather site: www.binaryweather.co.za
 */

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>


// ******************** modify below according to your setup ********************************
#define DHTPIN            D2        // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHT11     // DHT11 / DHT22 / DHT21
#define TEMPID            3         // datapoint id on the binaryweather webservice
#define HUMIDITYID        4         // datapoint id on the binaryweather webservice
#define UPDATEPERIOD      5         // how often must it read and send data

const char* ssid     = "<INSERT WIFI SSID HERE>";     // wifi ssid
const char* password = "<INSER WIFI PASS HERE>";              // wifi password
const char* apiUser = "<INSERT API USER HERE>";         // binaryweather username
const char* apiPass = "<INSER API PASS HERE>";       // binaryweather password

// *****************************************************************************************

DHT_Unified dht(DHTPIN, DHTTYPE);

const char* host = "www.binaryweather.co.za";
const int httpPort = 80;
const char* streamId   = "";
String privateKey;
bool authorized = false;

uint32_t delayMS;

/*******************************************************
 setup
 *******************************************************/
void setup() {
  Serial.begin(9600);
  delay(10);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  //connect to wifi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  dht.begin();
  Serial.println("BinarySpace Weather Station Part 1");

/*
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);
  */


  getJWToken(); //get authorization key

  if ( authorized )
    Serial.println( "Authorized!" );
  else
   Serial.println( "Not Authorized!" );
}

// main loop
void loop() 
{
  // read temperature/humidity sensors
  float temp = 0;
  float humidity = 0;
  
  sensors_event_t event;  

  // get temperature
  dht.temperature().getEvent(&event);

  if (isnan(event.temperature)) 
  {
    Serial.println("Error reading temperature!");
  }
  else 
  {
    Serial.print("Temperature: ");
    Serial.print(event.temperature);
    temp = event.temperature;
    Serial.println(" *C");

    sendSensorValue(TEMPID, temp);
  }

  // get humidity
  dht.humidity().getEvent(&event);

  if (isnan(event.relative_humidity)) 
  {
    Serial.println("Error reading humidity!");
  }
  else 
  {
    Serial.print("Humidity: ");
    Serial.print(event.relative_humidity);
    humidity = event.relative_humidity;
    Serial.println("%");

    sendSensorValue(HUMIDITYID, humidity);
  }

  // update every x minutes
  delay(UPDATEPERIOD * 60 * 1000);
}


void sendSensorValue(int id, float value)
{
  WiFiClient client;

  // Use WiFiClient class to create TCP connections
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  const int BUFFER_SIZE = JSON_OBJECT_SIZE(3);
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& dataRoot = jsonBuffer.createObject();
  dataRoot["id"] = id;
  //dataRoot["tstamp"] = unixTimestamp; // optional - server will use current date/time if omitted
  dataRoot["value"] = value;

  String url = "/api/datapoint.php";
  postRequest("PUT", &client, url, dataRoot, true);  
 
  
}

char* copy(const char* orig) {
    char *res = new char[strlen(orig)+1];
    strcpy(res, orig);
    return res;
}

// getJWToken
void getJWToken() {

  String url = "/api/authenticate.php";
  StaticJsonBuffer<400> jsonBuffer; //used to store server response

  //build json object to send data
  const int BUFFER_SIZE = JSON_OBJECT_SIZE(2);
  StaticJsonBuffer<BUFFER_SIZE> jsonRequestBuffer;

  JsonObject& loginRoot = jsonRequestBuffer.createObject();
  loginRoot["username"] = apiUser;
  loginRoot["password"] = apiPass;

  WiFiClient client;

  // Use WiFiClient class to create TCP connections
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  Serial.println("----------Retrieving JWT--------");
  Serial.println("Retrieving JWT");
  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  postRequest("POST",&client, url, loginRoot, false);

  //block until data available
  while (client.available() == 0)
  {
    if (client.connected() == 0)
    {
      return;
    }
  }
  Serial.print("response length:");
  Serial.println(client.available());

  client.setTimeout(1000); //in case things get stuck

  //read http header lines
  while (client.available()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);
    if (line.length() == 1) { //empty line means end of headers
      break;
    }
  }

Serial.println( "Headers done" );
Serial.println( "check for token" );
if (client.available()) 
{
    String line = client.readStringUntil('\n');
    //Serial.println(line);

    const char* lineChars = line.c_str();
    char* json = copy(lineChars);
    
    JsonObject& root = jsonBuffer.parseObject(json);
    
    if (root.containsKey("token"))
    {
      const char* tokArray = root["token"];
      String token(tokArray); //convert to String
      privateKey = token; // Global variable so we can get the token anywhere
      Serial.print( "token:");
      Serial.println( privateKey );
      authorized = true;
    }    
    else
    {
      Serial.println( "Failed to retrieve token" );
      return;
    }

}
}

// post request
void postRequest(const char* method, WiFiClient* client, String url, JsonObject& jsonRoot, bool needKey) {
  // This will send the request to the server
  client->print(method); // POST /auth/login HTTP/1.1
  client->print(" "); // 
  client->print(url);
  client->println(" HTTP/1.1");

  client->print("Host: "); 
  client->println(host);

  client->println("User-Agent: Esp8266WiFi/0.9");

  if (needKey) {
    client->print("Authorization: Bearer "); //Authorization: Bearer <token>
    client->println(privateKey);
  }

  client->println("Accept: application/json");
  client->println("Content-Type: application/json");
  client->print("Content-Length: ");

  String dataStr;
  jsonRoot.printTo(dataStr);

  client->println(dataStr.length());
  client->println("Connection: close");
  client->println();
  jsonRoot.printTo(*client);
  client->println();
  delay(10);
}

