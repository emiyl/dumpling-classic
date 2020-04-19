<p align="center">
  <img src="dumpling.png" alt="Logo" width="80" height="80">

  <h3 align="center">Dumpling</h3>

  <p align="center">
    A simple Wii U file dumper, developed with the intent of making Cemu set-up faster and easier.
  </p>
</p>

## Installation

Dumpling uses iosuhax and requires [MochaCFW](https://gbatemp.net/threads/mocha-cfw-the-sweet-chocolate-to-your-latte.452940/) to run, and won't work without it. You can probably use a fw.img with iosuhax as well, but I haven't tested that and it's easier to use Mocha.

To install, simply download the dumpling `.zip` file from the [releases](https://github.com/emiyl/dumpling/releases/latest) page, and extract it to the root of your SD card.

## Features

- Dumps files needed for Cemu online play
  - You must dump `otp.bin` and `seeprom.bin` separately with [wiiu-nanddumper](https://github.com/koolkdev/wiiu-nanddumper)
- Dumps the Friends List app for use in Cemu
- Create region-free decrypted game disc dumps
- Dumps digital games, updates and DLC from the system or USB storage
- Dumps game saves from the system or USB storage
- Dumps the entire decrypted nand
  - This can take up to 2 hours on a 32GB Wii U if all storage is used up

## To-do

1. Dump `otp.bin` and `seeprom.bin`
3. Make it "Mocha-less"

## Credits

- dimok789 for [ft2sd](https://github.com/dimok789/ft2sd/)
- dimok789 and FIX94 for [FTPiiU Everywhere](https://github.com/FIX94/ftpiiu/tree/ftpiiu_everywhere)
- shepgoba, rw-r-r-0644, luigoalma, vgmoose and Pysis for helping me with the project
- chrissie, Crementif and CrafterPika for testing
