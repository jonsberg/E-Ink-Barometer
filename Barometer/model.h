#pragma once
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <memory>

struct Model {
	float coord_lon;
	float coord_lat;
	int weather_0_id;
	const char* weather_0_main;
	const char* weather_0_description;
	const char* weather_0_icon;
	const char* base;
	float main_temp;
	float main_feels_like;
	float main_temp_min;
	float main_temp_max;
	int main_pressure;
	int main_humidity;
	int visibility;
	float wind_speed;
	int wind_deg;
	int clouds_all;
	long dt;
	int sys_type;
	long sys_id;
	const char* sys_country;
	long sys_sunrise;
	long sys_sunset;
	int timezone;
	long id;
	const char* name;
	int cod;

	Model(StaticJsonDocument<1024>& doc) {
		this->coord_lon = doc["coord"]["lon"]; // 13.4334
		this->coord_lat = doc["coord"]["lat"]; // 52.295

		JsonObject weather_0 = doc["weather"][0];
		this->weather_0_id = weather_0["id"]; // 800
		this->weather_0_main = weather_0["main"]; // "Clear"
		this->weather_0_description = weather_0["description"]; // "clear sky"
		this->weather_0_icon = weather_0["icon"]; // "01d"
		this->base = doc["base"]; // "stations"

		JsonObject main = doc["main"];
		this->main_temp = main["temp"];
		this->main_feels_like = main["feels_like"]; // 281.02
		this->main_temp_min = main["temp_min"]; // 282.03
		this->main_temp_max = main["temp_max"]; // 284.41
		this->main_pressure = main["pressure"]; // 1021
		this->main_humidity = main["humidity"]; // 31
		this->visibility = doc["visibility"]; // 10000
		this->wind_deg = doc["wind"]["deg"]; // 170
		this->clouds_all = doc["clouds"]["all"]; // 0
		this->wind_speed = doc["wind"]["speed"]; // 4.12
		this->dt = doc["dt"]; // 1678976154

		JsonObject sys = doc["sys"];
		this->sys_type = sys["type"]; // 2
		this->sys_id = sys["id"]; // 2038480
		this->sys_country = sys["country"]; // "DE"
		this->sys_sunrise = sys["sunrise"]; // 1678943962
		this->sys_sunset = sys["sunset"]; // 1678986648
		this->timezone = doc["timezone"]; // 3600
		this->id = doc["id"]; //155348
		this->name = doc["name"]; //"London"
		this->cod = doc["cod"]; // 200
	}
};

class APIFetcher {
	HTTPClient http;
	String url = "https://api.openweathermap.org/data/2.5/weather?id=";
	StaticJsonDocument<1024> doc;
	
public:
	APIFetcher(const char* city, const char* apiKey)
	{
		url += city;
		url += "&appid=";
		url += apiKey;
		url += "&units=metric";
		http.useHTTP10(true);
	}

	
	std::unique_ptr<Model> get_data() {
		http.begin(url);
		int httpCode = http.GET();
		if (httpCode == 200) {
			DeserializationError error = deserializeJson(doc, http.getStream());
			if (error) {
				Serial.print("deserializeJson() failed: ");
				Serial.println(error.c_str());
			}
			std::unique_ptr<Model> ptr(new Model(doc));
			doc.clear();
			return ptr;
		}
		Serial.print("Error code: ");
		Serial.println(httpCode);
		doc.clear();
		http.end();
		return nullptr;
	}
};

template<class T>
class FixedBuffer {
	T* arr = nullptr;
	const size_t m_cap;

public:
	FixedBuffer(const size_t cap) :
		m_cap(cap)
	{
		arr = new T[cap];

		for (size_t i = 0; i != m_cap; i++) {
			arr[i] = T();
		}
	}

	~FixedBuffer() {
		delete[] arr;
	}

	T& operator[](const size_t idx) {
		assert(idx >= 0 && idx <= m_cap - 1);
		return arr[idx];
	}

	void push_and_pop(const T& elem) {
		for (size_t i = m_cap - 1; i >= 1; i--)
		{
			arr[i] = arr[i - 1];
		}
		arr[0] = elem;
	}

	const size_t size() const {
		return m_cap;
	}

	T* begin() {
		return arr;
	}

	T* end() {
		return arr + m_cap;
	}
};

struct Point2D {
	int32_t x, y;

	Point2D() :
		x(0), y(0)
	{}

	Point2D(int32_t x, int32_t y) :
		x(x), y(y)
	{}

	Point2D& operator=(const Point2D& other) {
		x = other.x;
		y = other.y;
		return *this;
	}

	Point2D operator+(const Point2D& other) const {
		return Point2D(x + other.x, y + other.y);
	}

	Point2D operator-(const Point2D& other) const {
		return Point2D(x - other.x, y - other.y);
	}

	Point2D operator*(const Point2D& other) const {
		return Point2D(x * other.x, y * other.y);
	}

	String toString() {
		return "[x: " + String(x) + " ,y: " + String(y) + "]";
	}
};

static bool isVowel(char c) {
	switch (c) {
	case 'a':
	case 'e':
	case 'i':
	case 'o':
	case 'u':
	case 'A':
	case 'E':
	case 'I':
	case 'O':
	case 'U':
		return true;
	default:
		return false;
	}
}

static String removeVowels(String input) {
	String output = "";
	int length = input.length();

	for (int i = 0; i < length; i++) {
		char currentChar = input.charAt(i);

		if (i == 0 || i == length - 1 || !isVowel(currentChar)) {
			output += currentChar;
		}
	}

	return output;
}

static boolean summertime_EU(int year, byte month, byte day, byte hour, byte tzHours)
{
	/* European Daylight Savings Time calculation by "jurs" for German Arduino Forum
	   input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
	   return value: returns true during Daylight Saving Time, false otherwise */

	if (month < 2 || month>9) { //Month zero indexed
		return false;
	}
	if (month > 2 && month < 9) {
		return true;
	}
	if (month == 2 && (hour + 24 * day) >= (1 + tzHours + 24 * (31 - (5 * year / 4 + 4) % 7)) || month == 9 && (hour + 24 * day) < (1 + tzHours + 24 * (31 - (5 * year / 4 + 1) % 7))) {
		Serial.println("DST");
		return true;
	}
	else {
		return false;
	}
}

static String unixToString(long unix)
{
	time_t t = static_cast<time_t>(unix);
	tm* mytm = gmtime(&t);
	auto hour = mytm->tm_hour;
	hour += (summertime_EU(mytm->tm_year, mytm->tm_mon, mytm->tm_mday, mytm->tm_hour, 0)) ? 2 : 1;
	String min = (mytm->tm_min < 10) ? "0" + String(mytm->tm_min) : String(mytm->tm_min);
	return String(hour) + ":" + min;
}

static const char* courses[16] = { "N", "NNO", "NO", "ONO", "O", "OSO", "SO", "SSO", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW" };

static String degToString(size_t dir)
{
	dir = static_cast<size_t>((dir / 22.5) + 0.5) % 16;
	return String(courses[dir]);
}