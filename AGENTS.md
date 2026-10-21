# Watchy firmware — project notes & gotchas

Read this before touching the code. Append new gotchas here in the same
symptom → cause → rule form when they cost real debugging time.

## Layout

- `src/` — app: `Watchy_7_SEG.cpp/.h` (digital face 0, battery/weather helpers),
  `Watchy_Analog.cpp` (analog faces 1–9), `main.cpp` (boot banner), `settings.h`,
  `icons.h`, DSEG7 fonts.
- `lib/Watchy` is a **git submodule** → `github.com/igortetyuev/watchy` (fork of
  upstream sqfmi/Watchy 1.4.14; `upstream` remote in `~/projects/Watchy`). Library
  edits (`src/Watchy.cpp`, `Watchy.h`, `config.h`, `Cyrillic9pt7b.h`) are made
  **inside `lib/Watchy`**, committed there and pushed, then the superproject
  commit bumps the gitlink. `~/projects/Watchy` is an older standalone clone of
  the same fork — don't edit both. Fresh clone: `git clone --recursive`.
- Personal values live in `.env` (gitignored, see `.env.example`) and are
  injected as `-D` flags by `load_env.py` (`extra_scripts = pre:`):
  `WIFI_SSID`, `WIFI_PASSWD`, `GMT_OFFSET_SEC`, optional
  `OPENWEATHERMAP_APIKEY`, `CITY_ID`, `TEMP_UNIT`, `TEMP_LANG`.
  `settings.h` wraps the same macros in `#ifndef`.
- `~/projects/twatch-firmware` is a **different device** (LilyGO T‑Watch 2020,
  `/dev/cu.SLAB_USBtoUART`). Never mix the two.

## Hardware (verified with esptool)

- Watchy **v2.0**: ESP32‑PICO‑D4 rev 1.1, 4 MB flash, no PSRAM, PCF8563 RTC,
  UP button GPIO35, battery ADC GPIO34. `board = esp32dev` is correct.
- Serial port: `/dev/cu.usbserial-56230044651`, 115200 baud.
- Buttons: **BACK = top‑left** (GPIO25), **MENU = bottom‑left** (26),
  **UP = top‑right** (35), **DOWN = bottom‑right** (4). Pressed == `ACTIVE_LOW`.
- Do **not** add `-D ARDUINO_WATCHY_Vxx` to `platformio.ini`: the guard in
  `config.h` would then skip all pin defines. Revision is set in `config.h`.

## Build / flash / verify

```bash
cd ~/projects/watchy-firmware
pio run                                                        # build (~65% flash)
pio run -t upload --upload-port /dev/cu.usbserial-56230044651  # flash
pio device monitor -b 115200                                   # or pyserial, ~15 s
```

- Boot banner on every wake: `BOOT rev=2.0 rtc=PCF8563 batt=4.09V time=...`.
  Expect one per minute (RTC tick). Absence or repeating resets = panic.
- Harmless log noise: `Wire.cpp:152 Can't allocate memory for I2C_0`
  (`Wire.begin` is called twice: `main.cpp` and `Watchy::init`) and
  `nvs_open failed: NOT_FOUND` (before the first NVS save).
- `upload_speed = 115200`; higher rates are flaky on this USB bridge. Even at
  115200 an upload occasionally crawls at ~26 kbit/s (≈7 min for 1.3 MB) and
  still verifies fine — don't abort it, just wait.
- **Identify the board before flashing anything.** Symptom: `firmware.bin
  (length 4294240) … will not fit in 4194304 bytes of flash`. Cause: the
  T‑Watch env was flashed at a Watchy. Rule: `esptool.py flash_id` first —
  `ESP32-PICO-D4`, 4 MB, `/dev/cu.usbserial-56230044651` = Watchy;
  16 MB, `/dev/cu.SLAB_USBtoUART` = T‑Watch. The shell cwd says nothing about
  which watch is plugged in.
- **Stale object after editing the library.** Symptom: build succeeds but the
  watch runs the old code (an edit to `lib/Watchy/src/Watchy.cpp` didn't make
  it in). Cause: edits with a backdated mtime (checkout, submodule update) are
  not seen by SCons, so `Watchy.cpp.o` is reused. Rule: after editing library
  files check the build log recompiles `Watchy.cpp`; if not, delete
  `.pio/build/watchy/lib*/Watchy/Watchy.cpp.o` (or `pio run -t clean`) and rebuild.
- The e‑paper cannot be captured from the host — drawing changes must be checked
  on the watch by eye. Build + boot log only prove it doesn't crash.

## Library conventions (`lib/Watchy/src`)

