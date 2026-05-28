# SFM PBR

Plugin that adds a PBR Shader to Source Filmmaker.
This Shader is a *refactored* Version of [sfm_pbr](https://github.com/ficool2/sfm_pbr) by Ficool2
Which is a modified Version of the PBR Shader found in [Zombie Master: Reborn](https://github.com/zm-reborn)
Which is a modified Version of the PBR Shader called [source-pbr](https://github.com/thexa4/source-pbr) made by Thexa4

This Repository uses [LUX](https://github.com/LUX-Shaders-Team/LUX-Shaders) as it's Foundation, see Installation Intructions for more Details.

## Requirements

- Visual Studio 2022 or newer
- Windows
- A local copy of [LUX](https://github.com/LUX-Shaders-Team/LUX-Shaders)

## Installation Instructions

The Shader on this Repository uses C++ and HLSL Code from [LUX](https://github.com/LUX-Shaders-Team/LUX-Shaders)
There isn't a solution for submoduling this yet!
To use LUX with this Repository, Merge the MaterialSystem Folder from LUX with the one from this Repository.

Currently there is no Script for generating a fresh Solution File.
For now here is some Instructions for making this work :

After opening the Solution, right click the Project ( `shirodkxtro2_pbr` ) and navigate to the `Properties`.
For both the Release and Debug `Configuration:`, set the `Output Directory` ( full Path including Drive Letter ) for the `.dll`.
I suggest `src/game/usermod/`

Other Files ( including those from LUX ) should already be referenced in the Solution File.
If you are recreating the Solution, ensure no other C++ Files from `materialsystem/stdshaders/shadersource/` ( or it's Subfolders ) are included 
This Repository is only for PBR not LUX Shaders themselves.
Including these Shaders will cause Conflicts later down the Line with the LUX Shaders Addon

Next, open `materialsystem/stdshaders/lux_common_defines.h`
Uncomment `#define SFM_COMPATIBILITY`, and comment the other ones ( SDK2013SP, TF2SDK ) 

Next, edit `ShadersBuildDirectories.bat`.
Change the `targetdir` to `..\..\..\game\usermod\shaders`
Change the `GAME_DIR` to `..\..\..\game\usermod`

## Build Instructions

Run `materialsystem/stdshaders/!Compile_PBR.bat` to build the Shaders `.vcs` and `.inc` Files.
If there isn't red Text, you did everything correctly to build the Shaders.
If there is red Text, something is wrong. This could be just about anything.
Place the compiled FXC Files ( `shaders/fxc/*.vcs` ) into SFM's `shaders/fxc/` Folder.
The Full Path for this should be `../workshop/shaders/fxc/`

After `.inc` have been built, the `.dll` may be compiled from the `.sln` File.

To build the plugin, open the .sln in Visual Studio 2022 or newer and build.
Place the compiled DLL into SFM's `addons` folder.
The Full Path for this should be `../workshop/addons/`

## Legal

LUX is licensed under the `SOURCE 1 SDK LICENSE`, which can be found in the LICENSE File at the root of this Repository.
Proper Attribution must be included when using or redistributing any Part of its Codebase.
The required Attribution ( from LUX & Valve ) are provided in `thirdpartylegalnotices.txt`, any additional Credits are always appreciated.

The PBR Shader uses [Naughty Dog's](https://advances.realtimerendering.com/other/2016/naughty_dog/NaughtyDog_TechArt_Final.pdf) Microshadows.
SFM Black-Box Maps don't have indirect Lighting, Microshadows are a perfect Solution for applying SFM's SSAO to Direct Lightsources in a realistic Manner.

## Original Description

Check out the [Steam Workshop page](https://steamcommunity.com/sharedfiles/filedetails/?id=3671463307)!

Compared to ZMR's implementation, there is additional fixes for SFM compatibility, as well as new additions such as MRAO factor parameters.

This repository is a stripped version of the [Alien Swarm SDK](https://github.com/Nican/swarm-sdk).