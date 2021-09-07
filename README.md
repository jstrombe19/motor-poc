v0.1.0

Data is written to a csv file identified by the date and time the collection began (in UTC). The data written is written in the following order: raw position, processed position (which adjusts the position value if it is negative), raw index, and processed index (which adjusts the index value if it is negative).

This is the first version-controlled release of the nexys calibrator motor control repository. It was used for all vibration test activities as well as the first round of the lifecycle testing for the calibrator mechanism.

In order to begin running test operations, a user must first clone this repository from gitserver, then create three additional directories at the same level as /inc and /src : /data, /bin, and /obj. The Makefile is included for ease of use. Once the directories are created, run `make all` from the root directory.

To run test operations, execute ./bin/motor-control-cli. The interface will open unless there is an issue with creating a serial connection to the Nexys FPGA board. This connection is hardcoded as /dev/ttyUSB0, so if there are additional connections needed, make sure the Nexys FPGA board is connected first and powered on before making any other connections. It does not appear as a serialized connection, and so will appear as the next available USB device. Adjust these connections as needed to establish connection and launch the interface.

Upon launching the interface, the uptime will automatically begin incrementing. Data is not recorded until commanded to do so, so this counter is meant to be strictly informative. To begin recording data, press `1`. Before performing any other operation, a home position must be established. This is accomplished by pressing `3`, which will execute a performance maneuver (rotating from hardstop to hardstop and returning to center). Once this operation is complete, press `7` to define the home position.

From here, the performance maneuver, functional maneuver (60 degrees from center in each direction, returning to center), or lifecycle test operations may be executed, as indicated by the interface. 

The data is recorded in a pseudo-csv format, with raw encoder position as the first value, processed encoder position (a correction to show it as a positive value when the motor rotates CCW from the home position), raw encoder index, and processed encoder index (same correction as encoder position). Encoder position is indicative of encoder movement. Encoder index is indicative of full encoder rotations, and only increments or decrements upon completing a full rotation. The delta between encoder index records should be equal to 2048, though 2047 and 2049 are also acceptable values due to the nature of the conversion from encoder counts to degrees of rotation. All data is recorded in hexadecimal format. 

In addition to the encoder data, all movement commands are inserted in the data with leading and trailing newline commands. The movement command details are listed in an effort to simplify reviewing data.
