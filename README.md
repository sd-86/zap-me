# zap-me

With this library, you can operate various electric shock collars remotely using
your Arduino.

**WARNING**: E-Stim is not without risk. Make sure you know about the safety
rules and use this library responsibly and consensually. I am not responsible
for any damage caused by this library or its users.

You are **not** allowed to use this library with animals.

## Currently supported devices

This library currently supports the following devices:

* Chinese Shock Collar often designated with model number 880-3 Sometimes,
it is also labeled as "1000M IP6X". It has a characteristic remote with an
antenna almost as long as the remote, an LED next to it, supports 3 channels
and 3 functions (shock, vibration, sound).

* Dogtronic MT500 (experimental). This is a very old and expensive
collar that was made in Germany by Dogtronic (formerly known as Petsystems).
It features a 3-button remote with a small display and comes in an audio and
a vibration edition.

## Required Hardware

For the CH8803 and the Dogtronic collars you need a 433 Mhz transmitter, such as
the MX-FS-03V. These transmitters usually have 3 pins (1 for data, 1 GND, 1 VCC)
and connecting it to an Arduino is really easy: Just connect the data pin to
your designated transmit pin (any digital pin, e.g. PIN 10) and VCC/GND
according to specifications.

**Important**: If you want to achieve more range, you will have to solder an antenna
to the transmitter. Without an antenna, the range is still several meters in-door
in my tests though.

## FAQ

Q: Can I sniff my remote id with this library?

A: Not yet. Sniffing the id is possible with a MX-05V receiver but you will have
   to decode the data yourself. In the future, we might add a decoder to allow
   simultaneous use of the real remote and this library.

Q: Why is there no boost functionality for the Dogtronic collars?

A: There is no such function built into the collar. Hitting the boost button
   simply sends a continuous signal that is 2 levels higher than the current
   remote setting.

Q: Why can't I chose a random id with Dogtronic?

A: We don't know (yet). With the current implementation, we have found that
   only four ids work, see the comment in the header for more information.