- **State across deep sleep:** `RTC_DATA_ATTR` globals (`guiState`, `menuIndex`,
  `gmtOffset`, `faceIndex`, …) survive sleep but **not** reset/battery drain.
  Anything persistent goes to NVS `Preferences` namespace `"watchy"` and is
  reloaded in `init()`'s `default:` (reset) branch. Keys: `gmtOffset` (long),
  `face` (uchar), `alarmH`/`alarmM` (uchar), `alarmOn` (bool),
  `wifiCount` + `wifiSsidN`/`wifiPassN` (N = 0..4).
- **Adding a menu item** = 4 places: both `menuItems[]` arrays (`showMenu`,
  `showFastMenu`) and both `switch (menuIndex)` blocks in `handleButtonPress`
  (wake‑up path and the 5 s fast‑menu loop). Bump `MENU_LENGTH` in `config.h`.
- **Menu fits 9 rows at `MENU_HEIGHT 20`** (baseline of row 9 = 180). At 22 px
  the 9th row runs off the 200 px screen. `showWifiList` shares the constant.
- **Alarm («Будильник», `setAlarm`/`alarmDue`/`ringAlarm`):** checked in
  `init()` on **any** wake (RTC tick EXT0 and button EXT1 — a button press
  inside the window rings instead of reaching `handleButtonPress`). Fires in
  the window `[target, target + ALARM_LATE_MIN]` because the RTC tick can
  arrive late; re-firing inside the window is blocked by `alarmHandled`
  (RTC_DATA), reset when the window passes or on settings save. `snoozeAt`
  lives only in RTC_DATA (reset on reboot and on save). `ringAlarm` is a
  blocking loop — motor 1 s on/1 s off, right button = snooze
  `ALARM_SNOOZE_MIN`, both right = off, cap `ALARM_RING_MIN` minutes
  (default 5, `.env`-configurable via `load_env.py`; `ALARM_RING_MAX_MS`
  derives from it) — the countdown restarts after each snooze.
  Dismiss/snooze is decided only on **release**: press one right button,
  then the second while still holding the first = off. The released state
  must persist `ALARM_BTN_RELEASE_MS` (50 ms) — without that debounce the
  press bounce ends the wait in ~10 ms and everything snoozes.
  In `setAlarm` MENU auto‑repeat counts steps by elapsed time
  (`ALARM_REPEAT_DELAY_MS`/`ALARM_REPEAT_MS`), not per loop iteration, because
  a partial refresh (~300–500 ms) is slower than the 200 ms repeat period.
- **App screens (button loops):** `pinMode(..., INPUT)` all four, then **wait for
  the opening MENU press to be released** before polling, otherwise it is read
  again as "select/save" on the first iteration. Pattern: `setTimezone`,
  `showWifiList`, `showFaces`. Wait‑for‑press, then wait‑for‑release per button.
- **i18n:** UI strings live in `lib/Watchy/lang/*.json` (`en.json` is the key
  reference and defines order); `lib/Watchy/tools/gen_lang.py` (pre-script)
  reads `LANG` from `.env` and writes `include/lang_strings.h`
  (`S_<KEY>` enum + `STR[]`), only when content changes. Build fails if a
  language's keys don't match `en`. Print strings via `printL`/`printlnL`
  (accept `StrId` or `const char*`), font via `LANG_FONT`. Charset per
  language via `_charset`: `cyrillic` → `Cyrillic9pt7b` (custom byte map
  0x80–0xC1), `latin1` → `Latin9pt7b` (ISO‑8859‑1, FreeSans 9pt). A language
  needing other glyphs needs a new font + a branch in `langCpToByte`.
  A 9 pt line fits ≈18–20 chars on 200 px — keep translations short.
  `wday_*`/`month_*` must stay consecutive (`S_WDAY_1 + wday - 1`).
  `display.print("Привет")`-style direct non-ASCII output prints garbage —
  always go through `printL`.
- **Timezone:** `gmtOffset` is user‑set (menu «Часовой пояс», NVS) and must not be
  overwritten by the OpenWeatherMap `timezone` field. Default is 0 in
  `settings.h`, personal value via `GMT_OFFSET_SEC` in `.env`. Weather
  `CITY_ID` comes from `.env` (settings.h default is New York 5128581).
  - The RTC holds **local** time, not UTC. Saving a new offset in `setTimezone`
    therefore shifts the RTC by `(new − old)` so the face is right immediately;
    NTP later re‑syncs with the same offset. Any new code that changes
    `gmtOffset` must do the same shift or the clock jumps at the next sync.
  - Offset editor: ±30 min steps, wraps at −12:00 / +14:00, UP = earlier,
    DOWN = later (mirrors `setTime` button polarity).
