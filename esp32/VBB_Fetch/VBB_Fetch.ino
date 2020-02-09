#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// config
#include "config.h";

// globals
String apiUrl = "";

// https://arduinojson.org/v6/assistant/
const int JSON_DATA_SIZE = JSON_ARRAY_SIZE(LENGTH_OF_STATION_DATA) + LENGTH_OF_STATION_DATA * JSON_OBJECT_SIZE(3);

// init of lcd
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

struct Tram_Struct {
	char d[8];
	int t;
	char n[2];
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

	apiUrl = getApiUrl();
	Serial.println(apiUrl);

	startWiFi();
}

String getApiUrl () {
	// TODO: maybe there is an easier way to
	// concatenate that string? Hmmm…
	String url = "https://projects.coderwelsch.com/misc/vbb-display/?";
	
	url.concat("from=");
	url.concat(FROM_TRAM_STATION);
	url.concat("&");

	url.concat("to=");
	url.concat(TO_TRAM_STATION);
	url.concat("&");

	url.concat("length=");
	url.concat(LENGTH_OF_STATION_DATA);

	return url;
}

void updateLastMillis () {
	lastTickMillis = millis();
}
 
void loop () {
	if (
		dataFetched && 
		(tram1.t > TRAM_DEPARTURE_DELAY) &&
		(tram2.t > TRAM_DEPARTURE_DELAY)
	) {
		displayTimes();
	} else if ((WiFi.status() == WL_CONNECTED)) {
		Serial.println("FETCHING ...");

		getData();

		Serial.println("FETCHED ...");

		// when data fetching was not successfully
		// retry it in 5 secs
		if (!dataFetched) {
			Serial.println("FETCH ERROR. WAIT 5 SECS ...");
			delay(5000);
		} else {
			displayTimes();
			Serial.println("FETCHED SUCCESSFULLY");
		}

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
	int minsBefore = tram->t / 60;
	int minsAfter = (tram->t - secondsPassed) / 60;

	int minsLeft = tram->t / 60;
	String time = minsLeft > 9 ? String(minsLeft) : " " + String(minsLeft);

	tram->t -= secondsPassed;

	// check if the display should really update
	if (minsBefore === minsAfter) {
		return;
	}

	lcd.setCursor(0, line);
	lcd.print(
		String(tram->n) + " " +
		String(strndup(tram->d, 3)) + " | " +
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
 
	http.begin(apiUrl); // Specify the URL
	int httpCode = http.GET();

	if (httpCode >= 200 && httpCode < 400) { // Check for the returning code
		String payload = http.getString();
		StaticJsonDocument<JSON_DATA_SIZE> doc;
		DeserializationError error = deserializeJson(doc, payload);

		if (error) {
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.println("Datenfehler:");
			lcd.setCursor(0, 1);
			lcd.println(String(error.c_str()));

			dataFetched = false;
		} else if (doc.size()) {
			strcpy(tram1.d, doc[0]["d"].as<char*>());
			tram1.t = doc[0]["t"].as<int>();
			strcpy(tram1.n, doc[0]["n"].as<char*>());

			strcpy(tram2.d, doc[1]["d"].as<char*>());
			tram2.t = doc[1]["t"].as<int>();
			strcpy(tram2.n, doc[1]["n"].as<char*>());

			dataFetched = true;
		} else {
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.println("Keine Trams");

			lcd.setCursor(0, 1);
			lcd.println("gefunden -.-");

			dataFetched = false;
		}
	} else {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("HTTP Fehler:");

		lcd.setCursor(0, 1);
		lcd.print(String(httpCode));

		dataFetched = false;
	}

	http.end();
}
