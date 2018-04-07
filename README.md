# lcdTerminal
A usb-serial VT100/ANSI terminal using an LCD display module

The software for this project runs on an arduino-nano board.
The 'nano provides:
* a serial interface via USB to whatever host computer needs a display
* a parallel interface to an industry-standard LCD display module

The display module I used is a 4-line, 20 character module. The code should be easily adaptable to other
similar modules.

The terminal would probably also work via a standard RS232 interface to the 'nano. You'd have to build
or buy a TTL-to-RS232 serial adaptor (take a look on pollin.de, for example) but that's a hardware problem
that's out of scope for this README :-)

If you want to build this from the command line, you'll need to adapt the Makefile. You'll see that
in my environment I have a file called Arduino.make sitting in a directory that contains my collection
of Arduino environments. I found the file on the internet somewhere, but since it's GPL I''ve
reproduced my copy here in the external/ subdirectory.
If you want to look for a newer version you might want to start at https://github.com/sudar/Arduino-Makefile
