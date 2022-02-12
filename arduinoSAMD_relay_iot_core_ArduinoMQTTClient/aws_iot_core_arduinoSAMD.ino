#include "Arduino.h"
#include "aws_certificate.h"
#include "aws_parameter.h"
#include "wifi_credential.h"

#include <WiFiNINA.h> // change to #include <WiFi101.h> for MKR1000
#include <ArduinoECCX08.h>
#include <ArduinoBearSSL.h>

#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>

// Secure connection
WiFiClient    wifiClient;            // Used for the TCP socket connection
BearSSLClient client(wifiClient); // Used for SSL/TLS connection, integrates with ECC508
// Client MQTT
MqttClient    mqttClient(client);

// Callback for every message on the topic subscribed
void messageCallback(String &topic, String &payload);
// Establish connection to MQTT
bool manageAWSConnection();
// Subscribe topic
bool manageAWSSubscription();
// Publish message to topic
void publishMessage();

unsigned long getTime() {
  // get the current time from the WiFi module
  return WiFi.getTime();
}

void setup()
{
	Serial.begin(115200);
	while(!Serial){}
	Serial.flush();
	Serial.println();

	  if (!ECCX08.begin()) {
		Serial.println("No ECCX08 present!");
		while (1);
	  }

	  // Set a callback to get the current time
	  // used to validate the servers certificate
	  ArduinoBearSSL.onGetTime(getTime);

	  client.setEccSlot(1, AWS_CERT_CA, sizeof(AWS_CERT_CA));
	  client.setEccSlot(2, AWS_CERT_CRT, sizeof(AWS_CERT_CRT));

	  // Set the ECCX08 slot to use for the private key
	  // and the accompanying public certificate for it
	  client.setEccSlot(3, AWS_CERT_PRIVATE, sizeof(AWS_CERT_PRIVATE));

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
	  // poll for new MQTT messages and send keep alives
	  mqttClient.poll();

	  if (lastSendingTime+sendMessageEveryMillis<millis()){
		publishMessage();
		lastSendingTime = millis();
	}
}

bool manageAWSConnection(){
	// Configure WiFiClientSecure for the AWS IoT device certificate
//	client.setCACert(AWS_CERT_CA);
//	client.setCertificate(AWS_CERT_CRT);
//	client.setPrivateKey(AWS_CERT_PRIVATE);

//	// Connect to the MQTT broker on the AWS endpoint
//	clientMQTT.begin(AWS_IOT_ENDPOINT, 8883, client);
//
//	// Create a message handler
//	clientMQTT.onMessage(messageCallback);
//
//	Serial.print("Connecting to AWS IOT");
//
//	while (!clientMQTT.connect(THINGNAME)) {
//		Serial.print(".");
//		delay(100);
//	}
//
//	if(!clientMQTT.connected()){
//		Serial.println("AWS IoT Timeout!");
//		return false;
//	}


	  Serial.print("Attempting to MQTT broker: ");
	  Serial.print(AWS_IOT_ENDPOINT);
	  Serial.println(" ");

	  while (!mqttClient.connect(AWS_IOT_ENDPOINT, 8883)) {
	    // failed, retry
	    Serial.print(".");
	    delay(5000);
	  }
	  Serial.println();

	  Serial.println("You're connected to the MQTT broker");
	  Serial.println();


	return true;
}

bool manageAWSSubscription(){
	  // Subscribe to a topic
//	  if (clientMQTT.subscribe(AWS_IOT_SUBSCRIBE_TOPIC)){
//		  Serial.println("AWS IoT Connected!");
//		  return true;
//	  }else{
//		  Serial.print("AWS IoT ERROR: ");
//		  Serial.println(clientMQTT.lastError());
//		  return false;
//	  }
	  // subscribe to a topic
	  mqttClient.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
}

void publishMessage() {
  Serial.println("Publishing message");

  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("arduino/outgoing");
  mqttClient.print("hello ");
  mqttClient.print(millis());
  mqttClient.endMessage();
}

void onMessageReceived(int messageSize) {
  // we received a message, print out the topic and contents
  Serial.print("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    Serial.print((char)mqttClient.read());
  }
  Serial.println();

  Serial.println();
}
