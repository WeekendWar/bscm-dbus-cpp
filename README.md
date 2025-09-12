# bscm-dbus-cpp

C++ application to use BlueZ over D-Bus to scan for Bluetooth devices, connect to them, and manage GATT characteristics with notifications.

## Features

- Scan for Bluetooth devices using BlueZ over D-Bus
- Filter devices by desired service UUIDs
- Connect/disconnect to/from Bluetooth devices
- Discover GATT characteristics
- Enable/disable notifications on characteristics
- Read from and write to characteristics
- Real-time notification processing with terminal output

## Requirements

- Linux system with BlueZ installed
- D-Bus development libraries (`libdbus-1-dev`)
- CMake (3.10 or higher)
- C++17 compatible compiler

## Building

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install libdbus-1-dev build-essential cmake

# Build the application
mkdir build
cd build
cmake ..
make
```

## Usage

Run the application as root or with appropriate D-Bus permissions for BlueZ:

```bash
sudo ./bscm-bluetooth-manager
```

### Main Menu Options

1. **Scan for devices** - Discover nearby Bluetooth devices
2. **Set service filter** - Filter devices by service UUIDs (optional)
3. **List devices with desired services** - Show filtered devices
4. **Connect to device** - Establish connection to a selected device
5. **Disconnect from device** - Disconnect from a connected device
6. **Manage characteristics** - Work with GATT characteristics
7. **Process notifications** - Listen for notifications for 10 seconds

### Service Filtering

You can filter devices by service UUIDs. Enter UUIDs comma-separated:
- Example: `0000180f-0000-1000-8000-00805f9b34fb, 0000180a-0000-1000-8000-00805f9b34fb`
- Enter `none` to clear the filter and show all devices

### Characteristic Management

Once connected to a device, you can:
- Enable/disable notifications on characteristics
- Read characteristic values
- Write data to characteristics (in hex format)
- View characteristic UUIDs and properties

### Notification Output

When notifications are enabled and received, they are displayed in the terminal with:
- The characteristic path that sent the notification
- Raw data in hexadecimal format
- ASCII representation (printable characters only)

## Example Session

1. Start the application and scan for devices
2. Set a service filter (e.g., Battery Service: `0000180f-0000-1000-8000-00805f9b34fb`)
3. List filtered devices and select one to connect
4. Explore characteristics and enable notifications on desired ones
5. Write commands to control characteristics
6. Monitor notifications in real-time

## Architecture

- `dbus_helper.cpp/h` - Low-level D-Bus communication wrapper
- `bluetooth_manager.cpp/h` - High-level BlueZ interface and device management
- `main.cpp` - CLI interface and main application logic

## Troubleshooting

- Ensure BlueZ is running: `sudo systemctl status bluetooth`
- Check D-Bus permissions for BlueZ access
- Run as root if experiencing permission issues
- Verify Bluetooth adapter is available: `hciconfig`
