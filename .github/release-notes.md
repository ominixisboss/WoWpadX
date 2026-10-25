First release of **AzerothPad HD**, a fork of [WoWpadX](https://github.com/leoaviana/WoWpadX) for World of Warcraft 3.3.5a HD clients, with a matching ConsolePortLK addon.

> Not tested in-game yet. Please report problems in the Issues tab.

## Downloads
- **AzerothPadHD-win64.zip**: the controller app for Windows x64. Unzip and run `AzerothPadHD.exe`. If Windows says `VCRUNTIME140.dll` is missing, install the Microsoft Visual C++ Redistributable (x64).
- **ConsolePortLK-AzerothPadHD.zip**: the addon. Unzip into `World of Warcraft\Interface\AddOns`, replacing existing `ConsolePort*` folders.

Some antivirus programs (e.g. Avast "IDP.Generic") flag the unsigned exe because it sends keyboard/mouse input and reads the game window. Add an exception for the folder.

## AzerothPad HD (app)
- Pixel Bridge works on HD clients (DXVK, high resolution, DPI scaling) and stays synced when you alt-tab
- PS5: L3/R3 and the left/right halves of the touchpad can be separate bindable buttons
- DSX (DualSenseX) support: uses DSX's virtual controller (set DSX to DualShock 4 for the touchpad)
- Reorganised settings, tidier Pixel Bridge page, closes fully on exit

## ConsolePortLK (addon)
- Pixel Bridge fix for HD clients (on by default)
- From modern ConsolePort: PS5 and Xbox binding presets, Crossbar: Triggers layout, nested rings, pet actions and party targets in rings, rotating cursor arrow, Nudge, crosshair, spell menu, on-demand and whole-screen cursor modes
- Settings search, controller test (`/cptest`), clearer calibration for grip buttons

Credits: [WoWpadX](https://github.com/leoaviana/WoWpadX) and [ConsolePortLK](https://github.com/leoaviana/ConsolePortLK) by leoaviana; ported features from [ConsolePort](https://github.com/seblindfors/ConsolePort) by Sebastian Lindfors (Artistic License 2.0).
