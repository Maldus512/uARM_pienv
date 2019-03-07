# MaldOS

## What is this?

This is a simple Hardware Abstraction Layer meant to run a toy OS on Raspberry Pi 3. The library provides initialization of real and fake pheriperals, and a user application can be linked to resulting ELF to simplify the development process.

The objective of this project is twofold:

1. Provide an environment where CS students can develop a toy OS, following the Kaya OS philosophy (http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.98.3370&rep=rep1&type=pdf)
1. Provide a tutorial on how to write an Operating System on Raspberry Pi 3 (and armv8 architecture in general)

## Requirements

#### Mandatory

- python (required by scons)
- scons
- An appropriate toolchain to build aarch64 code (aarch64-linux-gnu or aarch64-elf; arm64 systems can use gcc)

#### Optional

- A Raspberry Pi 3
- A micro SD card
- `qemu-system-aarch64 >= 2.12` (previous versions do not cover Raspberry Pi 3)
- `aarch64-linux-gnu-gdb` or `aarch64-elf-gdb`

## Facilities Provided

The HAL library provides drivers for:

- Accessing fake tape devices (TAPEn files residing in the SD card boot partition)
- Accessing fake terminal devices (represented as 4 "windows" on the display)
- Accessing UART0
- Accessing the System Timer (not available under qemu) and internal ARM timer
- Context switch, interrupt and system call management
- GPIOs (obviously not available under qemu)

## Building

This repository uses scons (https://scons.org/) as automated building tool. The usage is similar to make:

- `scons` builds the HAL
- `scons example` builds the example code
- `scons all` builds both HAL and example code
- `scons -c` clears the build
- `scons run` runs the HAL on qemu with no guest kernel. By default it initializes all the peripherals and then just echoes everything on UART.
- the `--app` option for scons specifies an application to be linked together with the HAL. The command `scons all && scons --app=example/app.elf run` runs the provided example.

## Running

AArch64 code can be run both on qemu or on real hardware.

### Raspberry Pi 3

To run the compiled `kernel8.img` on a real Raspberry pi 3 you are going to need a micro SD card formatted with the first partition being FAT32 (not differently from a Raspbian image). The Raspberry Pi boot process requires 4 files:

- `bootcode.bin` : proprietary Broadcom firmware
- `fixup.dat` : proprietary Broadcom firmware
- `start.elf` : proprietary Broadcom firmware
- `kernel8.img` : your 64-bit OS

A working version of the firmware can be found in the `boot` subdirectory; the up to date version is published here: https://github.com/raspberrypi/firmware/tree/master/boot
Only the mentioned files are needed.
The fastest (and simplest) way to run a custom kernel is to download a Raspbian (or other Raspberry Pi OS) image and substituting the 'kernel8.img' file; otherwise an SD card needs to be formatted with a FAT32 partition and the files manually added.

With the SD card inserted, simply power on the Raspberry Pi. Some welcome messages should be displayed on screen and through the default UART serial console (baudrate 115200).

### qemu

To run the compiled `kernel8.img` on qemu you need the package `qemu-system-aarch64`. The scons script has a `run` target option that runs the command:

```lang=sh
qemu-system-aarch64 -M raspi3 -kernel boot/kernel8.img -drive file=test.dd,if=sd,format=raw -serial stdio
```

- `-kernel` specifies the kernel image to be executed.
- `-drive` specifies a file to be loaded as micro SD card. This file needs not to contain `kernel8.img` or to be a valid image file at all, since qemu loads the kernel through the `-kernel` option.
- `-serial stdio` indicates to redirect the UART0 serial output to stdio

## Debugging

A qemu-running kernel can be debugged using the options '-s' (run a gdb server at localhost:1234) and '-S' (freeze the cpu at startup). An appropriate gdb client for the aarch64 architecture is needed, such as `aarch64-linux-gnu-gdb` or `aarch64-elf-gdb`.

## TODO

- registers x26, x27 and x28 are corrupted during interrupt handling. Avoid that or update the documentation accordingly

## Acknowledgements

As of now this work has been built putting together the following pre-existing tutorials/resources:

- https://github.com/bztsrc/raspi3-tutorial
- https://github.com/LdB-ECM/Raspberry-Pi
- http://wiki.osdev.org/Raspberry_Pi_Bare_Bones
- https://adamransom.github.io/posts/raspberry-pi-bare-metal-part-1.html
- http://www.valvers.com/open-software/raspberry-pi/step01-bare-metal-programming-in-cpt1/
- https://github.com/gingold-adacore/rpi3-fosdem17
- https://github.com/vanvught/rpidmx512
- https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf

 This project is licensed under the terms of GPL v3.
