# CommandLine Clock

A clock that provides access to basic functionality through a command
line interface on the Serial port. The following functions are supported:

```
help [command]
    Print the list of supported commands.
list
    List the AceRoutine coroutines.
date [dateString]
    Print or set the date.
timezone [manual {offset} | dst (on | off)] |
    Print or set the current TimeZone.
basic [list] | extended [list] ]
    Print or set the currently active TimeZone.
sync [status]
    Sync the SystemClock from its external source, or print its sync
    status.
wifi (status | config [{ssid} {password}] | connect)
    Print the ESP8266 or ESP32 wifi connection info.
    Connect to the wifi network.
    Print or set the wifi ssid and password.
```

## Installation

### Arduino

* an Arduino-compatibile board with a serial port
* AceTime library (https://github.com/bxparks/AceTime)
* AceRoutine library (https://github.com/bxparks/AceRoutine)
* FastCRC (https://github.com/FrankBoesing/FastCRC)

Use the Arduino IDE to compile and upload the program and use the Serial
Monitor to type in the commands listed above.

Or use the `auniter.sh` tool from AUniter (https://github.com/bxparks/AUniter)
to type the commands directly in the terminal program:

```
$ auniter upmon nano:USB0
```

### Linux using UnixHostDuino

This clock has the interesting property that it can run on a Linux box using the
UnixHostDuino compatibility layer. This is very useful for testing and
debugging.

1) Install UnixHostDuino (https://github.com/bxparks/UnixHostDuino) as a sibling
project to this:

```
$ git clone https://github.com/bxparks/UnixHostDuino
```

2a) Ubuntu 18.04: Install zlib (https://www.zlib.net/) package to resolve the
dependency to the CRC32 routine:

```
$ sudo apt install zlib1g-dev
```

2b) Ubuntu 20.04: The zlib library may already be installed, I can't remember.

```
$ dpkg -l zlib1g-dev
ii  zlib1g-dev:amd64 1:1.2.11.dfsg-2ubuntu1.1 amd64        compression library - development
```

3) Run make:

```
$ make clean
$ make
```

4) Run the program:

```
$ ./CommandLineClock.out
setup(): begin
...
setup(): end
```

5) Type the `help` command:
```
> help
Commands:
  help [command]
  list
  date [dateString]
  sync [status]
  timezone manual {offset} | basic [list | {index}] | extended [list | {index}] | dst {on | off}]
```

## Persistent EEPROM Storage on Linux

When running on Linux (using UnixHostDuino), the timezone information that would
have been stored in the EEPROM on the device is stored in a file named
`commandline.dat` in the current directory.
