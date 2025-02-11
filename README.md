# loom_by_light

## Description:

arduino code for converting rows of a bitmap image into lights on an addressable light strip for use on a custom loom.
The purpose of this is for a custom loom, which allows the user to manually choose which
of 2 colours to use on each loom heddle, thereby creating custom 2d loom images based off
a bitmap.

## contents:

- images folder includes test bitmaps
- all-functions.txt is a list of all functions included in the program.
- loom_by_light folder contains loom_by_light.ino sketch.

## Required libraries:

- LiquidCrystal by Arduino, Adafruit -version last tested: 1.0.7.
- Adafruit NeoPixel by Adafruit -version last tested: 1.12.4,

## Program instructions:

### File Navigation:

the program starts allowing you to navigate files in the SD card's root.

- up/down - navigates files
- right - selects the bitmap to load.

### Configure:

Configure is opened once the file is selected, configures the settings such as max LED count,
image LED offset (the offset the image is on the led strip), brightness, and reset configure to default.
These settings are stored in the arduino's memory when it is shut off.

- up/down - change configure mode.
- left/right - decrement, increment value.
- select - exit configure mode

### Row Navigation:

Once the bitmap is opened you navigate rows of the bitmap and they are displayed on the LED strip as on or off.
The saved row is automatically loaded if one exists in memory.

- up/down - decrement, increment a row (the iteration loops at the end).
- right - save the current row number (this is remembered when the arduino is shut down).
- left - load the saved row number.

## Notes:

- The bitmap to be opened must be 24 bits per pixel and not compressed.
- If you are not using the LCD used in the development of this you may need to change the PIN's, see Line 78:
  const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7; //the pin values for the lcd display.
- The LED strip is connected to pin D01, to change this change Line 74: #define LED_PIN 1

## All hardware was purchased from QKits electronics: [QKits](https://store.qkits.com/)

- Adruino: Arduino UNO R3 Clone
- Addressable LED Strip: WS2812
- LCD shield with buttons: D1 ROBOT LCD Keypad Shield 2x16 SKU: SHLCD01
- Screw terminal shield SKU: PSSXB
- SD card holder and RTC shield SKU: SHDATALG

## Info:

- The program is written by Luke Johnson, organized by Elizabeth Johnson.
- this version of Loom by Light: 1.6.3
- some of the code was modified after being sourced from [bitsnbytes.co.uk](https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette)
- last edit date: 2025-February-11
