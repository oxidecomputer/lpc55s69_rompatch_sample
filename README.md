
# LPC55S69 ROM Patch Privilege Escalation and Arbitrary Code Execution PoC

This repository contains a TrustedFirmware-M non-secure app demonstrating that
non-secure code can utilize the LPC55 ROM patch hardware to gain arbitrary code
execution at secure, privileged mode. Non-secure, unprivileged arbitrary memory
write capability is a prerequisite for this vulnerability.

The PoC takes advantage of NXP's implementation of TrustedFirmware-M's Internal
Trusted Storage (ITS) which performs flash operations using the LPC55 ROM's
Flash API.  A thread is created which increments a counter stored via ITS.  This
ensures that the Flash ROM API will be called on a regular basis.  In a real
TrustedFirmware-M application, any exposed API that writes to flash can be used
for the same purpose.

A second thread is created to deploy the PoC from a non-secure context. The
shellcode is first loaded into an unused portion of the ROM.  As the ROM patch
appears to be size limited to 8 words (32 bytes), constants used by the shellcode
are stored in non-secure memory and only a pointer to that non-secure memory is
appended to the shellcode.  Finally, the Flash_Write ROM API method is modified
to call the shellcode.

The shellcode performs a series of memory writes as specified by the constants
stored in non-secure memory.  The PoC provides constants that extend the SAU
non-secure code and data regions to cover the entire non-secure aliases of the
flash and SRAMs respectively. The Secure AHB controller is then reconfigured to
a default state such that security checking is disabled.  Combined, this allows
non-secure code to access all flash and SRAM contents including secure code and
data sections.

The PoC loader thread then waits for 10 seconds to ensure Flash_Write has been
indirectly called via ITS and the shellcode has been executed.  The PoC loader
thread then proceeds to read the first 256 bytes of the flash (containing the
secure application's vector table which can be easily verified as pointing to
secure addresses) via the non-secure alias starting at 0x0.  Had the shellcode
not executed, attempting to read these locations would have resulted in a
SecureFault or BusFault depending on whether the SAU or Secure AHB denied the
request.

## How to build

```shell
apt-get install gcc-arm-none-eabi pipenv

git clone -b TF-Mv1.2.0 https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git
git clone git@github.com:oxidecomputer/lpc55s69_rompatch_sample

cd trusted-firmware-m
# Fix error in CMake config that treats TFM_TEST_REPO_VERSION as a path instead of # a string
git cherry-pick   8501b37db8e038ce39eb7f1039a514edea92c96e
pipenv shell
pip install -r tools/requirements.txt
pip install cmake

cmake -S . -B cmake_build \
  -DTFM_TEST_REPO_VERSION=TF-Mv1.2.0 \
  -DTFM_PLATFORM=nxp/lpcxpresso55s69 \
  -DTFM_TOOLCHAIN_FILE=toolchain_GNUARM.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DBL2=OFF \
  -DTFM_APP_PATH=../lpc55s69_rompatch_sample/
cmake --build cmake_build -- install
```

Built images are written to cmake_build/bin in a variety of formats.  `tfm_s` files are the Secure image while `tfm_ns` are the Non-secure image.

## Flash layout

Flash layout on LPC55S69 without BL2:

```text
* 0x0000_0000 Secure + Non-secure image area (512 KB):
*    0x0000_0000 Secure     image (256 KB)
*    0x0004_0000 Non-secure image (256 KB)
* 0x0008_0000 Protected Storage Area (10 KB)
* 0x0008_2800 Internal Trusted Storage Area (8 KB)
* 0x0008_4800 NV counters area (512 B)
* 0x0008_4A00 Unused (77.5 KB)
```

## How to flash using blhost

* Connect USB cable to High Spd port (P9)
* Hold ISP button while pressing and releasing Reset button to enter ISP mode
* `blhost -u 0x1fc9,0x0021 flash-erase-all`
* `blhost -u 0x1fc9,0x0021 write-memory 0x00000000 cmake_build/bin/tfm_s.bin`
* `blhost -u 0x1fc9,0x0021 write-memory 0x00040000 cmake_build/bin/tfm_ns.bin`
* Press and release Reset button

NOTE: I've seen blhost fail often during this process.  Repeat the whole sequence until each step is successful.  Waiting a few seconds after flash-erase-all seems to help.

## Console Output

