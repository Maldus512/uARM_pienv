# uARM_pienv

Experimenting bare metal programming on Raspberry pi 3. This is a simple example using some of the basic features of the board:
  - On board led blinking
  - Gpio 4 blinking (if external led is connected)
  - UART (hello world on boot)
  - Interrupts
  
Run make to build (needs arm-none-eabi toolchain). The resulting kernel.img is to be substituted to the corrisponding image in an 
already flashed SD card, or simply in the first fat32 partition containing start.elf and bootcode.bin from broadcom propretary
firmware (https://github.com/raspberrypi/firmware/tree/master/boot).

To add an external application compile it as .elf (with the same cross-compiling toolchain) and then run
```
$ make APP=example.elf
```
The kernel will launch the main function. Example of external application: https://github.com/Maldus512/uARM_kaya

As of now this work has been built putting together the following pre-existing tutorials/resources:
  - http://wiki.osdev.org/Raspberry_Pi_Bare_Bones
  - https://adamransom.github.io/posts/raspberry-pi-bare-metal-part-1.html
  - http://www.valvers.com/open-software/raspberry-pi/step01-bare-metal-programming-in-cpt1/
  - https://github.com/gingold-adacore/rpi3-fosdem17
  - https://github.com/vanvught/rpidmx512
  - https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
  
 This project is licensed under the terms of the Beerware license.
