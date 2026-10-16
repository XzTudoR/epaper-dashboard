/**
 * ==============================================================================
 * Google Apps Script - E-Paper Smart Dashboard Backend
 * ==============================================================================
 * Aggregates Open-Meteo weather forecast, Google Calendar schedules,
 * contact birthdays, and Google Tasks into a lightweight JSON feed for ESP32.
 * ==============================================================================
 */

// --- Global Configuration ---
const CONFIG = {
  // Timezone for date formatting and day calculations
  TIMEZONE: "Europe/Bucharest",

  // Geographic coordinates for Open-Meteo weather API (Default: Bucharest)
  LATITUDE: 44.4323,
  LONGITUDE: 26.1063,

  // Hour thresholds
  SCHEDULE_SWITCH_HOUR: 18,      // After 18:00, show the next day's schedule (skipping weekends)
  WEATHER_TOMORROW_HOUR: 22,     // After 22:00, show tomorrow's maximum temperature
  WEATHER_CURRENT_HOUR: 13,      // Between 13:00 and 21:59, show live temperature; before 13:00, today's max

  // Lookahead window for upcoming birthdays and all-day events (in days)
  UPCOMING_DAYS: 7,

  // Google Contacts Birthday Calendar ID
  BIRTHDAY_CALENDAR_ID: "addressbook#contacts@group.v.calendar.google.com"
};

/**
 * Main Web App Entrypoint (HTTP GET)
 * Returns application/json payload consumed by the ESP32 firmware
 */
function doGet(e) {
  try {
    const now = new Date();
    const currentDate = new Date(Utilities.formatDate(now, CONFIG.TIMEZONE, "yyyy-MM-dd'T'HH:mm:ss"));
    const currentHour = currentDate.getHours();

    // 1. Fetch Weather Data
    const weather = fetchWeatherData(currentHour);

    // 2. Fetch Schedule and Upcoming Events
    const { orar, evenimente } = fetchCalendarData(now, currentDate, currentHour);

    // 3. Fetch Google Tasks
    const tasks = fetchGoogleTasks();

    // 4. Localization labels
    const dateLabels = getDateLabels(currentDate);

    const responsePayload = {
      ziua: dateLabels.ziua,
      data: dateLabels.data,
      vremea: weather.temperature,
      meteo_icon: weather.iconCode,
      orar: orar,
      tasks: tasks,
      evenimente: evenimente
    };

    return ContentService.createTextOutput(JSON.stringify(responsePayload))
      .setMimeType(ContentService.MimeType.JSON);

  } catch (error) {
    console.error("[ERROR] Unhandled exception in doGet: " + error.toString());
    const errorPayload = {
      ziua: "EROARE",
      data: "--",
      vremea: "--°C",
      meteo_icon: 0,
      orar: ["Eroare server"],
      tasks: [],
      evenimente: []
    };
    return ContentService.createTextOutput(JSON.stringify(errorPayload))
      .setMimeType(ContentService.MimeType.JSON);
  }
}

/**
 * Fetch and parse weather data from Open-Meteo
 */
function fetchWeatherData(currentHour) {
  let temperature = "--°C";
  let iconCode = 0; // 0=Sun, 1=Sun+Cloud, 2=Cloud, 3=Rain, 4=Storm, 5=Snow

  try {
    const url = `https://api.open-meteo.com/v1/forecast?latitude=${CONFIG.LATITUDE}&longitude=${CONFIG.LONGITUDE}&current_weather=true&daily=temperature_2m_max,weathercode&timezone=${encodeURIComponent(CONFIG.TIMEZONE)}`;
    const response = UrlFetchApp.fetch(url, { muteHttpExceptions: true });
    const json = JSON.parse(response.getContentText());

    if (json.daily && json.current_weather) {
      let code;

      if (currentHour >= CONFIG.WEATHER_TOMORROW_HOUR) {
        // After 22:00 -> Tomorrow's maximum
        temperature = Math.round(json.daily.temperature_2m_max[1]) + "°C";
        code = json.daily.weathercode[1];
      } else if (currentHour >= CONFIG.WEATHER_CURRENT_HOUR) {
        // Between 13:00 and 21:59 -> Live current temperature
        temperature = Math.round(json.current_weather.temperature) + "°C";
        code = json.current_weather.weathercode;
      } else {
        // Until 13:00 -> Today's maximum
        temperature = Math.round(json.daily.temperature_2m_max[0]) + "°C";
        code = json.daily.weathercode[0];
      }

      iconCode = mapWeatherCodeToIcon(code);
    }
  } catch (err) {
    console.error("[WARN] Failed to fetch weather: " + err.toString());
    temperature = "--°C";
  }

  return { temperature, iconCode };
}

/**
 * Map WMO weather interpretation code to 0-5 icon indices
 */
function mapWeatherCodeToIcon(code) {
  if (code === 0) return 0; // Clear sky
  if (code === 1 || code === 2) return 1; // Mainly clear, partly cloudy
  if (code === 3 || code === 45 || code === 48) return 2; // Overcast, fog
  if ([51, 53, 55, 56, 57, 61, 63, 65, 66, 67, 80, 81, 82].indexOf(code) !== -1) return 3; // Rain / Drizzle / Showers
  if ([95, 96, 99].indexOf(code) !== -1) return 4; // Thunderstorm
  if ([71, 73, 75, 77, 85, 86].indexOf(code) !== -1) return 5; // Snow
  return 0;
}

/**
 * Fetch calendar events (timed classes for schedule, all-day events and birthdays)
 */
