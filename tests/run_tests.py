"""Run the firmware's portable C++ tests using g++ (no Arduino hardware required)."""
import os
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
core = Path(os.environ.get("PLATFORMIO_CORE_DIR", Path.home() / ".platformio"))
compiler = shutil.which("g++")
if not compiler:
    compiler = core / "packages/toolchain-gccmingw32/bin/g++.exe"
if not Path(compiler).exists():
    raise SystemExit("Install g++, or run: pio pkg install -g -t platformio/toolchain-gccmingw32")
output = root / ".pio/host-tests"
output.mkdir(parents=True, exist_ok=True)
env = dict(os.environ)
env["PATH"] = str(Path(compiler).parent) + os.pathsep + env.get("PATH", "")
source = (root / "src/main.cpp").read_text(encoding="utf-8")
relay = source.split("struct MemberUpload {", 1)[1].split("void handleDiagnosticsMode()", 1)[0]
(output / "member_update_functions.h").write_text("struct MemberUpload {" + relay, encoding="utf-8")
led = source.split("void updateStatusLed(TimerHandle_t)", 1)[1].split("void loadConfiguration()", 1)[0]
(output / "status_led_function.h").write_text("void updateStatusLed(TimerHandle_t)" + led, encoding="utf-8")
battery = source.split("void updateBatteryVoltage()", 1)[1].split("void updateLocalReading()", 1)[0]
(output / "battery_voltage_function.h").write_text("void updateBatteryVoltage()" + battery, encoding="utf-8")
helpers = "bool makeHomeCredentials(" + source.split("bool makeHomeCredentials(", 1)[1].split("void updateBatteryVoltage()", 1)[0]
handlers = "String encodeWifiField(" + source.split("String encodeWifiField(", 1)[1].split("void handleSettings()", 1)[0]
(output / "wifi_credentials_functions.h").write_text(helpers + handlers, encoding="utf-8")
for source_name, name in (("network_tests", "network_tests"), ("member_update_test", "relay_tests"),
                          ("status_led_test", "status_led_test"), ("battery_voltage_test", "battery_voltage_test"),
                          ("wifi_credentials_test", "wifi_credentials_test"), ("led_clock_test", "led_clock_test")):
    binary = output / (name + (".exe" if os.name == "nt" else ""))
    subprocess.run([str(compiler), "-std=c++11", "-Wall", "-Wextra", "-Werror",
                    "-I", str(root / "include"), "-I", str(output),
                    str(root / ("tests/" + source_name + ".cpp")), "-o", str(binary)], check=True, env=env)
    subprocess.run([str(binary)], check=True, env=env)
