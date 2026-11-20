# Solar Generator (Milwaukee PACKOUT XL)

A 12 V power box I built into a Milwaukee PACKOUT XL Tool Box, plus the firmware for its controller.

| Folder | What |
|---|---|
| `box/` | The box itself: parts to print (STL), the build book PDF, wiring guide and parts list. Start with [box/README.md](box/README.md) |
| `firmware/` | PlatformIO: `brain` (ESP32-S3 in the controller case) and `display` (ELECROW 4.3 in screen) |
| `shared/` | The link protocol and test vectors the firmware uses |

The box pairs with my iPhone app, **Off-Grid HQ**. It isn't in the App Store yet.

## Load the firmware onto the boards

Each green **Firmware** run on GitHub has two downloads, `brain-<number>` and `display-<number>`, each with `bootloader.bin`, `partitions.bin` and `firmware.bin`.

1. Install the flashing tool once: `pip install esptool` (Python is already on your PC).
2. Plug the board into the PC with a USB-C cable and find its COM port in Device Manager → Ports.
3. Unzip the download and, in that folder, run (replace `COM5` with your port):

```powershell
python -m esptool --chip esp32s3 --port COM5 --baud 921600 write_flash 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

Do it once with the `brain-…` files on the board for the case, and once with the `display-…` files on the ELECROW screen.
