#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// config
#include "config.h";

// globals
const char* url = "https://projects.coderwelsch.com/misc/vbb-display/";

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

struct Tram_Struct {
	char name[8];
	int timeLeft;
	int initialTime;
	char line[2];
};

struct Tram_Struct tram1;
struct Tram_Struct tram2;

bool dataFetched = false;
bool dataDisplayed = false;

int lastTickMillis = 0;
int TRAM_DEPARTURE_DELAY = 180;


void setup() {
	// lcd
	lcd.begin(16, 2);

	startWiFi();
}

void updateLastMillis () {
	lastTickMillis = millis();
}
 
void loop () {
	if (
		dataFetched && 
		(tram1.timeLeft > TRAM_DEPARTURE_DELAY) &&
		(tram2.timeLeft > TRAM_DEPARTURE_DELAY)
	) {
		displayTimes();
	} else if ((WiFi.status() == WL_CONNECTED)) {
		Serial.println("FETCH");

		getData();
		displayTimes();

		Serial.println("FETCHED");

		dataFetched = true;
		dataDisplayed = true;
	} else {
		lcd.clear();
		lcd.println("WiFi not connected");
	}
}

void displayTimes () {
	// check just every ten seconds if the display
	// should update something
	const int secondsPassed = (millis() - lastTickMillis) / 1000;

	if (secondsPassed >= 10) {
		// update time threshold
		updateLastMillis();

		Serial.println("UPDATE LCD TIME");

		displayTramLine(&tram1, 0, secondsPassed);
		displayTramLine(&tram2, 1, secondsPassed);
	}
}

void displayTramLine (Tram_Struct *tram, byte line, int secondsPassed) {
	// check if tram display has to update
	int minsBefore = tram->timeLeft / 60;
	int minsAfter = (tram->timeLeft - secondsPassed) / 60;

	int minsLeft = tram->timeLeft / 60;
	String time = minsLeft > 9 ? String(minsLeft) : " " + String(minsLeft);

	tram->timeLeft -= secondsPassed;

	lcd.setCursor(0, line);
	lcd.print(
		String(tram->line) + " " +
		String(strndup(tram->name, 3)) + " | " +
		time + " min"
	);
}

void startWiFi () {
	lcd.clear();
	lcd.setCursor(0, 0);

	Serial.begin(115200);
	delay(4000);
	WiFi.begin(WIFI_SSID, WIFI_PWD);

	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		lcd.print("Connect WiFi ...");
	}

	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("WiFi Connected");
}

void getData () {
	HTTPClient http;	
 
	http.begin(url); // Specify the URL
	int httpCode = http.GET();

	if (httpCode > 0) { // Check for the returning code
			String payload = http.getString();

			// https://arduinojson.org/v6/assistant/
			const size_t jsonDataSize = JSON_ARRAY_SIZE(4) + 4 * JSON_OBJECT_SIZE(3);
			StaticJsonDocument<jsonDataSize> doc;
			
			DeserializationError error = deserializeJson(doc, payload);

			if (error) {
				lcd.clear();
				lcd.setCursor(0, 0);
				lcd.println("Datenfehler:");
				lcd.setCursor(0, 1);
				lcd.println(String(error.c_str()));
			} else if (doc.size()) {
				strcpy(tram1.name, doc[0]["d"].as<char*>());
				tram1.initialTime = tram1.timeLeft = doc[0]["t"].as<int>();
				strcpy(tram1.line, doc[0]["n"].as<char*>());

				strcpy(tram2.name, doc[1]["d"].as<char*>());
				tram2.initialTime = tram2.timeLeft = doc[1]["t"].as<int>();
				strcpy(tram2.line, doc[1]["n"].as<char*>());
			}
	} else {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("HTTP Fehler:");

		lcd.setCursor(0, 1);
		lcd.print(String(httpCode));
	}

	http.end();
}
