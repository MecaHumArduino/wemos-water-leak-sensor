/**
  Wakes up every 30sec to read data from the water sensor, when water is
  detected, it connects to WiFi, then sends an alarm to an MQTT topic
  @author MecaHumArduino
  @version 1.0
*/

#include <Arduino.h>
#include "secrets.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

#define DEBUG true // switch to "false" for production
Ticker ticker;
WiFiClient espClient;
PubSubClient client(espClient);

#define sensorPower D7 // Power pin
#define sensorPin A0 // Analog Sensor pins
#define durationSleep 30 // seconds
#define NB_TRYWIFI 20 // WiFi connection retries

int sensorData = 0;

// **************
void tick();
void loop();
void setup();
int readSensor();
void connectToHass();
void connectToWiFi();
void publishAlarmToHass(int waterLevel);
// **************

void tick()
{
    // toggle state
    int state = digitalRead(LED_BUILTIN);  // get the current state of GPIO1 pin
    digitalWrite(LED_BUILTIN, !state);     // set pin to the opposite state
}

void connectToWiFi()
{
    int _try = 0;
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    if (DEBUG == true) {
        Serial.println("Connecting to Wi-Fi");
    }

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        _try++;
        if ( _try >= NB_TRYWIFI ) {
            if (DEBUG == true) {
                Serial.println("Impossible to connect WiFi, going to deep sleep");
            }
            ESP.deepSleep(durationSleep * 1000000);
        }
    }
    if (DEBUG == true) {
        Serial.println("Connected to Wi-Fi");
    }
}

void connectToHass()
{
    client.setServer(MQTT_SERVER, 1883);

    // Loop until we're reconnected
    while (!client.connected()) {
        if (DEBUG == true) {
            Serial.print("Attempting MQTT connection...");
        }
        // Attempt to connect
        // If you do not want to use a username and password, change next line to
        // if (client.connect("ESP8266Client")) {
        if (client.connect("ESP8266Client", MQTT_USER, MQTT_PASSWORD)) {
            if (DEBUG == true) {
                Serial.println("connected");
            }
        } else {
            if (DEBUG == true) {
                Serial.print("failed, rc=");
                Serial.print(client.state());
                Serial.println(" try again in 5 seconds");
            }
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

/**
 * This is a function used to get the reading
 * @param level
 * @return
 */
int readSensor()
{
    // Turn the sensor ON
	digitalWrite(sensorPower, HIGH);
	delay(10);
    // Perform the reading
	sensorData = analogRead(sensorPin);

    // Set to LOW to turn the sensor OFF
	digitalWrite(sensorPower, LOW);

  return sensorData;
}

void publishAlarmToHass(int waterLevel)
{
    // publish the reading to Hass through MQTT
    client.publish(MQTT_PUBLISH_TOPIC, String(waterLevel).c_str(), true);
    client.loop();
    if (DEBUG == true) {
        Serial.println("Alarm sent to Hass!");
    }
}
void setup()
{
    if (DEBUG == true) {
        Serial.print("Waking up ");
    }

    pinMode(sensorPower, OUTPUT); // Set D7 as an OUTPUT
    digitalWrite(sensorPower, LOW);

    ticker.attach(0.5, tick);

	Serial.begin(115200);

    // Wake up the leak sensor & get a reading
    int waterLevel = readSensor();
    if (DEBUG == true) {
        Serial.print("Water Level: ");
        Serial.println(waterLevel);
    }

    // If water is detected
    if (waterLevel > 1) {
        connectToWiFi();
        connectToHass();
        publishAlarmToHass(waterLevel);
    }

    if (DEBUG == true) {
        Serial.println("Going to deep sleep now");
    }

    ESP.deepSleep(durationSleep * 1000000);
}

void loop()
{

}