- **Wi‑Fi:** `WIFI_SSID/WIFI_PASSWD` come from `.env` (via `load_env.py`)
  and only seed the NVS list on first
  boot; afterwards the list (≤5, ordered) is edited on the watch.
  `connectWiFi()` scans first and treats a network as usable only if DNS for
  `ntpServer` resolves.
  - Seeding happens only when the `wifiCount` key is **absent**. Deleting every
    network leaves `wifiCount = 0`, so the build‑flag network does not come back;
    to re‑seed, erase NVS (`pio run -t erase`) or remove the key.
  - Hidden SSIDs are never tried: the scan filter skips anything not in
    `WiFi.scanNetworks()` results.
  - `WiFi.waitForConnectResult()` default timeout is **60 s** in arduino‑esp32
    2.x — always pass an explicit timeout (10000). `status=6` in the log is
    `WL_DISCONNECTED` after that timeout (AP out of range / wrong password),
    not a firmware fault.
  - After the WiFiManager portal succeeds, the password is available as
    `WiFi.psk()` — that's how «Настроить AP» overwrites a slot.
  - `WiFi.persistent(false)` before `begin()` so each sync does not rewrite the
    core's own Wi‑Fi NVS blob (flash wear); our list is the only source of truth.
- **Wi‑Fi list screen (`showWifiList`):** long BACK (≥ `WIFI_LONGPRESS_MS` 700)
  enters reorder mode (needs ≥2 networks, selection on a network, not «Добавить»);
  in reorder mode UP/DOWN swap, BACK saves, MENU restores the backup copy.
  Long‑press detection: measure hold time *while waiting for release*, then
  dispatch short/long — do not act on the press edge.
- Blocking button screens must `continue` after each handled press so the row
  bounds are re‑clamped (`sel >= rows`) — the list shrinks after «Удалить».

## Tooling

- `~/.platformio/penv/bin/python` does not exist on this machine. For esptool use
  `pio pkg exec -p tool-esptoolpy -- esptool.py --port <port> flash_id`
  (reports `ESP32-PICO-D4 (revision v1.1)`, `Detected flash size: 4MB`).
  `pio device monitor` refuses to run without a TTY on stdin (even in a PTY
  when stdout is redirected); for scripted serial capture use
  `/Users/igor/.platformio-venv/bin/python` — it has pyserial 3.5 installed.
  The watch only prints on a wake (RTC tick once/minute), so capture ≥75 s to
  be sure to catch a `BOOT` banner.
- Partition table `no_ota.csv` (framework default): nvs 20 K, **app0 2 MB**
  (0x10000–0x210000), spiffs 1.9 MB, coredump 64 K. The build's `Flash: 64%`
  is against that 2 MB app partition (2097152), so ~700 KB headroom — the
  "3MB app" comment in `platformio.ini` is wrong.

## Watch faces

- Base class: `drawWatchFace()` → `drawFace(faceIndex)`; app overrides
  `faceCount()` (10) and `drawFace(idx)`; 0 = digital 7‑SEG, 1–9 = analog
  (`Watchy_Analog.cpp`). Picker `showFaces()`: UP/DOWN cycle, MENU save, BACK cancel,
  full refresh per preview (partial refresh ghosts on dense dials).
- Face 0's `drawFace` calls `getWeatherData()`, which may bring Wi‑Fi up (slow)
  when the 30‑min interval has elapsed — also in the picker preview. Expected.
- **Analog geometry:** centre (100,100), `RADIUS 92` — not 98–100 — so the tick
  ring clears the corner icons (battery top‑right at x=178, Wi‑Fi top‑left).
  Hands: hour 0.7 R, minute 0.9 R, 3 px via a filled quad (two `fillTriangle`).
  - Half‑width offset must be `roundf(n)` with half‑width **1**, not 1.5:
    `roundf(1.5)` → 2 makes hands/ticks 5 px at cardinal angles.
  - Don't fake thickness with 3 parallel `drawLine`s — gaps appear on diagonals.
- **Adafruit GFX limits:** no bitmap scaling (`drawBitmapHalf` in
  `Watchy_Analog.cpp` does a 2:1 OR‑downsample from PROGMEM); bundled font sizes
  are only 9/12/18/24 pt plus the classic 5×7 (`setFont(NULL)`), so "1.5× smaller
  than 9 pt" means the 5×7 font — tried for the analog date and rejected (too
  small to read); the date stays `Cyrillic9pt7b` at (100,125). Each included
  Free* font costs a few KB flash.
- Centre text with `getTextBounds(txt,0,0,…)` then
  `setCursor(x - w/2 - x1, y - h/2 - y1)` — works for both GFX and classic fonts.
- Icon bitmaps live in `src/icons.h`: `battery` 37×21 (segments at +5,+5, 7×11,
  pitch 9), `wifi`/`wifioff` 26×18, `steps`, weather icons 48×32.
