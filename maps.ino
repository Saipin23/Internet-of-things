#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>


const char* ssid = "xxxxx"; // ชื่อ wifi
const char* password = "xxxxx";  // รหัสผ่าน wifi
const char* mqtt_server = "xxxxx"; //IP Server
const char* outTopic = "/kdi/inTopic"; 
const char* inTopic = "/kdi/outTopic"; 
const char* clientName = "";  
String sChipID;
char cChipID[20];

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

//json
StaticJsonDocument<200> doc;
DeserializationError error;
JsonObject root;

//GPS
static const int RXPin = 5, TXPin = 4;
static const uint32_t GPSBaud = 9600;
double lat,lon;
unsigned long previousMillis = 0;    
const long interval = 30000; //กำหนดเวลาในการส่งพิกัดตำแหน่ง       
// The TinyGPS++ object
TinyGPSPlus gps;
// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

void setup() {
  Serial.begin(115200);
  setup_wifi();
  ss.begin(GPSBaud);

  //이름 자동으로 생성
  sChipID=String(ESP.getChipId(),HEX);
  sChipID.toCharArray(cChipID,sChipID.length()+1);
  clientName=&cChipID[0];
  Serial.println(clientName);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//  payload 
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  /*
  deserializeJson(doc,payload);
  root = doc.as<JsonObject>();
  const char* red = root["r"];
  */
}

// mqtt 통신에 지속적으로 접속한다.
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientName)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(outTopic, "Reconnected");
      // ... and resubscribe
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void displayInfo()
{
  if (gps.location.isValid())
  {
    lat=gps.location.lat();
    lon=gps.location.lng();
  }
  else
  {
    Serial.print(F("INVALID"));
  }
}

void loop() {
   
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  

  while (ss.available() > 0)
    gps.encode(ss.read());
  unsigned long currentMillis = millis();
  if (currentMillis > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }

  if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      displayInfo();
      if (client.connected()) {
        sprintf (msg, "\{\"lat\":%.5f,\"lon\": %.5f}",lat, lon);
        Serial.println(msg);
        client.publish(outTopic, msg);
        
      }
  }
}
