# How to build

```shell
apt-get install gcc-arm-none-eabi

git clone -b TF-Mv1.2.0 https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git
git clone -b tfm_poc git@github.com:oxidecomputer/lpc55s69_rompatch_sample

cd trusted-firmware-m
pipenv shell
pip install -r tools/requirements.txt
pip install cmake

cmake -S . -B cmake_build \
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
* Hold ISP button while pressing and releasing Reset button
* `./blhost -u 0x1fc9,0x0021 flash-erase-all`
* `./blhost -u 0x1fc9,0x0021 write-memory 0x00000000 cmake_build/bin/tfm_s.bin`
* `./blhost -u 0x1fc9,0x0021 write-memory 0x00040000 cmake_build/bin/tfm_ns.bin`
* Press and release Reset button

NOTE: I've seen blhost fail often during this process.  Repeat the whole sequence until each step is successful.  Waiting a few seconds after flash-erase-all seems to help.

## Console Output

Both secure and non-secure images write their output to USART0.  On-board LPC-Link2 provides this as a USB-ACM device.  Otherwise, attach 3.3v TTL serial cable to header P8.

NOTE: Since the counter is written to flash, it is persisted across resets but will be cleared with `blhost flash-erase-all`.

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
[TOUCH FLASH API] Started
[TOUCH FLASH API] Counter: 89
[TOUCH FLASH API] Counter: 90
[TOUCH FLASH API] Counter: 91
[TOUCH FLASH API] Counter: 92
[LPC55 PoC] Started
[TOUCH FLASH API] Counter: 93
```