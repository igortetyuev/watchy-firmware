#ifndef SETTINGS_H
#define SETTINGS_H

//Weather Settings
#ifndef CITY_ID
#define CITY_ID "5128581" //New York City https://openweathermap.org/current#cityid
#endif

//You can also use LAT,LON for your location instead of CITY_ID, but not both
//#define LAT "40.7127" //New York City, Looked up on https://www.latlong.net/
//#define LON "-74.0059"

#ifdef CITY_ID
    #define OPENWEATHERMAP_URL "http://api.openweathermap.org/data/2.5/weather?id={cityID}&lang={lang}&units={units}&appid={apiKey}" //open weather api using city ID
#else
    #define OPENWEATHERMAP_URL "http://api.openweathermap.org/data/2.5/weather?lat={lat}&lon={lon}&lang={lang}&units={units}&appid={apiKey}" //open weather api using lat lon
#endif

#ifndef OPENWEATHERMAP_APIKEY
#define OPENWEATHERMAP_APIKEY "" //set OPENWEATHERMAP_APIKEY in .env (no key = internal temperature sensor only)
#endif
#ifndef TEMP_UNIT
#define TEMP_UNIT "metric" //metric = Celsius , imperial = Fahrenheit
#endif
#ifndef TEMP_LANG
#define TEMP_LANG "en"
#endif
#define WEATHER_UPDATE_INTERVAL 30 //must be greater than 5, measured in minutes
//NTP Settings
#define NTP_SERVER "pool.ntp.org"
#ifndef GMT_OFFSET_SEC
#define GMT_OFFSET_SEC 0 //UTC; set in .env (seconds) or change in the watch menu
#endif
#ifndef VIBRATE_OCLOCK
#define VIBRATE_OCLOCK 1 //short buzz every full hour; set VIBRATE_OCLOCK=0 in .env to disable
#endif

watchySettings settings{
    #ifdef CITY_ID
        .cityID = CITY_ID,
    #else
        .cityID = "",
        .lat = LAT,
        .lon = LON,
    #endif
    .weatherAPIKey = OPENWEATHERMAP_APIKEY,
    .weatherURL = OPENWEATHERMAP_URL,
    .weatherUnit = TEMP_UNIT,
    .weatherLang = TEMP_LANG,
    .weatherUpdateInterval = WEATHER_UPDATE_INTERVAL,
    .ntpServer = NTP_SERVER,
    .gmtOffset = GMT_OFFSET_SEC,
    .vibrateOClock = VIBRATE_OCLOCK != 0,
};

#endif