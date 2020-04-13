<p align="center">
  <a href="https://github.com/othneildrew/Best-README-Template">
    <img src="dumpling.png" alt="Logo" width="80" height="80">
  </a>

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
- Dumps all digital games, updates and DLC on the system
- Dumps all game saves on the system
- Dumps the entire decrypted nand
  - This can take up to 2 hours on a 32GB Wii U if all storage is used up
  
## To-do

Currently the software will dump _all_ games (or updates/DLC/saves) on internal storage. In a later update a feature will be added to only dump specific games.

Dumpling also only supports dumping from internal storage and USB, but disk dumping will be coming

## Credits

- dimok789 for writing [ft2sd](https://github.com/dimok789/ft2sd/) which this application is based off of
- shepgoba for being good at C
- chrissie and Crementif for helping me test USB dumping
