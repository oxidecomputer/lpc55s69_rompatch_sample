= A small exploit

This demonstrates some weaknesses with the LPC55S69 part.

= Background

XXX

= What is this program doing

This program is does a minimal amount of setup to get running in userspace.
When running normally, the LED blinks green and makes a system call after
every loop. If a bus fault occurs the LED blinks blue.

There is another function which reads the cpuid register (a privileged
register) and then blinks the LED red. This is not called under regular
code paths.

= How to build

`make` will build the program that just blinks the LED green.

`EXTRA_OPTION=CAUSE_FAULT make` will build the program to try and read the
cpuid register when we are in userspace. This will cause a busfault.

`EXTRA_OPTION=EXPLOIT` make will build the program to run the flash patcher
before starting the green LED loop. This will cause the green leds to
briefly turn on before blinking red.
