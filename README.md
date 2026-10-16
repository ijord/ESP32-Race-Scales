# DIY ESP32 Race Scales

A fully wireless, low-cost corner-weight scale system built using ESP32 boards, HX711 load cell amplifiers, and a simple web interface.

Build your own race scales for approximately $80–$100 instead of spending $1000+ on commercial systems.

---

## Demo / Build Video

https://youtu.be/h9uxLq12wOU

---

## Features

- Identical firmware on every scale, with automatic gateway selection
- Wireless communication using ESP-NOW between scales
- Real-time corner weights (LF / RF / LR / RR)
- Mobile-friendly web UI (no app required)  
- Battery monitoring for each pad(coming soon - batteries lasts 8-10hrs so was not in a hurry to do this)
- Stability locking (auto-detect when weight settles)  
- Unit switching (lbs / kg)  
- Calibration and zeroing from browser  
- Smart filtering for stable readings  

---

## Hardware Required

### Per Pad (x4)
- ESP32 (Lite or Dev board)  
- HX711 amplifier  
- 12 load cells 
- Wiring and connectors(comes with hx711's)  
- Battery(optional)

Disclaimer: Some of the links below are affiliate links, which means I may earn a commission at no extra cost to you.

As an Amazon Associate I earn from qualifying purchases.

Esp32 lite(4 pack) = $17
https://www.amazon.com/Bluetooth-Development-Antenna-MicroPython-Arduino/dp/B0D7V9GKHV?pd_rd_w=qu7ib&content-id=amzn1.sym.a1c22caf-2d4a-4c58-b920-210b8f6b9234&pf_rd_p=a1c22caf-2d4a-4c58-b920-210b8f6b9234&pf_rd_r=1K3H3M4S7FN856PNXRZX&pd_rd_wg=qjO1W&pd_rd_r=3da1b217-4232-4600-9ed4-17c113e8f9a5&pd_rd_i=B0D7V9GKHV&psc=1&linkCode=ll2&tag=traviscea05-20&linkId=ddd1caaf149a3d7df813d882b684685d&language=en_US&ref_=as_li_ss_tl

Alternate link for esp32 lites if others are sold out(you need 4 esp32s):
https://www.amazon.com/V1-0-0-Bluetuth-Development-ESP-32-MicroPython/dp/B0FBGKK484?crid=17WSD3V9NQ413&dib=eyJ2IjoiMSJ9.0mMutUw27FQh3oBplnQp1ehm2_8dKdtx9ewAsDJyPX2TvjIiAv0tHRh221LBU5XGbP78VNZbeMEqlELbOma7iFtdZbbSTFPk0mZLl5Yqcgoe_Exm8BTwBnzIoaSUUZM4gp5d5-_vyUAqpObyvbEQcYg1qrUN0IIQcIhH8B1XkMQyrHryTqTa_Y1MONCmQ7wu_7TbilEwf21As29RzhMz1QMHID95MqGbWyS1HaGAPak.Z9JD3uAwjzWMCq5MB7yQeq_6o0ThsDc1C0nS0Tonipk&dib_tag=se&keywords=esp32+lite+4&qid=1777433267&sprefix=esp32+lite+%2Caps%2C191&sr=8-17&linkCode=ll2&tag=traviscea05-20&linkId=6603be8a29e5e0e344c7640111766d7e&language=en_US&ref_=as_li_ss_tl

HX711(4 pack) = $7 
https://www.amazon.com/dp/B0BLND4VF6?social_share=cm_sw_r_cp_ud_dp_XQKKQSD12G3YQSVKNPX8&linkCode=ll2&tag=traviscea05-20&linkId=eac704287f110a03faf7c303873c94fd&language=en_US&ref_=as_li_ss_tl

Battery Pack(4 pack) = $17 
https://www.amazon.com/MakerHawk-Rechargeable-Protection-Connector-Electronic/dp/B0D9K7HQHT?_encoding=UTF8&pd_rd_w=EenFZ&content-id=amzn1.sym.a9c4acee-9ca0-46be-bae3-532a2b4b0d29%3Aamzn1.symc.5a16118f-86f0-44cd-8e3e-6c5f82df43d0&pf_rd_p=a9c4acee-9ca0-46be-bae3-532a2b4b0d29&pf_rd_r=J8X3P7FS184DMQ0HEJRD&pd_rd_wg=f3ZF4&pd_rd_r=d263d5cc-fec2-4817-bc85-5528fa1dd645&th=1&linkCode=ll2&tag=traviscea05-20&linkId=37cb3096cedf42353d0263f6f533e836&language=en_US&ref_=as_li_ss_tl

JST battery connectors(for more length) = 6$
https://www.amazon.com/dp/B0FW4Y4FSL?&linkCode=ll2&tag=traviscea05-20&linkId=2dc5248c0eb4ad13a03cd092e03af09c&language=en_US&ref_=as_li_ss_tl

Load cells(48, buy 3 sets of 20) = $45 
https://www.amazon.com/dp/B09KGSGL18?social_share=cm_sw_r_cp_ud_dp_BH1Y8XC925MPHF1E74TW&linkCode=ll2&tag=traviscea05-20&linkId=46be772bed19824dc515417869be07c8&language=en_US&ref_=as_li_ss_tl

Alternative Link for Load cells if others sell out(50 pack) = 35$
https://www.amazon.com/dp/B0FMNHCGMB?social_share=cm_sw_r_cso_cp_apin_dp_ZTAPC0312YVC65RHH3ZX&linkCode=ll2&tag=traviscea05-20&linkId=66c7a8294df63c4d4caa6ed5f871bc77&language=en_US&ref_=as_li_ss_tl

Wood 2x12x8 = $20 
***I used 18in sections here for my 245 tires. You may want more or less distance depending on your tire width.


## 3d Print files
PETG filament I used: [Amazon link](https://www.amazon.com/OVERTURE-Filament-Consumables-Dimensional-Accuracy/dp/B07PGYHYV8?adgrpid=191564566452&dib=eyJ2IjoiMSJ9.uV75SqbTLCgAy78vKWUZ0VUSWFtXi-5DzJINPhuQ9V8zrYwjYe66q0YGr8sG1nKs_rOAuqN59tx6vZ4L2lL_fWAbo0rkwGZ-Y7fKun_2db5yr8mI7cER3wtzvZrGoTCWxbB5r1Yics1vA5P2UVPJt1qCaX9FMctx4sXoK_W5GvG-Gf7wX5gsMw5-k1r5VXXaxdwAc9fhtHGWi4gdZoppUZdZbnXRd2UimD1gf57p_nyHo_3RnHGDTfJXPCMz0ZKtY-bOKHTBhSEb3nVChVI1alxOgWRryRfuJ16npHx-8OI.Pm-9rvDukxqjzlyPPor2StnBqBybCGUJVkUTXMVSx7Q&dib_tag=se&hvadid=779547513984&hvdev=c&hvexpln=0&hvlocphy=9194515&hvnetw=g&hvocijid=10685258310100459603--&hvqmt=e&hvrand=10685258310100459603&hvtargid=kwd-307270553510&hydadcr=8497_13833035_2116124&keywords=petg%2Bblack&mcid=471a93842d0535b09ebc346ed5f9d949&qid=1777687529&s=industrial&sr=1-4&th=1&linkCode=ll2&tag=traviscea05-20&linkId=5f6861a79bd44ac9079275ba2786f080&language=en_US&ref_=as_li_ss_tl)

Load cell holder:
<br>
Stl file: https://drive.google.com/file/d/1BfqhXBpTUUBheZa0adTtnxa-Y_LcbjTP/view?usp=sharing


Box for Esp32:
<br>
Stl file: https://drive.google.com/file/d/1Z9YkdcgCBxQTEjgYmJWulF77Iv5ffJcg/view?usp=sharing


## Wiring
Make sure to check the polarity on the battery/esp32 connection. Some esp32s would be backwards on the jst connection from the battery.  I had to swap mine to ensure it didn't blow the board.  The board will have the positive side labeled and that should be the side that gets the red wire from the battery.

<br>

Make the wheatstone bridges as I have described in the video above(black to black, white to white). 
Then wire the all the red wires from each corner as shown below

```
PAD layout:
______________
| A        C |
|            |
| B        D |
______________

PAD(Labeled above) → HX711
A → E+
D → E-
B → A-
C → A+

HX711 → ESP32
VCC → 3.3V
GND → GND
DT  → GPIO 12
SCK → GPIO 13
```

---


## Build and Upload with PlatformIO

Install PlatformIO IDE in VS Code (or PlatformIO Core) and open this repository.
**Flash the same firmware to every board.** No pad ID, role, channel, or MAC address
needs to be edited in source code. HX711, WiFiManager and WebSockets are installed automatically.

```sh
pio run -e S3-Mini
pio run -e S3-Mini -t upload
pio device monitor -e S3-Mini
```

`src/main.cpp` runs every scale. `src/dashboard.h` contains the dashboard.
`include/scale_network.h` contains the portable election policy and packet format.
The old parent/child build environments have been replaced by one `S3-Mini` build.

If uploading fails, hold BOOT, press/release RESET, release BOOT, then retry.
Set `upload_port` and `monitor_port` in `platformio.ini` if necessary.

### Firmware updates over Wi-Fi (OTA)

Choose **Scale settings → Update this scale's firmware** from the dashboard,
or open `http://<scale-IP>/update` in a normal browser. Build the firmware
with `pio run -e S3-Mini`, select **`.pio/build/S3-Mini/firmware.bin`**, then press
**Update**. The page reports success/failure and the scale reboots after success.
Use the application `firmware.bin` for this ESP32-S3 build, rather than a
bootloader, partition table or merged flash image.

Each individual upload updates only the selected board. The **Firmware update** page
updates the gateway; **Scale settings → Update this scale's firmware** selects a
particular member. Member uploads are streamed through the gateway to the member's
existing WiFiManager updater, so the browser can stay on home Wi-Fi or the kit
hotspot. The gateway checks the member's device ID before forwarding the image and
continues broadcasting heartbeats during transfer. The member must have joined
the kit Wi-Fi; an ESP-NOW-only connection has no HTTP updater address.

**Scale settings → Update all modules** accepts one file and uploads it to each
eligible member in turn, then the gateway last. Firmware 1.2.7 skips Scale-only
modules and lists them as skipped. The page checks that all eligible scales are
online and advertise a Wi-Fi address before starting.
Each member's reboot is confirmed by a changed boot ID before proceeding; progress,
reported version and failures are shown on the page. A failed transfer or unconfirmed
reboot stops the sequence before further modules are flashed. Keep the page open.
The same file is reused for every upload; this is sequential HTTP OTA, not multicast.

Scale-only modules show an update-unavailable message in place of their update
link. Select Auto or Prefer gateway and save settings to include one in updates.
The page has no firmware-update link in its top navigation; use the individual
scale links or the kit updater below them. A Wi-Fi connection is still required.
The relay verifies the member's HTTP device ID before forwarding firmware rather
than rejecting it because live readings expired while HTTP parsing blocked the loop.
Busy-command and gateway-restart failures have separate messages identifying the cause.
Members explicitly retry joining the current gateway SSID every ten seconds.
A Scale-only module still needs a gateway to join; power-cycling it can retry the
initial association. If it cannot obtain a Wi-Fi IP, use USB to update it first.

Install this build on the gateway first through its `/update` page. Then use the
new member links to update the other scales. Negative readings and firmware version
reporting require updating each scale. With an older gateway, direct member update
links still require connecting the browser to the kit hotspot.
Alternatively, power one Auto/Prefer-gateway scale at a
time and update it through its own hotspot or saved home Wi-Fi. A gateway change
after reboot may require reconnecting to the kit or using its new IP address.

The existing 4 MB flash layout has two 1.25 MiB OTA application slots. WiFiManager
uses the ESP32 Update library to write and validate the inactive slot before
selecting it for reboot. Stored calibration, corner, kit and Wi-Fi settings are
retained. Diagnostics shows the firmware build time and current application slot.
The firmware version is defined in `include/firmware_version.h` (currently
`1.2.20`) and appears in live data, the scale list, settings and diagnostics.
Beacons/readings carry it in a field unused by older reading packets, preserving
the 64-byte radio format and protocol version. Older boards display `unknown`
until updated. Firmware version and radio protocol version are separate values.
See [Espressif's OTA process](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/system/ota.html).

Hardware verification rejected an invalid image without changing the running
slot, then uploaded the application over Wi-Fi and booted from `app0` into `app1`.
HTTP/WebSocket service returned and calibration, corner, kit, 80 MHz CPU speed
and HX711 pins were retained.

Weighing/live updates pause during transfer. Run updates one scale at a time with
stable power. The local HTTP updater uses WiFiManager's existing network access
and has no separate update login. File selection may not work inside the phone's
captive-portal popup; open the update URL in a full browser instead.

### ESP32-S3 Super Mini settings

The build pins Espressif32 6.12.0 (Arduino-ESP32 2.0.17), uses native USB serial,
and a 4 MB flash layout. PSRAM is not required. The connected Super Mini has
4 MB flash and 2 MB PSRAM. Other boards must have at least 4 MB flash.
HX711 DT is **GPIO 12**, SCK is **GPIO 13**, and BOOT is **GPIO 0**.
Battery monitoring uses **GPIO3** and the onboard 2:1 divider.

### Battery operation

The CPU runs at **80 MHz**, selected by `board_build.f_cpu = 80000000L` in
`platformio.ini`. The diagnostics page reports the actual running CPU clock.
120 MHz is rejected by the pinned Arduino core; 160 MHz is the next supported
speed above it.
The ten-sample average, 250 ms ESP-NOW transmit interval, and 200 ms WebSocket
update interval are unchanged. Set that configuration value to `240000000L`
and rebuild/upload to compare operation at full CPU speed.

Radio transmit power requests **8.5 dBm** when either the Wi-Fi station or AP
starts. The limit applies to Wi-Fi and ESP-NOW on the shared radio.
Firmware 1.2.19 leaves this setting in place for the user's battery test.
Firmware 1.2.20 retains 8.5 dBm and scans all channels when connecting, ranking
matching home-network APs by signal strength rather than stopping at the first
SSID match. This selects an AP on connection; it does not add continuous roaming.
Firmware 1.2.18 at 17 dBm connected to home Wi-Fi on USB power, but the user
reported that it did not work on battery power.
Firmware 1.2.16 previously restored 12 dBm after
1.2.15 at 20 dBm showed failed discovery scans and home-network authentication
timeouts on USB-connected scale A0F262F4BA28. The open hotspot is retained.
At 12 dBm, that scale rejoined its saved home Wi-Fi at 192.168.10.215, completed
a discovery scan, and its open hotspot appeared in a fresh Windows Wi-Fi scan.
Its saved calibration and offset were verified unchanged. This does not measure
maximum range or validate all four scales together.
A subsequent 1.2.17 test requested and reported 18.5 dBm. Discovery scans completed,
but initial home Wi-Fi authentication timed out and the hotspot was absent from a
fresh Windows scan. The user reported a blue LED around the restore to 12 dBm;
whether 18.5 dBm eventually connected was not established.
Actual output remains subject to PHY, data-rate and country limits.
Serial diagnostics shows the driver readback; radiated output has not been measured.
This combination of CPU speed and power setting still needs hardware validation.

Earlier testing on the Super Mini at 240 MHz found that the default 20 dBm
produced authentication timeouts and failed
ESP-NOW delivery acknowledgements even with scales 1–2 metres apart. At 8.5 dBm,
the board joined the gateway hotspot and both scales exchanged readings with
mDNS still enabled. Firmware 1.2.15 restored maximum power at the user's request;
the reason for those earlier failures and current maximum usable range are unverified.

Earlier 80 MHz gateway validation with one HX711 and four simultaneous live views over 40 seconds
kept the median update interval at 201 ms and sampled the ADC at 11.93 Hz, compared
with 11.91 Hz in the 240 MHz baseline. The running clock, saved corner assignment
and calibration were checked on the device. This is not a four-scale radio-load
test or a battery-current measurement.

80 MHz is supported with the radio's APB clock still at 80 MHz. Lower CPU speed
reduces CPU power demand, but radio, board LEDs, regulator losses and load-cell
bridge excitation also contribute to battery draw. Datasheet modem-sleep current
figures are not a measurement of this complete scale with its radio active.
See [Espressif power management](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/system/power_management.html)
and the [ESP32-S3 datasheet](https://documentation.espressif.com/esp32_s3_datasheet_en.pdf).

Wi-Fi/ESP-NOW receive sleep remains disabled: modem sleep can delay Wi-Fi
reception, so it is not a guaranteed performance-neutral saving. Deep sleep or
powering down the HX711 would also interrupt continuous weighing. The pinned
Arduino framework has power-management/tickless-idle support disabled in its
precompiled SDK, so automatic light sleep needs a framework change rather than
just an application flag. See [Espressif Wi-Fi power saving](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-guides/wifi.html#wi-fi-power-saving-mode).

Battery-current savings have not been measured. Compare average current at the
battery input at 80 and 240 MHz with the same Wi-Fi connection, sensors and
number of browser clients; include idle hosting and active weighing in the test.

## Setup: one to four identical scales

1. Flash `S3-Mini` onto every board. Power one first and wait approximately
   6–10 seconds. It searches for an existing gateway, then hosts the kit if none
   is found. A single scale works on its own.
2. Join its open hotspot, named `<kit>-<last four device-ID digits>`, for example
   `SCALE-BA28`; no password is required. The exact name is printed on serial.
3. Open http://192.168.4.1 to use the dashboard. Choose **Wi-Fi setup** at the
   bottom of the page to optionally join a 2.4 GHz router.
4. Power the other scales. They discover the gateway automatically and appear
   in its dashboard. All new boards use the stored kit code `SCALE` by default.
5. Open **Scale settings** and assign each device LF, RF, LR, or RR. Device IDs
   are printed on serial; powering one device at a time also identifies it.
   Settings are saved on the selected device and that device restarts.
6. With the scales unloaded, press **ZERO**. Put a known weight on each scale,
   press **CAL**, and enter its corner or device ID and the known weight.
   With one online scale, leave the corner/ID blank to select it.

Firmware 1.2.15 uses an open kit hotspot. Update every module, including members;
older firmware still requires the former hotspot password and cannot associate
to the open AP. Saved home-network credentials are unchanged.
Firmware 1.2.20 shortens the hotspot name and moves the full station ID and
gateway preference into vendor metadata on Wi-Fi beacons/probe responses.
Update all scales to 1.2.20 for discovery of the renamed hotspot. Updated boards
still recognize older hotspot names and ESP-NOW beacons. ESP-NOW packets remain
64 bytes; beacon metadata version 2 identifies the short hotspot name. Static
Wi-Fi discovery metadata never adjusts the LED clock; live ESP-NOW beacons do.

The dashboard is at http://192.168.4.1/ or `http://<gateway-home-IP>/`.
The previous `/scales` URL remains an alias. Wi-Fi and scale settings links are
at the bottom of the dashboard, below the live scale table.
Bars under the corner labels match the physical scales: LF red, RF green,
LR white and RR black. An outline keeps the black bar visible on the dark page.
The gateway also advertises the kit name through mDNS: kit `SCALE` is available
at `http://scale.local/` (kit `RACE` uses `http://race.local/`).
The name follows the gateway role and uses the lowercase kit code; members do not
advertise that shared hostname. mDNS works on the same local network where the
client supports it; the numerical IP remains available.
The setup portal and serial output show the gateway's home IP. The hotspot stays
available after joining a router. Use the dashboard's **Wi-Fi setup** link to
change router credentials.

A single calibrated scale contributes to the total even without a corner
assignment. Corner percentages are withheld until all four corners have one
online, calibrated scale and the total is positive. Offline devices, missing
load-cell readings, uncalibrated devices, and duplicate corner assignments are
shown explicitly in the device table.
Unavailable corner weights show `--`, including missing HX711 readings and
remote scales whose telemetry has timed out. Connected scales reading zero still
show `0.0`. If the live gateway connection times out, all corner readings and
summary values show `--` until live data resumes.
Calibrated negative readings are displayed and included in totals, rather than
clamped to zero, so an incorrect tare remains visible. The existing half-pound
rounding and averaging are unchanged.
The live scale list marks the gateway's scale name with `(WiFi)` and shows each
scale's calibration gain (counts/lb) and zero offset in pounds, computed as stored
ADC offset divided by counts/lb. Zero offset stays in pounds when the weighing
display is switched to kg; it shows `—` without a valid calibration. These calibration
fields require firmware 1.2.0 on that scale; older readings display `—`. Update the
gateway to 1.2.0 to get the new table. Telemetry uses unused reading fields and a
metadata marker, keeping the 64-byte radio packet compatible with older firmware.

Firmware 1.2.8 reads only **GPIO3**, averaging eight calibrated ADC readings once
per second. First calculate **VBUS = 2 × GPIO3 voltage**. If VBUS is **4.5 V or
lower**, add the measured **0.88 V** correction; above 4.5 V, display VBUS without
the correction because USB bypasses the battery diode. Each corner shows **Battery: x.xx V** below
its color bar. The API field is `battery_voltage`; GPIO1 is no longer sampled.
Offline scales, duplicate corner assignments and older firmware without this
corrected GPIO3 reading show `Battery: --`. Update the gateway first, then each
scale to 1.2.8. On USB power, the displayed value is the USB supply voltage rather
than the cell's charging voltage; GPIO3 senses VBUS.
The dashboard also estimates usable battery percentage below the voltage, entirely
in the browser. A piecewise-linear approximation of a Li-ion discharge curve uses
these voltage/percentage points: **3.64/0, 3.80/20, 3.90/50, 4.00/75, 4.10/90,
4.20/100**. This is a heuristic for the scale's usable range, not a measured profile
of these cells or a fuel-gauge measurement. Values clamp to 0–100%, with `~` marking
the estimate. Missing/offline/duplicate scales show `--`; readings above 4.5 V show
`USB power` instead of a percentage. Only the gateway needs firmware 1.2.9 for this
display, provided members already report battery voltage. Radio packets are unchanged.
See [Adafruit's Li-ion/LiPo voltage profile](https://learn.adafruit.com/li-ion-and-lipoly-batteries/voltages)
for the general discharge shape; the 3.64 V usable cutoff is specific to this kit.
The ADC uses 11 dB attenuation (see
[Espressif ADC documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html)).

The Super Mini's RGB LED on GPIO48 repeats **100 ms network color, 900 ms off**
when no battery indication is active. With a battery indication, it repeats
**200 ms network color, 300 ms off, 200 ms battery color, 300 ms off**.
The network pulse is green for a member or
searching scale, blue for a gateway associated to a Wi-Fi network as a client,
and yellow for a gateway hosting its AP without a Wi-Fi client association.
The battery pulse is white when the reported voltage is above 4.5 V (USB/charging),
red below 3.80 V (under 20% on the usable-charge curve), and off otherwise. No
battery pulse is shown for 20–80% or above 80%; no intermediate color was specified.
Before the first voltage sample, the battery pulse is off. The battery state is
shared atomically with the software timer, so the LED keeps blinking independently
of HTTP uploads. GPIO48 matches the common Super Mini
[board example](https://github.com/hwthinker/ESP32S3-supermini#contoh-program-akses-led-ws2812-di-gpio48).
The main dashboard includes this LED guide immediately above the bottom settings links.
Firmware 1.2.12 synchronizes the start of each one-second LED cycle to the gateway's
ESP-NOW beacons. Each scale retains its own status/battery colors and flash length.
Beacons carry a phase stamped at transmission; members use the captured arrival
time so delayed receive-queue processing does not shift the cycle. A 10 ms timer
renders the absolute phase, with radio latency and timer scheduling limiting
alignment precision. On lost beacons, the LED keeps running at the last offset;
it re-aligns when a gateway is found. All boards, including the gateway, need
1.2.12 for synchronization. Older firmware still exchanges normal 64-byte readings.

## Live web updates

The dashboard and HX711 diagnostics receive a WebSocket snapshot every 200 ms
(five updates per second), replacing repeated HTTP polling. Each page keeps one
connection to port 81 on the same board and reconnects automatically if updates
stop. The connection status warns when displayed readings may be stale.
The ten-sample weighing average is unchanged, so new weight measurements still
depend on the HX711 sampling rate. Tare, calibration and settings use HTTP.

Gateway background Wi-Fi scans are deferred while a live view is connected to
avoid taking the radio away from its operating channel. Same-channel gateways
can also discover and merge through ESP-NOW beacons while live views are open.
Discovery across different channels waits until live views close. Startup discovery and member
failover still run normally. Using Wi-Fi setup can still interrupt updates while
the board scans or connects to a router.

For network diagnosis, connect USB and run `pio device monitor --port COM11`
(substitute the board's actual port). Serial output at 115200 baud includes
discovery scan results/duration, recognized scale SSIDs, role transitions and
Wi-Fi disconnect reason codes. A network summary every 10 seconds shows role
(`0` searching, `1` gateway, `2` member), Wi-Fi status (`3` connected), channel,
actual transmit-power limit, received/rejected messages and ESP-NOW send results.
`tx_error=ESP_OK` means the send was queued; `delivery=0` is a successful radio
acknowledgement and `delivery=1` is a failure. Neither alone confirms that a
remote tare/calibration command was applied; those use the separate command ACK.

## HX711 diagnostics

Open **HX711 diagnostics** from the dashboard or setup portal, or visit
`http://<board-IP>/diagnostics`. This page shows the HX711 on that physical board:
DT/SCK pins, DT level, recent conversion activity/rate, sample counts/ages, raw signed
ADC counts, the normal channel-A average, and stored tare/calibration values.
Conversion activity does not prove that the load cells are wired correctly.
DT has a weak pull-up so an unplugged input reports not-ready instead of floating.

Normal weighing samples **A at gain 128**. **Start A+B test** alternates A/128
and B/32, allowing 400 ms to settle after channel changes, as specified by the
[HX711 datasheet](https://cdn.sparkfun.com/assets/b/f/5/a/e/hx711F_EN.pdf). This pauses weighing
and tare/calibration on this board; other scales continue. Stop the test to
resume normal sampling. It also expires within 10 seconds without a page renewal.
Raw values remain visible with their age when stale; unused B inputs may float.
The test does not modify the stored offset or calibration. For another scale's
two channels, open its own diagnostics page on the kit network.

## Architecture and gateway selection

Every board continuously reads its own HX711, keeps its calibration locally,
and can host the dashboard. Gateway/member are temporary runtime roles.

- At startup, boards scan for a gateway advertising the same kit code. A later
  board joins an existing gateway even if its preference is **Prefer gateway**.
- Without a gateway, **Auto** and **Prefer gateway** boards can become one after
  discovery. **Scale only** boards keep searching.
- If simultaneous starts or a network partition produce two gateways, periodic
  scans when no live view is connected, or received ESP-NOW beacons on the current
  channel, converge on a preferred gateway, then the lowest device ID as a tie-break.
- Members join the gateway's Wi-Fi hotspot. This keeps their radio on its channel
  even when the gateway joins a router on another channel. ESP-NOW carries
  readings, heartbeats, and commands using automatically discovered STA MACs.
  Failed Wi-Fi associations retry every 10 seconds. ESP-NOW readings continue on
  the current radio channel during retries; they do not require an assigned IP.
- Members wait 20 seconds without a gateway heartbeat before searching again.
  A new gateway can emerge after another 6–8 seconds. This grace period tolerates
  WiFiManager connection attempts. Browser access may need a hotspot reconnect
  and refresh after a gateway change.
- Tare, calibration, and setting changes receive application-level ACKs. Commands
  retry for up to five seconds; duplicates within a gateway session reuse the
  cached result instead of repeating a tare or calibration. The UI reports
  failures instead of treating a radio send as successful application delivery.
- Radio callbacks only queue received packets and signal completed sends.
  Sampling, configuration writes, and commands run from the main loop. Normal
  sampling averages ten readings without blocking on a missing HX711.

### Stored settings and recovery

**Scale settings** stores these values on each board:

| Setting | Values / purpose |
| --- | --- |
| Kit code | 1–8 letters/numbers; default `SCALE`. All boards in a kit use the same code. |
| Corner | Unassigned, LF, RF, LR, RR; independent of gateway role. |
| Role preference | Auto, Prefer gateway, Scale only. |
| Calibration | Offset and counts per pound, attached to the physical load cells. |
| Home Wi-Fi | Configure through WiFiManager as gateway, or share a network with the kit from Scale settings. |

Corner IDs remain 1=LF, 2=RF, 3=LR, 4=RR. Existing assignments and calibrations
are preserved when upgrading from the previous labels. Dashboard/API field names
now use `lf`, `rf`, `lr`, `rr`; reload any open dashboard after upgrading.

To change a whole kit's code, update the other boards from the gateway's settings
page first, then update the gateway last. Changed boards temporarily host/search
for the new kit until the remaining boards join it. Use a distinct kit code when
another set of race scales is nearby. Kit codes identify groups; they are not
cryptographic pairing or authentication. The gateway admits up to four scales,
including itself.

Hold **BOOT for three seconds while running** to restore Auto mode, kit `SCALE`,
and an unassigned corner, then restart. Calibration and home Wi-Fi are retained.
This recovers a board configured as Scale only when no gateway is available.

### Sharing home Wi-Fi

On the gateway, open **Scale settings → Share Wi-Fi with all scales**. The default
uses the gateway's saved home-network SSID/password without sending the password
back to the browser. If none are available, enter an SSID and password; leave the
password blank for an open network. The page visits members first and the gateway
last, reports each result, and continues after individual failures. Scale-only
members are included. Every target needs firmware **1.2.11+** and a kit Wi-Fi IP;
ESP-NOW-only and offline scales are reported as failures, not silently skipped.

The gateway forwards URL-encoded credentials over the existing kit Wi-Fi HTTP
connection. The member accepts writes only from its current gateway IP and only
when the requested device ID matches its own. Its reply confirms that same ID.
SSID/password are committed together in a single NVS blob; prior saved settings
remain readable. Passwords are not returned in responses or printed in serial logs.
Saving does not reboot or disconnect scales. Members keep using the kit AP, and
the shared home network is used on their next gateway connection. A connected
gateway stays on its current network until reconnecting or restarting.

### Connection limits

A replacement gateway without saved credentials still offers its hotspot and
weighing dashboard. Gateway changes can also change the home IP.

This version uses a stored kit code rather than a pairing/enrollment window.
Secure enrollment and credential distribution over ESP-NOW are not implemented.
Wi-Fi scans and connection attempts can briefly interrupt radio delivery, and
saving Wi-Fi credentials can pause processing for the connection timeout.

### Upgrading existing boards

The unified ESP-NOW packet format is incompatible with the old parent/child
firmware: **update every board in the kit**. A valid old parent LF calibration
and offset are migrated on first boot. Old remotely calculated corner calibration
values cannot be mapped reliably to device identities; recalibrate those physical
scales after upgrading. Legacy preference namespaces are left intact.

## Verification

Firmware build:

```sh
pio run -e S3-Mini
```

Run the portable C++ tests with a local `g++` compiler:

```sh
python tests/run_tests.py
```

On Windows, the runner also finds PlatformIO's `platformio/toolchain-gccmingw32`
package, installable with `pio pkg install -g -t platformio/toolchain-gccmingw32`.
Tests exercise single-scale hosting, joining an existing gateway, simultaneous
hosts, preferred gateways, heartbeat loss, Scale only mode, timer/sequence
rollover, malformed packets, signed readings, compatible firmware-version fields,
and duplicate command handling. Relay tests compile the production handlers with
controlled transports to check target identity, multipart framing, partial writes,
invalid/incomplete files, remote failures and cancellation.
LED tests exercise pulse duration, repeat interval and gateway/member colors.
Clock tests cover different boot times, delayed receive processing, lost beacons,
gateway changes and timer rollover.
Wi-Fi sharing tests cover saved/manual credentials, validation, atomic storage,
legacy migration, matching device identity, Scale-only members and partial failures.

With Node.js installed, test browser reconnect and stale-update handling:

```sh
node tests/live_updates_test.mjs
node tests/dashboard_test.mjs
node tests/kit_update_test.mjs
node tests/kit_wifi_test.mjs
```

Hardware acceptance checks still require multiple boards: boot all four together,
assign/calibrate corners, change the router channel, remove the gateway, verify
that another hosts the kit, and check tare/calibration persistence after reboot.

---

## Important Notes

## Stability Tips

- Let scales settle before zeroing
- Use solid ground wiring
- Add capacitors to HX711 (0.1µF + 10µF)
- Avoid long signal wires

---

## Known Issues / Troubleshooting

### Jumping Values

**Check:**
- HX711 wiring
- Grounding
- Offset correctness
- Packet validation

### Watchdog Resets (ESP32)

Add:

```cpp
delay(1);
WiFi.setSleep(false);
```


## Example Use Cases

- Track day corner balancing
- Suspension tuning
- Weight distribution analysis
- DIY race car setup

---

## Inspiration

This project was created as an affordable, open-source alternative to professional race scales, which typically cost $1000 or more.

---

## Contributing

Contributions are welcome, including:

- UI improvements
- Filtering algorithms
- BLE or logging features
- Hardware variations

---

## License

This project is source-available and free for personal and educational use. Commercial use is NOT permitted.

---

## Support

If this project helped you:

- Star the repository
- Share your build
- Contribute improvements
