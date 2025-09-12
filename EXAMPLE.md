# Example Usage

This document provides an example of how to use the Bluetooth Device Manager application.

## Typical Usage Flow

### 1. Start the Application
```bash
sudo ./bscm-bluetooth-manager
```

### 2. Scan for Devices
```
=== Main Menu ===
1. Scan for devices
2. Set service filter
3. List devices with desired services
4. Connect to device
5. Disconnect from device
6. Manage characteristics
7. Process notifications
0. Exit

Enter your choice (0-7): 1
```

The application will scan for 10 seconds and display found devices:
```
Found 3 devices:
[0] My Fitness Tracker (AA:BB:CC:DD:EE:FF)
    Services: 0000180f-0000-1000-8000-00805f9b34fb, 0000180a-0000-1000-8000-00805f9b34fb
[1] Smart Watch (11:22:33:44:55:66) [CONNECTED]
    Services: 0000180f-0000-1000-8000-00805f9b34fb
[2] Unknown Device (99:88:77:66:55:44)
    Services: 00001800-0000-1000-8000-00805f9b34fb
```

### 3. Set Service Filter (Optional)
```
Enter your choice (0-7): 2
Enter desired service UUIDs (comma-separated, or 'none' to clear): 0000180f-0000-1000-8000-00805f9b34fb
Set desired services: 0000180f-0000-1000-8000-00805f9b34fb
```

### 4. Connect to a Device
```
Enter your choice (0-7): 4
Select device to connect:
[0] My Fitness Tracker (AA:BB:CC:DD:EE:FF)
[1] Smart Watch (11:22:33:44:55:66) [CONNECTED]

Enter your choice (0-1): 0
Connecting to device: /org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF
Successfully connected to device
```

### 5. Manage Characteristics
```
Enter your choice (0-7): 6
Select connected device:
[0] My Fitness Tracker (AA:BB:CC:DD:EE:FF) [CONNECTED]

Enter your choice (0-0): 0

=== Characteristic Management ===
Device: My Fitness Tracker

Characteristics:
[0] UUID: 00002a19-0000-1000-8000-00805f9b34fb
    Path: /org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0001/char0002
    Flags: read notify
[1] UUID: 00002a50-0000-1000-8000-00805f9b34fb
    Path: /org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0001/char0004
    Flags: write write-without-response

Actions:
1. Enable notifications
2. Disable notifications
3. Read characteristic
4. Write to characteristic
0. Back to main menu

Enter your choice (0-4): 1
Select characteristic: 0
Enabling notifications for: /org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0001/char0002
Notifications enabled
```

### 6. Process Notifications
```
Enter your choice (0-4): 0
Enter your choice (0-7): 7
Processing notifications for 10 seconds...

*** NOTIFICATION from /org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0001/char0002 ***
Data: 64
ASCII: d

*** NOTIFICATION from /org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0001/char0002 ***
Data: 63
ASCII: c
```

### 7. Send Commands
```
Actions:
1. Enable notifications
2. Disable notifications
3. Read characteristic
4. Write to characteristic
0. Back to main menu

Enter your choice (0-4): 4
Select characteristic: 1
Enter hex data to write (e.g., '01 02 03' or '010203'): 01 FF
Writing to characteristic: /org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF/service0001/char0004
Write successful
```

## Common Service UUIDs

- **Battery Service**: `0000180f-0000-1000-8000-00805f9b34fb`
- **Device Information**: `0000180a-0000-1000-8000-00805f9b34fb`
- **Heart Rate**: `0000180d-0000-1000-8000-00805f9b34fb`
- **Generic Access**: `00001800-0000-1000-8000-00805f9b34fb`
- **Generic Attribute**: `00001801-0000-1000-8000-00805f9b34fb`

## Common Characteristic UUIDs

- **Battery Level**: `00002a19-0000-1000-8000-00805f9b34fb`
- **Manufacturer Name**: `00002a29-0000-1000-8000-00805f9b34fb`
- **Serial Number**: `00002a25-0000-1000-8000-00805f9b34fb`
- **Heart Rate Measurement**: `00002a37-0000-1000-8000-00805f9b34fb`

## Troubleshooting

If you encounter connection issues:
1. Ensure BlueZ is running: `sudo systemctl status bluetooth`
2. Check if the device is in pairing mode
3. Try connecting manually first with `bluetoothctl`
4. Verify the device supports GATT services