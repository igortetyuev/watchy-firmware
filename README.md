# Watchy firmware

Firmware for the [SQFMI Watchy](https://watchy.sqfmi.com) e-paper watch
(ESP32-PICO-D4, v2.0), built with PlatformIO. Based on the upstream 7-SEG
example with a localized UI, nine analog faces, a face picker, a multi-network
Wi-Fi list, a user-settable time zone and a daily alarm.

The Watchy library is a git submodule (`lib/Watchy`, a fork of
[sqfmi/Watchy](https://github.com/sqfmi/Watchy)):

```sh
git clone --recursive https://github.com/igortetyuev/watchy-firmware.git
cd watchy-firmware
cp .env.example .env     # then edit, see below
pio run                                                        # build
pio run -t upload --upload-port /dev/cu.usbserial-XXXXXXXX     # flash
```

## Configuration (`.env`)

All personal settings live in a `.env` file next to `platformio.ini`. It is
gitignored; `.env.example` lists every key. At build time `load_env.py` turns
the values into `-D` compiler flags and `gen_lang.py` picks the UI language, so
**changing `.env` requires a rebuild and reflash**.

```ini
WIFI_SSID=MyNetwork
WIFI_PASSWD=secret
GMT_OFFSET_SEC=7200
LANG=en
OPENWEATHERMAP_APIKEY=
CITY_ID=5128581
TEMP_UNIT=metric
TEMP_LANG=en
VIBRATE_OCLOCK=1
```

### UI language (`LANG`)

One language per build. Supported values:

| `LANG` | Language   | Font                        |
|--------|------------|-----------------------------|
| `en`   | English    | Latin9pt7b (FreeSans, ISO-8859-1) |
| `de`   | Deutsch    | Latin9pt7b                  |
| `es`   | Español    | Latin9pt7b                  |
| `fr`   | Français   | Latin9pt7b                  |
| `pt`   | Português  | Latin9pt7b                  |
| `ru`   | Русский    | Cyrillic9pt7b               |

To switch, set `LANG=xx` in `.env` and run `pio run -t upload`. The strings are
in `lib/Watchy/lang/<xx>.json`; `en.json` is the reference — every language
must define the same keys or the build fails with a list of what is missing.
To add a language, copy `en.json`, translate it (keep lines short: a 9 pt line
holds about 18–20 characters on the 200 px screen) and, if the script needs
glyphs outside ISO-8859-1 or Cyrillic, add a font.

Only menu and app screens are localized; the weather description language
(`TEMP_LANG`) is a separate OpenWeatherMap setting.

### Time zone (`GMT_OFFSET_SEC`)

Offset from UTC in seconds, e.g. `7200` for UTC+2, `-18000` for UTC−5. This is
only the **default** used on first boot (or after an NVS erase). The watch then
keeps its own value, editable in the *Time zone* menu in 30-minute steps
(UP = earlier, DOWN = later, MENU saves). The RTC holds local time; saving a
new offset shifts the clock immediately and NTP syncs keep using it.

### Wi-Fi (`WIFI_SSID`, `WIFI_PASSWD`) and the setup AP

The watch keeps an ordered list of up to five networks in NVS and tries them
in order when it needs the network (NTP sync, weather). `WIFI_SSID` /
`WIFI_PASSWD` seed the first entry of that list on first boot only; after that
the list is managed on the watch in *Wi-Fi setup*:

- UP/DOWN select, MENU opens a network (*Setup via AP* / *Delete*),
  *Add* appends a new one;
- long-press BACK (≥ 0.7 s) enters reorder mode: UP/DOWN move the network,
  BACK saves, MENU cancels.

*Setup via AP* (and *Add*) starts the WiFiManager captive portal: the watch
broadcasts an open access point named **`Watchy AP`** and shows its IP on
screen. Connect to it from a phone, pick your network and enter the password;
the credentials are stored in the selected slot. The portal times out after
60 s. Hidden SSIDs are not supported (the watch only connects to networks it
sees in a scan).

Leaving `WIFI_SSID` empty is fine — just add networks through the AP later.

### Weather (`OPENWEATHERMAP_APIKEY`, `CITY_ID`, `TEMP_UNIT`, `TEMP_LANG`)

Used by the digital face. Without an API key the face shows the on-board
temperature sensor instead. `CITY_ID` is an OpenWeatherMap city id,
`TEMP_UNIT` is `metric` or `imperial`.

### Hourly vibration (`VIBRATE_OCLOCK`)

`1` (default) makes the watch buzz briefly at every full hour, `0` turns it
off. Build-time only; there is no menu item for it.

## Alarm

*Alarm* in the menu: UP/DOWN move focus between hours, minutes and *Enabled*;
MENU increments (hold 2 s for fast repeat), BACK saves. When it rings, one
right button snoozes for 10 minutes, both right buttons together turn it off.
If the watch misses the exact minute (the RTC wake can be late) it still rings
on the first wake within the next 5 minutes.

## Hardware notes

Buttons: BACK top-left, MENU bottom-left, UP top-right, DOWN bottom-right.
Boot banner on serial (115200) on every wake: `BOOT rev=2.0 rtc=PCF8563 …`.
See `AGENTS.md` for build/flash gotchas and library conventions.
