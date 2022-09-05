.. image:: https://img.shields.io/badge/License-MIT-purple.svg
    :target: https://github.com/Dennis-van-Gils/project-Tachometer/blob/master/LICENSE.txt

Tachometer
==========

A stand-alone Arduino tachometer using a transmissive photointerrupter with an
optical encoder disk.

The rotation rate is displayed on an OLED screen. The units can be switched
between RPM, rev/s and rad/s by pressing one of the OLED screen buttons. The
display will go blank when no rotation has been detected after a certain
timeout period.

Change the global constant `N_SLITS_ON_DISK` of `main.cpp` to match the number
of slits on your encoder disk.

- Github: https://github.com/Dennis-van-Gils/project-Tachometer

Hardware
========
* Adafruit #3857: Adafruit Feather M4 Express - Featuring ATSAMD51 Cortex M4
* Adafruit #2900: Adafruit FeatherWing OLED - 128x32 OLED Add-on For Feather
* OMRON EE-SX1041 Transmissive Photomicrosensor

Click `here <https://github.com/Dennis-van-Gils/project-Tachometer/blob/main/docs/schematic_diagram.pdf>`_
for the electronic wiring diagram.
