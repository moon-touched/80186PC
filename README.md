# 80186PC

80186PC is an emulator, created to run a specific piece of software, a
real-mode Windows 3.0 application requiring a particular hardware
configuration. However, it is also may be of general interest.

The following XT-like hardware is emulated:
  
  * x86 CPU, using libx86emu.
	
  * Intel 8259 programmable interrupt controller.
  
  * Intel 8254 programmable interval timer. Note that the implementation of it
    is incomplete and is suitable for DOS only.
	
  * Intel 8255 programmable peripheral interface, including various PC XT
    discretes.

  * Hercules Graphics Card.
  
  * Logitech bus mouse.
  
  * XTIDE interface and one hard drive on it. Original machine used a MFM hard
    drive, however, IDE was easier to emulate and a specific type of drive was
	unimportant.
	
  * XT Keyboard.
  
  * Intel Above Board EMS card, implementing 8 MiB of small-frame EMS.
  
Please note that all other XT hardware is not implemented, including floppy
drive and PC speaker, as it is was irrelevant for the purpose this emulator was
created for.

80186PC currently requires Windows, primarily because it uses virtdisk API
in order to access VHD disk images.
  
# Building

80186PC may be built using normal CMake procedure, and requires SDL to be
installed.

# Running

A bootable VHD image containing (at least) suitable DOS should be created
before running 80186PC. Afterwards, 80186PC may be run, and should be passed
the path to that image as the only command line argument.

80186PC uses left alt key as a mouse capture release key, and emulates extended
XT keyboard.

# Licensing

80186PC is licensed under the terms of the MIT license (see LICENSE).

Note that 80186PC also includes a BIOS image in 80186PC/Resources/bios.bin,
derived from Turbo XT Bios and XTIDE bios, and MDA font file in font.bmp in the
same directory, derived from the original IBM ROMs.

Also note that 80186PC references libx86emu as a submodule, which is licensed
under its own license.
