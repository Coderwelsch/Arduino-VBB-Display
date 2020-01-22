#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h";

const char* url = "https://projects.coderwelsch.com/misc/vbb-display/";


void setup() {
	Serial.begin(115200);
	delay(4000);
	WiFi.begin(WIFI_SSID, WIFI_PWD);
 
	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		Serial.println("Connecting to WiFi..");
	}
 
	Serial.println("Connected to the WiFi network");
}
 
void loop () {
  JsonArray jsonArray = getJsonData;
  
	if ((WiFi.status() == WL_CONNECTED)) { // Check the current connection status
		HTTPClient http;
 
		http.begin(url); // Specify the URL
		int httpCode = http.GET();
 
		if (httpCode > 0) { // Check for the returning code
				String payload = http.getString();
				Serial.println(httpCode);
				Serial.println(payload);
				
				// https://arduinojson.org/v6/assistant/
				const size_t jsonDataSize = JSON_ARRAY_SIZE(15) + 15 * JSON_OBJECT_SIZE(3);
				StaticJsonDocument<jsonDataSize> doc;
				DeserializationError error = deserializeJson(doc, payload);

				if (error) {
					Serial.println("Datenfehler: " + String(error.c_str()));
				} else {
					jsonArray = doc.as<JsonArray>();

					for (JsonVariant tram : jsonArray) {
						const char* direction = tram["d"];
						const char* tramLine = tram["n"];
						int time = tram["t"].as<int>();

						Serial.printf("%s %s | %s min\n", tramLine, direction, String(time / 60));
					}
				}
		} else {
			Serial.println("Error on HTTP request");
		}
 
		http.end(); //Free the resources
	}
 
	delay(10000);
}

JsonArray getJsonData () {
  
}
