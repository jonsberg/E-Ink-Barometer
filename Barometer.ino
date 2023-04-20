#include <WiFi.h>
#include "hidden.h"
#include "model.h"
#include "display.h"

inline void tdelay(size_t millis) {
	vTaskDelay(millis / portTICK_PERIOD_MS);
}

void connect() {
	Serial.print("Connecting to WiFi: ");
	WiFi.begin(ssid, passwd);

	while (WiFi.status() != WL_CONNECTED) {
		tdelay(1000);
		Serial.print(".");
	}

	Serial.println("Connected to IP Address: ");
	Serial.print(WiFi.localIP());
	Serial.println();
	WiFi.setAutoReconnect(true);
}

APIFetcher fetcher(city, api_key);
Display::DisplayWrapper dw;
Display::OverlayHandler handler = Display::OverlayHandler(dw);

Display::Frame above(0, 0, 248, 64);
Display::Frame bottom(0, 64, 240, 64);

Display::Table table = Display::Table(3, 3, above);
Display::Barometer baro = Display::Barometer(24, bottom);

void setup() {
	Serial.begin(115200);
	tdelay(1000);
	connect();
	handler.add_component(&table);
	handler.add_component(&baro);
}

static long last_entry = 0;



void loop() {
	std::unique_ptr<Model> d = fetcher.get_data();
	if (d != nullptr ) {
		if (d->dt - last_entry > 0)
		{
			auto c = handler.get_components();
			for (auto i : c)
			{
				i->update(*d);
				i->paint(dw);
			}

			last_entry = d->dt;
		}
	}

	tdelay(300000);
}