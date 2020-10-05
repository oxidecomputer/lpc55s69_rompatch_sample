# A small exploit

This demonstrates some weaknesses with the LPC55S69 part.

# Background

The spreadsheet describing the LPC55S6x protected flash regions describes a
ROM patching region at `0x9ec00`. The details about what this region actually
contains are not documented, nor is the mechanism used to patch the ROM
described. Reverse engineering the ROM on the LPC55S69-EVK board lead to
the discovery of a custom hardware block for handling the flash patching.

The LPC55S6x ROM uses the `svc` handler for patching. The function at
`0x1300fab8` reads the ROM patch out of `0x9ec00`, copies the appropriate
functions to the syscall table in RAM at `0x14005a00` and uses a hardware
block with a base at `0x5003e000` to set up the patch.

A sample sequence looks like:

```
pyocd> r32 0x13001000
13001000:  00000000    ....
pyocd> r32 0x5003e0f4
5003e0f4:  08400007    .@..
pyocd> w32 0x5003e0f4 0x20000000
pyocd> w32 0x5003e100 0x13001000
pyocd> w32 0x5003e0f0 0xffffffff
pyocd> w32 0x5003e0f4 0x7
pyocd> r32 0x13001000
13001000:  ffffffff    ....
```

This hardware block is presumably used for bug fixing a (supposedly)
read only area. Unfortunately, this block is still accessable from both
privileged and non-priviledged accesses. This coupled with the use of the
`MEMREMAP` register provides a path for privilege escalation as follows:

- Use the flash patcher to patch a function pointer used in the system
call handler in ROM to a function of our choosing
- Set `MEMREMAP` to use the vector table in ROM instead of flash
- Make a system call which goes through ROM and into the function of our
choosing

# What is this program doing

This program is does a minimal amount of setup to get running in userspace.
When running normally, the LED blinks green and makes a system call after
every loop. If a bus fault occurs the LED blinks blue.

There is another function which reads the cpuid register (a privileged
register) and then blinks the LED red. This is not called under regular
code paths.

# How to build

`make` will build the program that just blinks the LED green.

`EXTRA_OPTION=CAUSE_FAULT make` will build the program to try and read the
cpuid register when we are in userspace. This will cause a busfault.

`EXTRA_OPTION=EXPLOIT` make will build the program to run the flash patcher
before starting the green LED loop. This will cause the green leds to
briefly turn on before blinking red.
