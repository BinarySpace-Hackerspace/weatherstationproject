# BinaryWeather API Notes

## Gettting a JWT Token
Send a POST request to `http://www.binaryweather.co.za/api/authenticate.php` with a JSON object containing a `username` and `password`

```c++
  //build json object to send data
  const int BUFFER_SIZE = JSON_OBJECT_SIZE(2);
  StaticJsonBuffer<BUFFER_SIZE> jsonRequestBuffer;

  JsonObject& loginRoot = jsonRequestBuffer.createObject();
  loginRoot["username"] = "<username>";
  loginRoot["password"] = "<password>";
  
  String url = "/binaryweather/api/authenticate.php";
  postRequest("POST", &client, url, loginRoot, false);
```
This should return a JSON object containing the JWT Token
```c++
  JsonObject& root = jsonBuffer.parseObject(<result from server>);
  if (root.containsKey("token"))
  {
    const char* tokArray = root["token"];
    String token(tokArray); //convert to String
    privateKey = token; // Global variable so we can get the token anywhere
  }
```
Store this token for use in further calls

## Sending data
Send a PUT request to `http://www.binaryweather.co.za/api/datapoint.php` with a JSON object containing a `id` and `value` and optional `tstamp`. The `id` is the ID of the data point as shown on the [BinaryWeather](https://www.binaryweather.co.za) site under **My Points**

```c++
  const int BUFFER_SIZE = JSON_OBJECT_SIZE(3);
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& dataRoot = jsonBuffer.createObject();
  dataRoot["id"] = pointID;
  dataRoot["tstamp"] = unixTimestamp; // optional - server will use current date/time if omitted
  dataRoot["value"] = sensorValue;

  String url = "/binaryweather/api/datapoint.php";
  postRequest("PUT", &client, url, dataRoot, true);
```

## Function to post requests to the server
This is an example of one way to post requests to the server.
```c++
void postRequest(const char* method, WiFiClient* client, String url, JsonObject& jsonRoot, bool needKey) {
  // This will send the request to the server
  client->print(method); // POST /auth/login HTTP/1.1
  client->print(" "); // POST /binaryweather/api/authenticate.php HTTP/1.1
  client->print(url);
  client->println(" HTTP/1.1");

  client->print("Host: "); //Host: www.binaryweather.co.za
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
```