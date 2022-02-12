#include "Arduino.h"
#include "aws_certificate.h"
#include "aws_parameter.h"
#include "wifi_credential.h"

#include "WiFi.h"
#include "WiFiClientSecure.h"

#include <MQTTClient.h>
#include <ArduinoJson.h>

// Secure connection
WiFiClientSecure client = WiFiClientSecure();
// Client MQTT
MQTTClient clientMQTT = MQTTClient(256);

// Callback for every message on the topic subscribed
void messageCallback(String &topic, String &payload);
// Establish connection to MQTT
bool manageAWSConnection();
// Subscribe topic
bool manageAWSSubscription();
// Publish message to topic
bool publishMessage();

void setup()
{
	Serial.begin(115200);
	while(!Serial){}
	Serial.flush();
	Serial.println();

	// WIFI_SSID, WIFI_PASSWORD Stored on wifi_credential.h file
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

	Serial.print("Connecting to ");
	Serial.print(WIFI_SSID); Serial.println("");

	int i = 0;
	while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
		delay(1000);
		Serial.print(++i); Serial.print('.');
	}

	Serial.println('\n');
	Serial.println("Connection established!");
	Serial.print("IP address:\t");
	Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer

	if (manageAWSConnection()){
		manageAWSSubscription();
	}
}

unsigned long lastSendingTime = millis();
unsigned long sendMessageEveryMillis = 5000;

void loop()
{
	clientMQTT.loop();
	if (lastSendingTime+sendMessageEveryMillis<millis()){
		publishMessage();
		lastSendingTime = millis();
	}
}

bool manageAWSConnection(){
	// Configure WiFiClientSecure for the AWS IoT device certificate
	client.setCACert(AWS_CERT_CA);
	client.setCertificate(AWS_CERT_CRT);
	client.setPrivateKey(AWS_CERT_PRIVATE);

	// Connect to the MQTT broker on the AWS endpoint
	clientMQTT.begin(AWS_IOT_ENDPOINT, 8883, client);

	// Create a message handler
	clientMQTT.onMessage(messageCallback);

	Serial.print("Connecting to AWS IOT");

	while (!clientMQTT.connect(THINGNAME)) {
		Serial.print(".");
		delay(100);
	}

	if(!clientMQTT.connected()){
		Serial.println("AWS IoT Timeout!");
		return false;
	}
	return true;
}

bool manageAWSSubscription(){
	  // Subscribe to a topic
	  if (clientMQTT.subscribe(AWS_IOT_SUBSCRIBE_TOPIC)){
		  Serial.println("AWS IoT Connected!");
		  return true;
	  }else{
		  Serial.print("AWS IoT ERROR: ");
		  Serial.println(clientMQTT.lastError());
		  return false;
	  }
}

void messageCallback(String &topic, String &payload) {
	Serial.println("incoming: [" + topic + "]" );
	Serial.println("---------------------------" );

	StaticJsonDocument<200> doc;
	deserializeJson(doc, payload);
	serializeJsonPretty(doc, Serial);
	Serial.println();
	Serial.println("---------------------------" );
}

bool publishMessage()
{
	// Create simple message
	DynamicJsonDocument doc(512);

	doc["time"] = millis();
	doc["deviceName"] = "esp32";
	doc["status"] = "OK";

	char jsonBuffer[512];
	serializeJson(doc, jsonBuffer); // print to client

	Serial.print("Message on ");
	Serial.print(AWS_IOT_PUBLISH_TOPIC);
	Serial.print(" topic... ");
	// Publish message to AWS_IOT_PUBLISH_TOPIC
	if (clientMQTT.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer)){
		Serial.println("Published!");
		return true;
	}else{
		Serial.println("Error!");
		return false;
	}
}
