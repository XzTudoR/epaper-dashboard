#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "icons.h"

#if __has_include("config.h")
  #include "config.h"
#else
  #include "config.h.example"
#endif

// Display type definition for Good Display GDEY042T81 (400x300 B/W)
using DashboardDisplay = GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT>;

namespace DisplayUI {

  // Truncate text with ellipsis if it exceeds maximum pixel width
  inline String truncateText(U8G2_FOR_ADAFRUIT_GFX& fonts, const String& text, int maxWidth) {
    if (fonts.getUTF8Width(text.c_str()) <= maxWidth) {
      return text;
    }
    String shortened = text;
    while (shortened.length() > 0 && fonts.getUTF8Width((shortened + "..").c_str()) > maxWidth) {
      shortened.remove(shortened.length() - 1);
    }
    return shortened + "..";
  }

  // Map weather code integer to corresponding 32x32 PROGMEM icon
  inline const unsigned char* getWeatherBitmap(int meteoIcon) {
    switch (meteoIcon) {
      case 1: return icon_sun_cloud;
      case 2: return icon_cloud;
      case 3: return icon_rain;
      case 4: return icon_storm;
      case 5: return icon_snow;
      case 0:
      default: return icon_sun;
    }
  }

  // Draw header section (Watermark, Day of Week, Date, Weather with icon)
  inline void drawHeader(DashboardDisplay& display, U8G2_FOR_ADAFRUIT_GFX& fonts,
                         const String& ziua, const String& dataStr,
                         const String& vremeaStr, int meteoIcon) {
    // Watermark label
    if (strlen(WATERMARK_LABEL) > 0) {
      fonts.setFont(u8g2_font_helvR10_tr);
      fonts.setCursor(5, 15);
      fonts.print(WATERMARK_LABEL);
    }

    // Centered Day of the Week
    fonts.setFont(u8g2_font_helvB18_tr);
    int textWidthZiua = fonts.getUTF8Width(ziua.c_str());
    fonts.setCursor((400 - textWidthZiua) / 2, 35);
    fonts.print(ziua);

    // Centered Date
    fonts.setFont(u8g2_font_helvR14_tr);
    int textWidthData = fonts.getUTF8Width(dataStr.c_str());
    fonts.setCursor((400 - textWidthData) / 2, 60);
    fonts.print(dataStr);

    // Centered Weather & Icon
    fonts.setFont(u8g2_font_helvB14_tr);
    int textWidthVremea = fonts.getUTF8Width(vremeaStr.c_str());
    int totalWeatherWidth = textWidthVremea + 8 + 32;
    int startWeatherX = (400 - totalWeatherWidth) / 2;

    fonts.setCursor(startWeatherX, 85);
    fonts.print(vremeaStr);

    const unsigned char* weatherBitmap = getWeatherBitmap(meteoIcon);
    display.drawBitmap(startWeatherX + textWidthVremea + 8, 62, weatherBitmap, 32, 32, GxEPD_WHITE, GxEPD_BLACK);
  }

  // Draw dividing grid lines
  inline void drawSeparators(DashboardDisplay& display) {
    display.drawLine(240, 100, 240, 290, GxEPD_BLACK);
    display.drawLine(240, 195, 400, 195, GxEPD_BLACK);
  }

  // Draw left section: Schedule / Classes
  inline void drawSchedule(DashboardDisplay& display, U8G2_FOR_ADAFRUIT_GFX& fonts, JsonArrayConst orar) {
    display.drawBitmap(76, 96, icon_cap, 46, 46, GxEPD_WHITE, GxEPD_BLACK);
    display.drawBitmap(132, 102, icon_book, 33, 33, GxEPD_WHITE, GxEPD_BLACK);

    int count = orar.size();
    if (count == 0) {
      fonts.setFont(u8g2_font_helvR12_tr);
      const char* msg = "Liber azi!";
      int tw = fonts.getUTF8Width(msg);
      fonts.setCursor((240 - tw) / 2, 210);
      fonts.print(msg);
    } else if (count <= 3) {
      fonts.setFont(u8g2_font_helvB12_tr);
      int yStart = (count == 1) ? 210 : ((count == 2) ? 195 : 175);

      for (JsonVariantConst v : orar) {
        String txt = truncateText(fonts, v.as<String>(), 230);
        int tw = fonts.getUTF8Width(txt.c_str());
        fonts.setCursor((240 - tw) / 2, yStart);
        fonts.print(txt);
        yStart += 35;
      }
    } else {
      fonts.setFont(u8g2_font_helvB10_tr);
      int yStart = 160;
      for (JsonVariantConst v : orar) {
        if (yStart > 290) break;
        String txt = truncateText(fonts, v.as<String>(), 230);
        fonts.setCursor(5, yStart);
        fonts.print(txt);
        yStart += 22;
      }
    }
  }

