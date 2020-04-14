<p align="center">
  <img src="dumpling.png" alt="Logo" width="80" height="80">

  <h3 align="center">Dumpling</h3>

  <p align="center">
    A simple Wii U file dumper, developed with the intent of making Cemu set-up faster and easier.
  </p>
</p>



## Installation

Dumpling uses iosuhax and requires [MochaCFW](https://gbatemp.net/threads/mocha-cfw-the-sweet-chocolate-to-your-latte.452940/) to run, and won't work without it. You can probably use a fw.img with iosuhax as well, but I haven't tested that and it's easier to use Mocha.

To install, simply download the dumpling `.elf` file from the [releases](https://github.com/emiyl/dumpling/releases/latest) page, and drag and drop it to the `/wiiu/apps` folder on your SD card.

## Features

- Dumps files needed for Cemu online play
  - You must dump `otp.bin` and `seeprom.bin` separately with [wiiu-nanddumper](https://github.com/koolkdev/wiiu-nanddumper)
- Dumps the Friends List app for use in Cemu
- Create region-free decrypted game disc dumps
- Dumps all digital games, updates and DLC on the system
- Dumps all game saves on the system
- Dumps the entire decrypted nand
  - This can take up to 2 hours on a 32GB Wii U if all storage is used up

## To-do

1. Dump `otp.bin` and `seeprom.bin`
2. Dump specific titles/updates/DLC
3. Make it "Mocha-less"

## Credits

- dimok789 for [ft2sd](https://github.com/dimok789/ft2sd/)
- dimok789 and FIX94 for [FTPiiU Everywhere](https://github.com/FIX94/ftpiiu/tree/ftpiiu_everywhere)
- shepgoba and rw-brick for being good at C
- chrissie, Crementif and CrafterPika for testing
