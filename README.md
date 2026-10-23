# MyPlugin

A blank SKSE plugin that runs on Skyrim SE, AE and VR from a single DLL, built on
[CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR/tree/ng) (alandtse's maintained `ng` branch).

## Requirements

- Visual Studio 2022+ with the "Desktop development with C++" workload
- CMake 3.21+ and Ninja
- [vcpkg](https://github.com/microsoft/vcpkg) with `VCPKG_ROOT` set

## Getting started

```powershell
git clone --recursive <your repo url>
# or, in an existing clone:
git submodule update --init --recursive
```

Build from a **x64 Native Tools / Developer PowerShell** prompt:

```powershell
cmake --preset release
cmake --build --preset release
```

The output is `build/release/MyPlugin.dll`. The first build compiles CommonLib from source and
takes 10–20 minutes. Later builds are incremental.

To copy the DLL straight into a mod folder after each build, set `OUTPUT_FOLDER`:

```powershell
cmake --preset release -DOUTPUT_FOLDER="D:/MO2/mods/MyPlugin"
```

You can also open the folder directly in Visual Studio, which picks up `CMakePresets.json`.

## Renaming the plugin

Change `project(MyPlugin ...)` and `PLUGIN_AUTHOR` in `CMakeLists.txt`. The plugin name,
version and author are generated from there.

## Runtime requirements (for users)

- SKSE64 (SE/AE) or SKSEVR (VR)
- Address Library for SKSE Plugins (SE/AE), or VR Address Library for SKSEVR (VR)

Logs go to `Documents/My Games/Skyrim Special Edition/SKSE/MyPlugin.log` (or the
`Skyrim VR` folder on VR).

## Writing cross-runtime code

- Use `REL::RelocationID(seID, aeID)` for addresses. VR uses the SE IDs through the VR Address Library.
- Use `REL::Module::IsVR()` / `IsAE()` / `IsSE()` for behavior that differs at runtime.
- Use `REL::Relocate(se, ae, vr)` for offsets that differ between runtimes.

## Updating CommonLib

```powershell
git -C extern/CommonLibSSE-NG pull origin ng
git add extern/CommonLibSSE-NG
```

If the new version needs newer packages, copy the `baseline` from
`extern/CommonLibSSE-NG/vcpkg-configuration.json` into this project's `vcpkg-configuration.json`.