Both secure and non-secure images write their output to USART0.  On-board LPC-Link2 provides this as a USB-ACM device.  Otherwise, attach 3.3v TTL serial cable to header P8.

NOTE: Since the counter is written to flash, it is persisted across resets but will be cleared with `blhost flash-erase-all`.

NOTE: After `blhost flash-erase-all`, the Internal Trusted Storage area is uninitialized which seems to cause a hang when creating the counter.  If you do not see constant output showing an increasing counter value, reset the device once.  If it still hangs after a reset, something is wrong.

```text
=== [SAU NS] =======
NS ROM Base: 0x00040000
NS ROM Limit: 0x0007FFFF
NS DATA Base: 0x20022000
NS DATA Limit: 0x20043FFF
NSC Base: 0x1003FCC0
NSC Limit: 0x1003FF80
PERIPHERALS Base: 0x40000000
PERIPHERALS Limit: 0x4010FFFF
=== [AHB MPC NS] =======
NS ROM Base: 0x00040000
NS ROM Limit: 0x0007FFFF
NS DATA Base: 0x20022000
NS DATA Limit: 0x20043FFF
[Sec Thread] Secure image initializing!
Booting TFM v1.2.0
[Crypto] MBEDTLS_TEST_NULL_ENTROPY is not suitable for production!
LPC55 ROM Patch PoC starting as non-secure app...
[TOUCH FLASH API] Started[LPC55
PoC] Good morning from this thread
[TOUCH FLASH API] Counter: 0
[TOUCH FLASH API] Counter: 1
[TOUCH FLASH API] Counter: 2
[TOUCH FLASH API] Counter: 3
[LPC55 PoC] Starting rom patch
[LPC55 PoC] Finished ROM patch
[TOUCH FLASH API] Counter: 4
[TOUCH FLASH API] Counter: 5
[TOUCH FLASH API] Counter: 6
[TOUCH FLASH API] Counter: 7
[TOUCH FLASH API] Counter: 8
[TOUCH FLASH API] Counter: 9
[TOUCH FLASH API] Counter: a
[TOUCH FLASH API] Counter: b
[TOUCH FLASH API] Counter: c
[TOUCH FLASH API] Counter: d
[LPC55 PoC] Start of Secure Code:
[LPC55 PoC] 0x0:        0       8       0       30      31      1       0       10      9d      1       0       10      31      ab      0     0
[LPC55 PoC] 0x10:       35      ab      0       10      39      ab      0       10      99      1       0       10      3d      ab      0     0
[LPC55 PoC] 0x20:       0       0       0       0       0       0       0       0       0       0       0       0       5       ab      0     0
[LPC55 PoC] 0x30:       99      1       0       10      0       0       0       0       a9      1       0       10      ad      1       0     0
[LPC55 PoC] 0x40:       b1      1       0       10      b5      1       0       10      b9      1       0       10      bd      1       0     0
[LPC55 PoC] 0x50:       c1      1       0       10      c5      1       0       10      c9      1       0       10      cd      1       0     0
[LPC55 PoC] 0x60:       d1      1       0       10      d5      1       0       10      d9      1       0       10      dd      1       0     0
[LPC55 PoC] 0x70:       e1      1       0       10      e5      1       0       10      e9      1       0       10      ed      1       0     0
[LPC55 PoC] 0x80:       f1      1       0       10      f5      1       0       10      f9      1       0       10      fd      1       0     0
[LPC55 PoC] 0x90:       1       2       0       10      5       2       0       10      9       2       0       10      d       2       0     0
[LPC55 PoC] 0xa0:       11      2       0       10      15      2       0       10      19      2       0       10      1d      2       0     0
[LPC55 PoC] 0xb0:       21      2       0       10      25      2       0       10      29      2       0       10      2d      2       0     0
[LPC55 PoC] 0xc0:       31      2       0       10      35      2       0       10      39      2       0       10      3d      2       0     0
[LPC55 PoC] 0xd0:       41      2       0       10      45      2       0       10      49      2       0       10      4d      2       0     0
[LPC55 PoC] 0xe0:       51      2       0       10      55      2       0       10      59      2       0       10      5d      2       0     0
[LPC55 PoC] 0xf0:       61      2       0       10      65      2       0       10      69      2       0       10      6d      2       0     0
[TOUCH FLASH API] Counter: e
```