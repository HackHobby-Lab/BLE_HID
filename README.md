![Platform](https://img.shields.io/badge/Platform-ESP32-yellow.svg) ![ESP-IDF](https://img.shields.io/badge/Framework-ESP--IDF-D32F2F?logo=espressif&logoColor=red)




# BLE-HID Keyboard & Mouse 
A BLE-HID project for microcontroller platforms (e.g., ESP32 series). Enables keyboard, mouse, (HID over GATT Profile) support via BLE and replay the commands over GATT.

This features a custom GATT characteristics exposed for communication especially for receiving commands and replaying them. 
You can use python script to connect to the device and send the commands.





## Download

use git to clone the repository.

```bash
git clone https://github.com/HackHobby-Lab/BLE_HID.git
cd BLE_HID
```

## Usage

You can open this project with ESP-IDF and then build the code and flash it on to your ESP32.

Start with cleaning the project,
```
idf.py fullclean
```
To build the code,
```
idf.py build
```
To flash and monitor the device,

```
idf.py -p COMx flash monitor
```

If you need to find your COM port, either go to device manager and look for device under `ports` or a handy trick in `CMD` is,

```
mode 
```
It'll print available  COM Ports.

## Python Client
Python file is located under `PythonClient` folder.
It is named `main.py`.  In order to run it you should have the following things installed.
1. Python 3
2. bleak 

And then once python is installed just navigate to the `PythonClient` folder and navigate to the `BLEHID\Scripts`
and from there just run `.\activate`.
This will create the virtual environment. Or you can simply create your own environment. 

Now you are ready to install the `bleak`. Just run following command,
```
pip install bleak
```
Make sure you are in virtual environment when you run this.

Once it is down, again navigate back to `PythonClient` main folder and there run,
```
python ./main.py
```
And your script will start running.  And, your are good to go.

## License

[MIT](https://choosealicense.com/licenses/mit/)  
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
