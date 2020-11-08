# zap-me

With this library, you can operate various electric shock collars remotely using
your Arduino.

**WARNING**: E-Stim is not without risk. Make sure you know about the safety rules
and use this library responsibly and consensually. I am not responsible for any
damage caused by this library or its users.

You are **not** allowed to use this library with animals.

## Currently supported devices

This library currently only supports the Chinese Shock Collar often designated
with model number 880-3. Sometimes, it is also labeled as "1000M IP6X". It has a
characteristic remote with an antenna almost as long as the remote, an LED next
to it, supports 3 channels and 3 functions (shock, vibration, sound).

## Required Hardware

For the CH8803 collar you need a 433 Mhz transmitter, such as the MX-FS-03V.
These transmitters usually have 3 pins (1 for data, 1 GND, 1 VCC) and connecting
it to an Arduino is really easy: Just connect the data pin to your designated
transmit pin (any digital pin, e.g. PIN 10) and VCC/GND according to specifications.

**Important**: If you want to achieve more range, you will have to solder an antenna
to the transmitter. Without an antenna, the range is still several meters in-door
in my tests though.
