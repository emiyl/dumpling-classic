# Dumpling

Dumpling is a simple Wii U file dumper, developed with the intent of making Cemu set-up faster and easier.

## Installation

Dumpling uses iosuhax and requires [MochaCFW](https://gbatemp.net/threads/mocha-cfw-the-sweet-chocolate-to-your-latte.452940/) to run, and won't work without it.

To install, simply download the dumpling `.elf` file from the [releases](https://github.com/emiyl/dumpling/releases/latest) page, and drag and drop it to the `/wiiu/apps` folder on your SD card.

## Features

- Dumps files needed for Cemu online play
  - You must dump `otp.bin` and `seeprom.bin` separately with [wiiu-nanddumper](https://github.com/koolkdev/wiiu-nanddumper)
- Dumps the Friends List app for use in Cemu
- Dumps all digital games, updates and DLC stored on internal storage
- Dumps all game saves on internal storage
- Dumps the entire decrypted nand
  - This can take over 2 hours on a 32GB Wii U
  
## To-do

Currently the software will dump _all_ games (or updates/DLC/saves) on internal storage. In a later update a feature will be added to only dump specific games.

Dumpling also only supports dumping from internal storage and USB, but disk dumping will be coming
