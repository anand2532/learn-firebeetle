# learn-firebeetle

PlatformIO project for **DFRobot FireBeetle ESP32** (`board = firebeetle32`) using the Arduino framework.

This firmware drives the built-in LED (`LED_BUILTIN`, GPIO2) with a smooth "breathing + heartbeat" animation.

## 1) Prerequisites

- Linux, macOS, or Windows
- USB data cable connected to FireBeetle ESP32
- Python 3.8+ (recommended)
- PlatformIO Core (CLI) installed

Install PlatformIO Core:

```bash
python3 -m pip install --user -U platformio
```

Verify installation:

```bash
pio --version
```

If `pio` is not found, add your user scripts directory to `PATH` (commonly `~/.local/bin` on Linux).

## 2) Project Configuration

Board and framework are defined in `platformio.ini`:

```ini
[env:firebeetle32]
platform = espressif32
board = firebeetle32
framework = arduino
```

## 3) Build Firmware

From the project root:

```bash
pio run
```

What this does:

- Resolves toolchain and framework packages
- Compiles code in `src/`
- Links firmware
- Produces artifacts in `.pio/build/firebeetle32/`

Important output files:

- `.pio/build/firebeetle32/firmware.bin` (flash image)
- `.pio/build/firebeetle32/firmware.elf` (ELF for debug/symbols)

## 4) Flash (Upload) to FireBeetle ESP32

### Auto-detect serial port and upload

```bash
pio run -t upload
```

### Upload with explicit port

If auto-detect fails, list serial devices:

```bash
pio device list
```

Then upload with a selected port:

```bash
pio run -t upload --upload-port /dev/ttyUSB0
```

Common Linux port names:

- `/dev/ttyUSB0`
- `/dev/ttyUSB1`
- `/dev/ttyACM0`

## 5) Monitor Serial Output

Open serial monitor:

```bash
pio device monitor
```

Or specify port and baud rate:

```bash
pio device monitor -p /dev/ttyUSB0 -b 115200
```

Useful monitor controls:

- `Ctrl+C`: exit monitor
- `Ctrl+T`, then `Ctrl+R`: reset board from monitor
- `Ctrl+T`, then `Ctrl+H`: show monitor help

## 6) Upload and Monitor in One Command

Very convenient for development:

```bash
pio run -t upload && pio device monitor -b 115200
```

With explicit port:

```bash
pio run -t upload --upload-port /dev/ttyUSB0 && pio device monitor -p /dev/ttyUSB0 -b 115200
```

## 7) Clean Build

If you suspect stale artifacts:

```bash
pio run -t clean
pio run
```

## 8) Troubleshooting

### Permission denied on serial port (Linux)

Add your user to dialout group and re-login:

```bash
sudo usermod -a -G dialout $USER
```

### Board not detected

- Confirm you are using a **data** USB cable (not charge-only)
- Try a different USB cable/port
- Press RESET before upload

### Upload times out / "Connecting..." issues

- Press and hold BOOT, start upload, release BOOT when flashing begins
- Try a lower upload speed by adding this to `platformio.ini`:

```ini
[env:firebeetle32]
upload_speed = 460800
```

### Serial monitor shows garbage text

- Wrong baud rate. Use the same baud in code and monitor (typically `115200`)
- Close any other app using the same serial port

## 9) Recommended Development Loop

1. Edit code in `src/main.cpp`
2. Build:
   ```bash
   pio run
   ```
3. Upload:
   ```bash
   pio run -t upload
   ```
4. Monitor:
   ```bash
   pio device monitor -b 115200
   ```

## 10) VS Code / Cursor (Optional)

If you use PlatformIO IDE extension, these tasks map to buttons:

- Build
- Upload
- Monitor

CLI commands in this README remain the same and are often faster for repeat workflows.




scp -i ./dev_key.pem -r ./current-sensor-data ubuntu@15.207.18.221:~/
ssh -i ./dev_key.pem ubuntu@15.207.18.221
sudo cp /home/ubuntu/current-sensor-data/current-sensor-data.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable current-sensor-data.service
sudo systemctl start current-sensor-data.service