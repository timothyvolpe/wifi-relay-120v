# 120 WiFi Controllable Relay
120V Multi-wire Branch WiFi Relay (ESP)

## Installation

The code was originally written in Visual Studio and compiled with Visual Micro. To compile and run it this way, simply create a Visual Micro Arduino project and add all the neccessary files to the project.

To use the Arduino IDE, it is more complicated as one must either create a custom library for the .cpp files, or copy the contents of each .cpp file into its respective .h and include them.

## Usage

In conjunction with project: https://www.timvolpe.com/projects/wifipoolpump

The code is designed to work with a Home Assistant server, or be operated by the buttons on the front of the device. The program button sets the relay, while the reset button resets the relay (and restarts the device).

When a device is first used and no WiFi credentials are stored in memory, the device defaults as an access point, and when connected to provides a portal for entering the appropriate WiFi credentials.

The Home Assistant plugin is available in https://github.com/timothyvolpe/featherstone-ha

## Credits:
- Timothy Volpe, Programming
