
# 32x Game Starter Kit
Game Starter Kit for the 32x.  Has stripped down components from the D32xr repo and with a demo game.  If you are wanting to start your 32x game development journey you can start here.

![ThanksToChilly](https://github.com/FrankenwareStudios/32xGameStarterKit/assets/133242361/de1263cd-cf49-45da-a724-2bdc084296e7)

![GameTitle](https://github.com/FrankenwareStudios/32xGameStarterKit/assets/133242361/ea81977c-2e86-4ef0-81f6-76440d65f91d)

![WorldMap](https://github.com/FrankenwareStudios/32xGameStarterKit/assets/133242361/637f40b1-fe58-498f-9f8f-5049d458b55d)

![BattleScene](https://github.com/FrankenwareStudios/32xGameStarterKit/assets/133242361/e8c733fe-f8dc-42ee-8308-cdc1826839a9)




Features
============
- works on real hardware
- supports Vic's Yatssd!  So you can draw your maps/levels with Tiled and plug them in
- Animated sprite helpers (each state can have its own sprite speed).  For example, one sprite may have 8 different states or animations with 2 frames in each animation.  One can be at 10 tics and the others at various other tics.
- .bmp files for graphics only supported.  BE SURE to use the same pallatte for all .bmp files
- Assets stored within the assets folder and all assets are managed within the assets.s file.  The assets are simple (.bmp file graphics, .wave for sfx, and .zgm for music) no need for extra .pal files
- demonstrates splashscreens and title screens with fading
- demonstrates warping/transfering from one map to another.
- demonstrates dialog options with customizable font treated as a sprite (so you can have custom fonts)
- demonstrates a simple battle sequence (battle AI logic still would need to be created)
- AI example of NPC roaming the world and obeying collision detections
- collision detection: extended yatssd to have a collision layer that you can create within Tiled
- Music and Sound (must use zgm files for music and .wav at 8 bit mono 11kHz-22kHz)
- Organized coding structure were most objects can be defined just withi gameworld.h file
- place holder for items to be added soon
- place holder for quest/story line to be added soon 
- NOTE: Code is not optimized and may contain some warnings

Upcoming
============
- adding a GameEditor for the gameworld.h file that way you don't have to hand code in values
- add examples for items
- add examples for quests/story

Tiled plugins
============

The Tools\Tiled-Yatssd Extension directory contains Javascript plugins for exporting Tilemaps and Tilesets in YATSSD format with the extended collision layer added.  Please be aware this extension is not officially supported by Vic.  Refer to Vic's Yatssd GitHub for current releases and extensions he might add himself.  https://github.com/viciious/yatssd

The plugins require Tiled version 1.5 or newer.

[More information on Tiled plugins.](https://doc.mapeditor.org/en/stable/reference/scripting/)

Building
============

**Prerequisites**

A posix system and Chilly Willy's latest DevKit from August 2022 found here - > (http://gendev.spritesmind.net/forum/viewtopic.php?f=7&t=3024) is requred to build this Game Kit.  This does build on Windows WSL2 (Infact this entire game kit was developed within it).  Current version built with this Game Kit was 12.1.0


If you're using Windows 10/11, you must also [install WSL 2](https://docs.docker.com/desktop/windows/wsl/).


License
============
If a source file does not have a license header stating otherwise, then it is covered by the MIT license.

The game kit uses graphical assets from Shining Force 1 and 2 which can be found: https://www.spriters-resource.com/genesis_32x_scd/sf1/

Credits
============

New Original code introduced by Matthew Nimmo at FrankenwareStudios LLC

D32xr code by Victor Luchitz and Joseph Fenton (Vic & Chilly Willy)

Yatssd code by Victor Luchitz aka Vic

32X devkit by Joseph Fenton aka Chilly Willy
