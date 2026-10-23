# Real Expression for Skyrim VR

[Nexus Mods page](https://www.nexusmods.com/skyrimspecialedition/mods/196445)

## Description

This is an AI coded plugin which receives OSC face-tracking data from a supported VR headset - e.g. Quest Pro
(tested), XR Elite (untested), VIVE tracker (untested) and applies that data to your player character in
Skyrim VR. Not really useful for most people since Skyrim is a single player game, but maybe interesting in
conjunction to the Skyrim VR multiplayer mods currently in the works. The Quest Pro is tested connecting
through Steam Link in the following video:

[![Real Expression for Skyrim VR demo](https://img.youtube.com/vi/0gvsQzCKxes/hqdefault.jpg)](https://www.youtube.com/watch?v=0gvsQzCKxes)

Be sure to enable tracking data in your VR streaming application (for Steam Link, see below).

## Installation instructions

Install this as usual for an SKSE plugin. In Steam Link, enable face tracking pass-through as follows:

![Steam Link face tracking settings](https://staticdelivery.nexusmods.com/mods/1704/images/196445/196445-1792717739-732279081.png)

Set the output port in the included plugin's .ini file (the default port is 9000). You can also adjust the
scale factor for each of the mappings between the OSC addresses and the Skyrim mfg phonemes.

## Main features

Currently the following facegen modifiers, phonemes, and moods are mapped:

### Eyes

| Modifier | OSC parameter(s) |
|---|---|
| BlinkLeft | `EyesClosedL` |
| BlinkRight | `EyesClosedR` |
| SquintLeft | `LidTightenerL` |
| SquintRight | `LidTightenerR` |
| LookDown | `EyesLookDownL`, `EyesLookDownR` |
| LookLeft | `EyesLookLeftL`, `EyesLookLeftR` |
| LookRight | `EyesLookRightL`, `EyesLookRightR` |
| LookUp | `EyesLookUpL`, `EyesLookUpR` |

### Brows

| Modifier | OSC parameter(s) |
|---|---|
| BrowDownLeft | `BrowLowererL` |
| BrowDownRight | `BrowLowererR` |
| BrowUpLeft | `InnerBrowRaiserL`, `OuterBrowRaiserL` |
| BrowUpRight | `InnerBrowRaiserR`, `OuterBrowRaiserR` |

### Phonemes: confident mappings (mouth)

| Phoneme | OSC parameter(s) |
|---|---|
| Aah | `JawDrop` |
| BMP | `LipPressorL`, `LipPressorR` |
| OohQ | `LipPuckerL`, `LipPuckerR` |
| DST | `TongueTipAlveolar`, `ToungeTipAlveolar` |
| Th | `TongueTipInterdental`, `ToungeTipInterdental` |
| K | `BackDorsalVelar` |

### Phonemes: guesses

Each can be turned off in the ini with `bEnable<Name>`.

| Phoneme | OSC parameter(s) |
|---|---|
| BigAah | `JawDrop` (same as Aah) |
| ChJSh | `FrontDorsalPalate` |
| Eee | `LipStretcherL`, `LipStretcherR` |
| FV | `LipSuckLB`, `LipSuckRB` |
| N | `TongueTipAlveolar`, `ToungeTipAlveolar` (same as DST) |
| Oh | `LipFunnelerLB`, `LipFunnelerLT`, `LipFunnelerRB`, `LipFunnelerRT` |
| W | `LipPuckerL`, `LipPuckerR` (same as OohQ) |

**Not mapped:** Eh, I, R. The game controls these phonemes.

A few notes:

- Steam Link sends tongue values under both spellings, "Tongue" and "Tounge", so the code reads both.
- Some phonemes share sources, so they always move together: Aah and BigAah, DST and N, OohQ and W.
- MoodHappy, MoodSad, MoodDisgusted are also attempted to be detected from the face-tracking data. If you
  have ideas on how to detect the other moods from headset data (e.g. MoodAnger, MoodSad) then let me know.

## Requirements

Skyrim VR with a headset which does face-tracking. Only the Quest Pro is tested, as it is the most accessible
(inexpensive) headset which has full face tracking (the Steam Frame only has eye-tracking). Feel free to build
from scratch, as the code is available at <https://github.com/ZouljaBoyTellem/RealExpressionVR> (this is a new
GitHub account I'm using for vibe coded Skyrim VR stuff, so as to not pollute my regular GitHub, but feel free
to build from scratch if you don't trust the code).

Runtime dependencies:

- SKSEVR
- VR Address Library for SKSEVR

Logs go to `Documents/My Games/Skyrim VR/SKSE/RealExpressionVR.log`.

## Shout outs

Code generated with Claude Opus 5.5. PC in video is using the
[Japhet Preset](https://www.nexusmods.com/skyrimspecialedition/mods/167408). Hair is
[HG Hairdos SMP (Male)](https://www.nexusmods.com/skyrimspecialedition/mods/167408). Outfit is part of the
[BDOR Male Collection (2024) by Kirax](https://modbooru.com/mods/bdor-2024-male-collection-by-kirax.1886).

## Building from source

### Requirements

- Visual Studio 2022+ with the "Desktop development with C++" workload
- CMake 3.21+ and Ninja
- [vcpkg](https://github.com/microsoft/vcpkg) with `VCPKG_ROOT` set

Built on [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR/tree/ng) (alandtse's maintained `ng`
branch).

### Getting started

```powershell
git clone --recursive https://github.com/ZouljaBoyTellem/RealExpressionVR
# or, in an existing clone:
git submodule update --init --recursive
```

Build from a **x64 Native Tools / Developer PowerShell** prompt:

```powershell
cmake --preset release
cmake --build --preset release
```

The output is `build/release/RealExpressionVR.dll`. The first build compiles CommonLib from source and
takes 10–20 minutes. Later builds are incremental.

To copy the DLL straight into a mod folder after each build, set `OUTPUT_FOLDER`:

```powershell
cmake --preset release -DOUTPUT_FOLDER="D:/MO2/mods/RealExpressionVR"
```

You can also open the folder directly in Visual Studio, which picks up `CMakePresets.json`.

### Updating CommonLib

```powershell
git -C extern/CommonLibSSE-NG pull origin ng
git add extern/CommonLibSSE-NG
```

If the new version needs newer packages, copy the `baseline` from
`extern/CommonLibSSE-NG/vcpkg-configuration.json` into this project's `vcpkg-configuration.json`.