function fetchCalendarData(now, currentDate, currentHour) {
  // Determine target schedule date
  const scheduleDate = new Date(currentDate.getTime());
  if (currentHour >= CONFIG.SCHEDULE_SWITCH_HOUR) {
    scheduleDate.setDate(scheduleDate.getDate() + 1);
  }

  // Skip weekend days forward to Monday
  const dayOfWeek = scheduleDate.getDay();
  if (dayOfWeek === 6) { // Saturday -> Monday
    scheduleDate.setDate(scheduleDate.getDate() + 2);
  } else if (dayOfWeek === 0) { // Sunday -> Monday
    scheduleDate.setDate(scheduleDate.getDate() + 1);
  }

  const startSchedule = new Date(scheduleDate.getFullYear(), scheduleDate.getMonth(), scheduleDate.getDate(), 0, 0, 0);
  const endSchedule = new Date(scheduleDate.getFullYear(), scheduleDate.getMonth(), scheduleDate.getDate(), 23, 59, 59);

  const startEvents = new Date(currentDate.getFullYear(), currentDate.getMonth(), currentDate.getDate(), 0, 0, 0);
  const endEvents = new Date(currentDate.getFullYear(), currentDate.getMonth(), currentDate.getDate() + CONFIG.UPCOMING_DAYS, 23, 59, 59);

  const orarList = [];
  const evenimenteList = [];

  const shortMonths = ["ian", "feb", "mar", "apr", "mai", "iun", "iul", "aug", "sep", "oct", "nov", "dec"];

  function addUpcomingEvent(title, eventStart) {
    const dEv = new Date(Utilities.formatDate(eventStart, CONFIG.TIMEZONE, "yyyy-MM-dd") + "T00:00:00");
    const dNow = new Date(Utilities.formatDate(now, CONFIG.TIMEZONE, "yyyy-MM-dd") + "T00:00:00");
    const diffDays = Math.round((dEv - dNow) / (1000 * 60 * 60 * 24));

    if (diffDays < 0) return;

    let cleanTitle = title
      .replace(/birthday/gi, "bday")
      .replace(/zi de naștere/gi, "bday")
      .replace(/zi de nastere/gi, "bday");

    const dateFormatted = eventStart.getDate() + " " + shortMonths[eventStart.getMonth()];
    let formatted = cleanTitle + " - " + dateFormatted;

    if (diffDays === 0) formatted += " (Azi!)";
    else if (diffDays === 1) formatted += " (mâine)";

    if (evenimenteList.indexOf(formatted) === -1) {
      evenimenteList.push(formatted);
    }
  }

  // 1. Google Contacts Birthdays
  try {
    const birthdayCal = CalendarApp.getCalendarById(CONFIG.BIRTHDAY_CALENDAR_ID);
    if (birthdayCal) {
      const bdays = birthdayCal.getEvents(startEvents, endEvents);
      bdays.forEach(ev => addUpcomingEvent(ev.getTitle(), ev.getStartTime()));
    }
  } catch (err) {
    console.warn("[WARN] Could not read birthday calendar: " + err.toString());
  }

  // 2. User Google Calendars
  try {
    const calendars = CalendarApp.getAllCalendars();
    for (let c = 0; c < calendars.length; c++) {
      if (calendars[c].getId() === CONFIG.BIRTHDAY_CALENDAR_ID) continue;

      // All-day events for upcoming days list
      const upcoming = calendars[c].getEvents(startEvents, endEvents);
      for (let i = 0; i < upcoming.length; i++) {
        if (upcoming[i].isAllDayEvent()) {
          addUpcomingEvent(upcoming[i].getTitle(), upcoming[i].getStartTime());
        }
      }

      // Timed schedule events for schedule target date
      const scheduleEvents = calendars[c].getEvents(startSchedule, endSchedule);
      for (let j = 0; j < scheduleEvents.length; j++) {
        if (!scheduleEvents[j].isAllDayEvent()) {
          const timeStr = Utilities.formatDate(scheduleEvents[j].getStartTime(), CONFIG.TIMEZONE, "HH:mm");
          let item = timeStr + " " + scheduleEvents[j].getTitle();
          if (scheduleEvents[j].getLocation()) {
            item += " (" + scheduleEvents[j].getLocation().split(',')[0].trim() + ")";
          }
          if (orarList.indexOf(item) === -1) {
            orarList.push(item);
          }
        }
      }
    }
  } catch (err) {
    console.error("[ERROR] Failed to read user calendars: " + err.toString());
  }

  return { orar: orarList, evenimente: evenimenteList };
}

/**
 * Fetch incomplete tasks from Google Tasks API
 */
function fetchGoogleTasks() {
  const tasksList = [];
  try {
    const taskLists = Tasks.Tasklists.list().getItems();
    if (taskLists && taskLists.length > 0) {
      const tasks = Tasks.Tasks.list(taskLists[0].getId(), {
        showCompleted: false,
        showHidden: false
      }).getItems();

      if (tasks) {
        for (let i = 0; i < tasks.length; i++) {
          tasksList.push(tasks[i].getTitle());
        }
      }
    }
  } catch (err) {
    console.warn("[WARN] Google Tasks fetch failed: " + err.toString());
    tasksList.push("Eroare API");
  }
  return tasksList;
}

/**
 * Return localized Romanian day and month names
 */
function getDateLabels(date) {
  const daysRo = ["DUMINICA", "LUNI", "MARTI", "MIERCURI", "JOI", "VINERI", "SAMBATA"];
  const monthsRo = [
    "ianuarie", "februarie", "martie", "aprilie", "mai", "iunie",
    "iulie", "august", "septembrie", "octombrie", "noiembrie", "decembrie"
  ];

  return {
    ziua: daysRo[date.getDay()],
    data: date.getDate() + " " + monthsRo[date.getMonth()]
  };
}
