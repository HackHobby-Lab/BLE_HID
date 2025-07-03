| Supported Targets | ESP32 | ESP32-S3 |
| ----------------- | ----- | -------- |

# ESP BLE HID 

This example implement a BLE HID device profile related functions, in which the HID device has 4 Reports:

1. Mouse
2. Keyboard and LED
3. Consumer Devices
4. Vendor devices

Users can choose different reports according to their own application scenarios.
BLE HID profile inheritance and USB HID class.

## How to Use 

Before project configuration and build, be sure to set the correct chip target using:

```bash
idf.py set-target <chip_name>
```

### Hardware Required

* A development board with ESP32/ESP32-S3
* A USB cable for Power supply and programming

### Build and Flash

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type ``Ctrl-]``.)


## Troubleshooting

Just Pray it works