# ep-agora-ble-reference-app
Bluetooth Low Energy reference application for the EP Agora IoT module, powered by Mbed-OS

This example shows how to set up Agora as a BLE GATT Server, allowing a peer to read, write, and subscribe to all available sensors/actuators on the board.

## Building

To build the example, you must already have set up a toolchain support by Mbed, and have Mbed-CLI build tools installed. For more instructions on how to get started with Mbed development, see [associated documentation here](https://os.mbed.com/docs/mbed-os/v5.14/tools/installation-and-setup.html).

After you have Mbed tools set up, you can build this example by following these steps (works with Mbed CLI, see IDE documentation for Mbed Studio):

1.) Clone the repository: `git clone https://github.com/EmbeddedPlanet/ep-agora-ble-reference-app.git && cd ep-agora-ble-reference-app`
2.) Download all dependencies: `mbed new . && mbed deploy`
3.) Set up your toolchain and target, eg: `mbed toolchain GCC_ARM && mbed target EP_AGORA`
4.) Build the example: `mbed compile`

Once the build is complete, you can upload the example to your board using the drag-and-drop programming feature of the Flidor debug board, also available from Embedded Planet.

Simply drag the hex output file, `BUILD/EP_AGORA/GCC_ARM/ep_agora-ble-reference-app.hex` (exact location depends on your toolchain), and place the file on the USB storage device that shows up when you plug in the Flidor board.

## Example Operation

Note: You can see detailed runtime information and debug UART output from the Agora module by opening a serial port connection.
The Flidor dev kit includes this functionality and enumerates as a USB to Serial adapter when connected. Use a serial terminal application, like Minicom (Linux) or TeraTerm (Windows) to
view the debug UART output. The default settings are show in the screenshot below:



When the example starts running, you should see the on-board red LED blinking slowly. This means the example successfully initialized all sensors and systems and is now advertising over BLE.

There are several ways of interacting with Agora over BLE. The simplest way is to download a generic BLE scanner app on a smartphone. 
We recommend the LightBlue app from PunchThrough or nRFConnect from Nordic Semiconductor. Both of these apps provide easy ways
of scanning for, connecting to, and interacting with BLE devices like Agora.

Alternatively, if you have a Nordic Semiconductor Devkit, you can download thei nRFConnect for Desktop application which will allow you to
scan for and interact with BLE devices using a DK and an app on your computer.

The screenshots below show how to interact with Agora and read sensor data over BLE using the LightBlue app mentioned above.

1.) Open the app on your smartphone and drag downwards on the list to begin scanning. A device named "EP Agora" should be listed in the scan results.

2.) Tap on the "EP Agora" entry to connect to your Agora board. At this time, you should notice the on-board red LED blinking faster. This indicates the Agora board has connected to a peer over BLE.

3.) Once the app has discovered all services/characteristics, you can browse what's available over BLE. For example, to view the ambient temperature in the room,
scroll down through the services list to find the service beginning with `00000001`, and tap on the characteristic with UUID `0x2A6E` ([Temperature](https://www.bluetooth.com/specifications/gatt/characteristics/)).

4.) This will open a detailed view of the characteristic, showing its current value, and allowing you to read a new value, or subscribe to notifications on when it changes.
By default, the data is shown in hexadecimal format. You can change this by tapping "Hex" up in the corner. On this page, you will be able to change the format to something
more human-readable, like a `uint16`.

5.) In my case, the current value of the sensor was `2300` in decimal. According to the BLE GATT specification for the `0x2A6E` characteristic, this value shows
the temperature in increments of 0.01 degrees Celsius. So the ambient temperature at this moment is 23C, around standard room temperature.

All of the sensors available on Agora are presented over BLE in this example. Since some of the data types are not covered by a standard GATT characteristic, you will find that many of them have custom UUIDs (128-bits long).

For more information on what the custom UUIDs are and how data is represented in this example, see the BLE GATT specification for this example in the docs folder.

## APIs and Concepts Exemplified

This example shows the use of:

- Mbed's BLE API to set up a GATT Server
- The use of the reference BLE Services for each on-board sensor (available in our [ep-oc-mcu library](https://github.com/EmbeddedPlanet/ep-oc-mcu))
- Setting up a basic file system using the default system block device (external QSPI on Agora), used for storing BLE pairing credentials
- Initializing and polling on-board sensors
- Using several APIs available from the [ep-oc-mcu library](https://github.com/EmbeddedPlanet/ep-oc-mcu), including ButtonIn and CallChain
