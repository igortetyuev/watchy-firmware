# Reads KEY=VALUE pairs from .env (next to platformio.ini) and passes them to
# the compiler as -D flags. Strings are quoted, integers passed as-is.
import os
Import("env")

STRING_KEYS = {"WIFI_SSID", "WIFI_PASSWD", "OPENWEATHERMAP_APIKEY",
               "CITY_ID", "TEMP_LANG", "TEMP_UNIT"}
INT_KEYS = {"GMT_OFFSET_SEC", "VIBRATE_OCLOCK", "ALARM_RING_MIN",
            "NIGHT_WAKE_MIN", "NIGHT_NO_WIRELESS", "WRIST_REFRESH",
            "WRIST_REFRESH_MIN_S"}
# "HH:MM" (or plain minutes) -> minutes-of-day macro
TIME_KEYS = {"NIGHT_START": "NIGHT_START_MIN", "NIGHT_END": "NIGHT_END_MIN"}

path = os.path.join(env.subst("$PROJECT_DIR"), ".env")
if os.path.isfile(path):
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = (s.strip() for s in line.split("=", 1))
            if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
                value = value[1:-1]
            if key in STRING_KEYS:
                env.Append(CPPDEFINES=[(key, env.StringifyMacro(value))])
            elif key in TIME_KEYS:
                if ":" in value:
                    h, m = value.split(":", 1)
                    value = int(h) * 60 + int(m)
                env.Append(CPPDEFINES=[(TIME_KEYS[key], int(value))])
            elif key in INT_KEYS:
                env.Append(CPPDEFINES=[(key, int(value))])
else:
    print("load_env.py: no .env found, building with defaults from settings.h")
