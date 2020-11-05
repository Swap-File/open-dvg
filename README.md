# open-dvg
====
This is a heavily modified version of the v.st vector board that targets installation in arcade cabinets with an Electrohome GO5-802 / 805 monitor.

It uses the same serial protocol as the USB-DVG, meaning you can use standard builds of AdvanceMAME with it.

Protocol Documentation Here:  https://github.com/amadvance/advancemame/blob/master/advance/osd/dvg.c

Hardware and software is currently in BETA!  The PCB is very out of date, it requires a lot of hand rework and the Teensy 4.1 hangs off the end of it.  The software is not optimized either; consider it a proof of concept.  Buy a USB-DVG if you want something that works out of the box.

The original project which targets Vectrex monitors can be found here: https://github.com/osresearch/vst

## Step 1: Building the hardware
The circuit board design in this repo is not up to date.  See Op-Amp and DAC reference adjustment notes here:  
https://github.com/osresearch/vst/issues/25#issuecomment-702268536

Cut the clk and data traces going to the XY DAC and wire them to SPI1 with a jumper, and route the old Y op amp to the WZ DAC's unused output.  See photos.

## Step 2: Flashing the Firmware
Make sure you have [Visual Studio Code](https://code.visualstudio.com/) with [PlatformIO](https://platformio.org/) installed.

Open the *teensyv* folder in Visual Studio Code and press the upload button to compile and load your teensy.
Note:  This has only been testest with battlezone!

## Step 3: Make sure you get a test pattern to display
Hook up your vst board to your monitor and ensure you see a suitable test pattern displayed.

## Step 4: Compile AdvanceMAME
http://www.advancemame.it/doc-build
