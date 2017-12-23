/*
 *  This sketch demonstrates how to set up a simple HTTP-like server.
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/gpio/0 will set the GPIO2 low,
 *    http://server_ip/gpio/1 will set the GPIO2 high
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#define NUM 4

const char* ssid = "lolnet";
const char* password = "anetaaneta";
const char* mqtt_server = "192.168.0.10";
const char* topicName = "strop";

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);
WiFiClient espClient;
PubSubClient pubSubClient(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

String locations[NUM] = {"gauc", "zed", "tv", "stul"};
int pins[NUM] = {D5, D6, D7, D8};
int pinStatus[NUM] = {1, 1, 1, 1};

// Update these with values suitable for your network.
IPAddress ip(192,168,0,80);  //Node static IP
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);

void connectToWifi() {
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
}

int getIdFromName(String name) {
  for(int i=0;i<NUM;i++){
      if (name.equals(locations[i])) {
        return i;
      }
    }
    return -1;
}

int getPinStatusFromName(String name) {
  if (name.equals("on")) {
    return 0; 
  } else {
    return 1;
  }
  
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

 String stringPayload = String((char*)payload);
 stringPayload = stringPayload.substring(0, length);
 stringPayload.trim();
 Serial.println(stringPayload);
 if(stringPayload.equals("getAll")){
   publishStatesToMqtt();
 } else if (stringPayload.startsWith("set")) {
    int spaceIndex = stringPayload.indexOf(" ");
    String pinAndState = stringPayload.substring(spaceIndex, length);
    pinAndState.trim();
    spaceIndex = pinAndState.indexOf(" ");
    String pinName = pinAndState.substring(0,spaceIndex);
    pinName.trim();
    String state = pinAndState.substring(spaceIndex,pinAndState.length());
    state.trim();
    int id = getIdFromName(pinName);
    if (id != -1) {
      setPinValue(id, getPinStatusFromName(state));
    }
 }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  
  // prepare pins
  for(int i=0;i<NUM;i++){
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], pinStatus[i]);
  }
  pubSubClient.setServer(mqtt_server, 1883);
  pubSubClient.setCallback(callback);
  connectToWifi();
}

void reconnect() {
  // Loop until we're reconnected
  while (!pubSubClient.connected()) {
    Serial.print("Attempting MQTT connection to ...");
    Serial.print(mqtt_server); 
    Serial.print("...."); 
    // Create a random client ID
     String clientId = "Strop2";
    // Attempt to connect
    if (pubSubClient.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      publishStatesToMqtt();
      // ... and resubscribe
      pubSubClient.subscribe(topicName);
    } else {
      Serial.print("failed, rc=");
      Serial.print(pubSubClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void publishStatesToMqtt() {
   for(int i=0;i<NUM;i++){
    publishStateToMqtt(i);
  }
}

void publishStateToMqtt(int i) {
    String publish = locations[i] + " " + ((pinStatus[i] == 0)?"on":"off");
    pubSubClient.publish(topicName, publish.c_str());
}

void setPinValue(int pinId, int value) {
  digitalWrite(pins[pinId], value);
  pinStatus[pinId] = value;
  publishStateToMqtt(pinId);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wifi not connected");
    connectToWifi();
  }

  if (!pubSubClient.connected()) {
    reconnect();
  }
  pubSubClient.loop();
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  Serial.println("new client");
  unsigned long time = millis();
  while(!client.available()){
    delay(1);
    Serial.print(".");
    if ((millis() - time) > 2000) {
      client.stop();
      return;
    }
  }
  Serial.println("");
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  if(req.indexOf("GET / ") == -1){
    // Match the request
    int newValues[NUM] = {-1, -1, -1, -1};
    bool requestOk = false;
    for(int i=0;i<NUM;i++){
      if (req.indexOf("/" + locations[i] + "/0") != -1){
        if (pinStatus[i] != 1){
          newValues[i] = 1;
        }
        requestOk = true;
      }else if (req.indexOf("/" + locations[i] + "/1") != -1){
        if (pinStatus[i] != 0){
          newValues[i] = 0;
        }
        requestOk = true;
      }
    }
    if(!requestOk){
       Serial.println("invalid request");
       client.stop();
       return;
    }
    // Set pins according to the request
    for(int i=0;i<NUM;i++){
      if(newValues[i]!=-1){
        setPinValue(i, newValues[i]);
      }
    }
  }
  
  client.flush();

  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
  for(int i=0;i<NUM;i++){
    s += locations[i] + ": ";
    s += (pinStatus[i] == 0)?"on":"off";
    s += "\r\n<br>";
  }
  
  s += "</html>\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}

