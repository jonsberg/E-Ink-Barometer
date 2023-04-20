#pragma once
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_213_B73.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <vector>

#include "model.h"

using DisplayType = GxEPD2_BW<GxEPD2_213_B73, GxEPD2_213_B73::HEIGHT>;

namespace Display {
	class DisplayWrapper;

	struct Frame {
		const Point2D upper_left;
		const int32_t width, height;

		Frame(uint32_t x, uint32_t y, int32_t w, int32_t h) :
			upper_left(Point2D(x, y)), width(w), height(h)
		{}
	};

	struct IComponent {
		virtual void paint(DisplayWrapper& display) = 0;
		virtual void update(const Model& data) = 0;
	};

	class DisplayWrapper : public DisplayType {
	public:

		DisplayWrapper() :
			DisplayType(GxEPD2_213_B73(SS, 17, 16, 4))  // GDEH0213B73
		{
			init(115200);
			setRotation(3);
			setTextColor(GxEPD_BLACK);
			setFullWindow();
			fillScreen(GxEPD_WHITE);
			Serial.println("Display Wrapper loaded");
		}

		void set_frame(const Frame& fr) {
			setPartialWindow(fr.upper_left.x, fr.upper_left.y,
				fr.width, fr.height);
		}

		void blank_frame(const Frame& fr) {
			fillRect(fr.upper_left.x, fr.upper_left.y,
				fr.width, fr.height, GxEPD_WHITE);
		}
	};

	class Table : public IComponent {
		class Cell {
			Point2D pxy;
			String message;

		public:
			Cell() :
				pxy(Point2D(0, 0)), message("")
			{}

			Cell(int32_t x, int32_t y) :
				pxy(Point2D(x, y)), message("")
			{}

			Cell(const Cell& other) :
				pxy(other.pxy), message(other.message)
			{}

			Cell(Cell&& other) noexcept :
				pxy(std::move(other.pxy)), message(std::move(other.message))
			{}

			Cell& operator=(const Cell& other) {
				if (this == &other) {
					return *this;
				}

				pxy = other.pxy;
				message = other.message;
				return *this;
			}

			Point2D offset_point(int32_t bx, int32_t by) const {
				int32_t xo = pxy.x + bx;
				int32_t yo = pxy.y + by;
				return Point2D(xo < 0 ? 0 : xo, yo < 0 ? 0 : yo);
			}

			void setMessage(const String& msg) {
				this->message = msg;
			}

			const String& getMessage() const {
				return message;
			}

			String printMessage() const {
				String str = "{";
				str += message;
				str += "}";
				return str;
			}

			String printCursor() const {
				String str = "{x: ";
				str += pxy.x;
				str += " y: ";
				str += pxy.y;
				str += "}";
				return str;
			}
		};
		const Frame& m_frame;
		const size_t rows;
		const size_t cols;
		const int32_t rowHeight;
		const int32_t colWidth;
		std::vector<std::vector<Cell>> cells;

	public:
		Table(size_t rows, size_t cols, const Frame& fr) :
			rows(rows), cols(cols), rowHeight(fr.height / rows), colWidth(fr.width / cols),
			cells(rows, std::vector<Cell>(cols)), m_frame(fr)
		{
			int32_t rowStep = rowHeight;
			for (size_t i = 0; i != rows; i++) {
				int32_t colStep = 0;
				for (size_t j = 0; j != cols; j++) {
					cells[i][j] = Cell(colStep, rowStep);
					colStep += colWidth;
				}
				rowStep += rowHeight;
			}
		}

		const std::vector<Cell>& operator[](int row) const {
			return cells[row];
		}

		Point2D centerElement(const Cell& cell, DisplayType& display) {
			int16_t tbx, tby; //not used
			uint16_t tbw = 0, tbh = 0;

			//Serial.println("OriginCursor: " + String(cell.printCursor()));

			if (!cell.getMessage().isEmpty()); {
				display.getTextBounds(cell.getMessage(), 0, 0, &tbx, &tby, &tbw, &tbh);
				//Serial.println("tbw: " + String(tbw) + " tbh: " + String(tbh));
				int32_t bx = (colWidth - tbw);
				bx = (bx < 0) ? bx : bx / 2;       //outta box?
				int32_t by = (rowHeight - tbh);
				by = (by < 0) ? by : by / 2;
				//Serial.println("bx: " + String(bx) + " by: " + String(by));
				Point2D res = cell.offset_point(bx, -by); //should
				//Serial.println("OffsetCursor: " + res.toString());
				//Serial.println();
				return res;
			}
		}

