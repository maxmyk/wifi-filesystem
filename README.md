# wifi-filesystem
Continuation of the educational project for the Operating Systems course. The project aims to create a Wi-Fi module which emulates a file system.

Any USB-enabled machine can mount the ESP32-S2 mini as a regular USB drive and read/write files on it.

The host machine serves the image file to the ESP32-S2 mini over Wi-Fi.

The current version achieves read speeds of **~250 kB/s** and write speeds of **~50 kB/s**. Speeds are at [USB 1.0 MEDIUM SPEED](https://fl.hw.cz/docs/usb/usb10doc.pdf) level, good for "Phone, Audio, Compressed Video". Currently, speed improvements are at the designing and planning stage.

## How to run
### ESP part
- As with every ESP32 project with the Arduino core, install https://github.com/espressif/arduino-esp32 via the boards manager. Arduino core lowers the entry point for most people. Arduino == easy(ier).

- Open [USBMSCESP32-S2mini.ino](USBMSCESP32-S2mini/USBMSCESP32-S2mini.ino) using Arduino IDE. Change the `server_ip` to your host machine.

- Change the `WIFI_SSID` and `WIFI_PASSWORD` to yours inside [wifi_secret.h](USBMSCESP32-S2mini/wifi_secret.h).

- Select LOLIN32-S2 mini or your board if you use a different one.

- Upload the code. _For ESP32-S2 mini - hold BOOT (0), press&release RESET (RST), then release BOOT_

### Host part
- compile using CMake.
```bash
cmake -B build
cmake --build build
```
- Run: `./build/wifi_usb_drive_server path/to/your/image.img`

## Important
You need a FAT32 image. The `.img` image used was created using the disks utility. Runtime image emulation is planned.

---

Original Authors (team): <a href="https://github.com/fazhur">Журба Федір</a>, <a href="https://github.com/Bohdan213">Пелех Богдан</a>, <a href="https://github.com/maxmyk">Михасюта Максим</a></mark><br>
Maintainer: <a href="https://github.com/maxmyk">Maksym Mykhasyuta (Михасюта Максим)</a></mark><br>