  // Draw top right section: Tasks
  inline void drawTasks(DashboardDisplay& display, U8G2_FOR_ADAFRUIT_GFX& fonts, JsonArrayConst tasks) {
    display.drawBitmap(305, 75, icon_tasks, 30, 30, GxEPD_WHITE, GxEPD_BLACK);

    int count = tasks.size();
    if (count == 0) {
      fonts.setFont(u8g2_font_helvR12_tr);
      const char* msg = "Niciun task.";
      int tw = fonts.getUTF8Width(msg);
      fonts.setCursor(240 + (160 - tw) / 2, 145);
      fonts.print(msg);
    } else if (count <= 2) {
      fonts.setFont(u8g2_font_helvB12_tr);
      int yPos = 145;
      for (JsonVariantConst v : tasks) {
        String txt = truncateText(fonts, v.as<String>(), 150);
        int tw = fonts.getUTF8Width(txt.c_str());
        fonts.setCursor(240 + (160 - tw) / 2, yPos);
        fonts.print(txt);
        yPos += 25;
      }
    } else {
      fonts.setFont(u8g2_font_helvR10_tr);
      int yPos = 125;
      for (JsonVariantConst v : tasks) {
        if (yPos > 185) break;
        String txt = truncateText(fonts, v.as<String>(), 150);
        int tw = fonts.getUTF8Width(txt.c_str());
        fonts.setCursor(240 + (160 - tw) / 2, yPos);
        fonts.print(txt);
        yPos += 18;
      }
    }
  }

  // Draw bottom right section: Events and Birthdays
  inline void drawEvents(DashboardDisplay& display, U8G2_FOR_ADAFRUIT_GFX& fonts, JsonArrayConst evenimente) {
    display.drawBitmap(300, 202, icon_tortGroup, 40, 40, GxEPD_BLACK);

    int count = evenimente.size();
    if (count == 0) {
      fonts.setFont(u8g2_font_helvR12_tr);
      const char* msg = "...";
      int tw = fonts.getUTF8Width(msg);
      fonts.setCursor(240 + (160 - tw) / 2, 265);
      fonts.print(msg);
    } else if (count <= 2) {
      fonts.setFont(u8g2_font_helvB10_tr);
      int yPos = 260;
      for (JsonVariantConst v : evenimente) {
        String txt = truncateText(fonts, v.as<String>(), 150);
        int tw = fonts.getUTF8Width(txt.c_str());
        fonts.setCursor(240 + (160 - tw) / 2, yPos);
        fonts.print(txt);
        yPos += 22;
      }
    } else {
      fonts.setFont(u8g2_font_helvR10_tr);
      int yPos = 255;
      for (JsonVariantConst v : evenimente) {
        if (yPos > 300) break;
        String txt = truncateText(fonts, v.as<String>(), 150);
        int tw = fonts.getUTF8Width(txt.c_str());
        fonts.setCursor(240 + (160 - tw) / 2, yPos);
        fonts.print(txt);
        yPos += 15;
      }
    }
  }

  // Master render function executing full UI refresh cycle
  inline void renderDashboard(DashboardDisplay& display, U8G2_FOR_ADAFRUIT_GFX& fonts, const JsonDocument& doc) {
    String ziua      = doc["ziua"] | "LUNI";
    String dataStr   = doc["data"] | "--";
    String vremeaStr = doc["vremea"] | "--°C";
    int meteoIcon    = doc["meteo_icon"] | 0;

    display.setFullWindow();
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);

      drawHeader(display, fonts, ziua, dataStr, vremeaStr, meteoIcon);
      drawSeparators(display);
      drawSchedule(display, fonts, doc["orar"].as<JsonArrayConst>());
      drawTasks(display, fonts, doc["tasks"].as<JsonArrayConst>());
      drawEvents(display, fonts, doc["evenimente"].as<JsonArrayConst>());

    } while (display.nextPage());
  }

} // namespace DisplayUI