		virtual void paint(DisplayWrapper& display) override {
			Serial.println("Draw Table");
			display.set_frame(m_frame);
			display.setFont(&FreeMonoBold12pt7b);
			display.firstPage();

			do {
				display.blank_frame(m_frame);
				for (size_t i = 0; i != rows; i++)
				{
					for (size_t j = 0; j != cols; j++)
					{
						const Cell& c = cells[i][j]; //changed
						Point2D offs = centerElement(c, display);
						display.setCursor(offs.x, offs.y);
						display.print(c.getMessage());
					}
				}
			} while (display.nextPage());
		}

		size_t cycle = 0;

		void update(const Model& data) {
			Serial.println("Update Table");
			cells[0][0].setMessage("@" + unixToString(data.dt));
			cells[0][2].setMessage(removeVowels(data.name));

			cells[1][0].setMessage(String((int)(data.main_temp)) + "C");
			cells[1][1].setMessage(String(data.main_humidity) + "%");
			cells[1][2].setMessage(String((int)(data.wind_speed/*m/s*/ * 1.94)) + "kn");

			cells[2][0].setMessage(String(data.main_pressure) + "hPa ");
			cells[2][1].setMessage(String(cycle));
			cells[2][2].setMessage(degToString(data.wind_deg));
			cycle++;
		}
	};

	template<class T>
	static int32_t linear_transform(const T& val, const T& curr_min, const T& curr_max, const T& target_min, const T& target_max) {
		if (curr_min >= curr_max) {
			Serial.printf("linearTransform: min >= max %d , %d", curr_min, curr_max);
			return 0;
		}
		//(Wert - Minimum Wert)* (Neuer Maximalwert - Neuer Minimalwert) / (Maximalwert - Minimalwert) + Neuer Minimalwert
		return static_cast<int32_t>((val - curr_min) * (target_max - target_min)) / (curr_max - curr_min) + target_min;
	}

	class Barometer : public IComponent {
		FixedBuffer<int32_t> values;
		const Frame& m_frame;
		const int32_t y_max, y_min; //pixels
		long last_entry = 0;

	public:
		Barometer(size_t hours, const Frame& fr) :
			values(hours), y_min(0), y_max(fr.height), m_frame(fr)
		{
			for (auto i : values) {
				values[i] = 0;
			}
		}

//#define TEST
#ifdef TEST
		std::vector<int32_t> test_vec;

		virtual void update(const Model& data) override {
			int32_t start = 950;
			for (auto i : values) {
				test_vec.push_back(linear_transform<int32_t>(start, 950, 1050, y_min, y_max));
				start += 4;
			}
		}

		virtual void paint(DisplayWrapper& display) override {
			Serial.println("Drawing Barometer");
			static const int32_t width = 9;
			static const int32_t gap = 1;
			display.set_frame(m_frame);
			do
			{
				display.blank_frame(m_frame);
				int32_t cursor_x = m_frame.upper_left.x + m_frame.width - width + gap; //offset move right
				for (const auto i : test_vec)
				{
					display.fillRect(cursor_x, m_frame.upper_left.y + m_frame.height, width, -i, GxEPD_BLACK);
					Serial.printf("cursor_x set: %d , val: %d\n", cursor_x, i);
					cursor_x -= (width + gap);
				}
			} while (display.nextPage());
		}
#endif
#ifndef TEST
		virtual void update(const Model& data) override {
			if (data.dt - last_entry >= 3600) {
				Serial.println("Update Barometer");
				auto s = linear_transform<int32_t>(data.main_pressure, 950, 1050, y_min, y_max);
				values.push_and_pop(s);
				last_entry = data.dt;
			}
		}

		virtual void paint(DisplayWrapper& display) override {
			Serial.println("Drawing Barometer");

			static const int32_t width = 9;
			static const int32_t gap = 1;
			display.set_frame(m_frame);
			do
			{
				display.blank_frame(m_frame);
				int32_t cursor_x = m_frame.upper_left.x + m_frame.width - width + gap; //offset

				for (const auto i : values)
				{
					display.fillRect(cursor_x, m_frame.upper_left.y + m_frame.height, width, -i, GxEPD_BLACK);
					Serial.printf("cursor_x set: %d , val: %d\n", cursor_x, i);
					cursor_x -= (width + gap);
				}
			} while (display.nextPage());
		}
#endif
	};

	class OverlayHandler {
		DisplayWrapper& m_display;
		std::vector<IComponent*> components;

	public:
		OverlayHandler(DisplayWrapper& display) :
			m_display(display)
		{}

		~OverlayHandler() {
			for (size_t i = 0; i != components.size(); i++)
			{
				delete components[i];
			}
			components.clear();
		}

		void add_component(IComponent* comp) {
			components.push_back(comp);
		}

		const std::vector<IComponent*>& get_components() {
			return components;
		}
	};
}