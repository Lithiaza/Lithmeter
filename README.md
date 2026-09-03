<div align="center">

<img src=".github/logo.png" width="96" alt="LithMeter">

# LithMeter

**A real-time DPS meter and combat overlay for [SoulWorker](https://store.steampowered.com/app/1377580/Soulworker/).**

Live damage tables, per-player breakdowns, skill tracking, buff uptime and combat history in a compact stacked overlay.

![Platform](https://img.shields.io/badge/platform-Windows%20x64-0078D6)
![Language](https://img.shields.io/badge/C%2B%2B-20-00599C)
![UI](https://img.shields.io/badge/UI-ImGui%20%2B%20DirectX%2011-5C2D91)
![Version](https://img.shields.io/badge/version-1.7.1.29-brightgreen)

</div>

---

## Ready to run

The repository includes a checked-in `release/` folder. End users do **not** need a command prompt, injector or separate build step:

1. Download or clone this repository.
2. Keep every file in `release/` together.
3. Run `release/SoulMeter.exe` as Administrator before starting SoulWorker.

The meter waits for SoulWorker, attaches automatically, and creates `option.xml` and `imgui.ini` beside the executable after the first launch. Those files are intentionally not included in the repository because they are user-specific settings.

For a smaller download, use the `LithMeter-v1.7.1.29-ReadyToRun.zip` release archive. It contains the same runtime payload without the source tree.

### Included runtime files

```text
release/
├── SoulMeter.exe
├── SoulMeterHook.dll
├── sqlite3.dll
├── SWDB.db
└── Lang/
    ├── en.json
    ├── jp.json
    ├── kr.json
    └── zh_tw.json
```

## Features

- Compact stacked-only meter with a scrollbar for long tables.
- Persistent `Rows...` selections with **Show all** and **Deselect all**.
- Per-character skill dropdowns, add/remove skill blocks, live hits and crits.
- `S.Hit/Cast` reports hits for the most recently observed cast, not a lifetime average.
- Solid mode or a left-to-right opacity slider (0.00 transparent to 1.00 dark).
- Resizable edges and corners with normal Windows resize cursors.
- English, Japanese, Korean and Traditional Chinese language files.
- `@Lithiaza` branding and version shown in the meter title bar.

## Controls

| Action | Default key / gesture |
|---|---|
| Show or hide overlay | `Ctrl` + `End` |
| Reset current run | `Ctrl` + `Del` |
| Open the feature menu | Right-click the title bar |
| Open player details | Left-click a character name |
| Resize the meter | Drag any edge or corner |

## Building from source

Building is only needed for contributors. Use **Visual Studio 2022** with the v143 C++ toolset and a Windows 10/11 SDK. Open `Soulworker Utility.sln`, select **Release** and **x64**, then build. `BUILD_EXE.cmd` is an optional developer convenience script; it is not needed by users of the included release build.

The solution builds the meter and its capture hook, then copies the database and language files beside the executable. Do not commit Visual Studio output, PDBs or personal `option.xml` / `imgui.ini` files.

## Attribution

LithMeter is an unofficial community fork/custom UI. It builds on the SoulMeter work of:

| Contributor | Credit |
|---|---|
| **[AFNGP / AFN](https://github.com/AFNGP/SoulMeter)** | Upstream SoulMeter maintenance and feature work |
| **Rainy** | Earlier SoulMeter maintenance and build/UI work this fork builds on |
| **[FeAr](https://github.com/fearek/DPSMeter/)** | Global-server meter lineage |
| **[Park3740](https://github.com/Park3740/SoulMeter)** | Original SoulMeter project |
| **[Lithiaza](https://github.com/0xarray/SoulMeter-1)** | This custom stacked UI, skill tracking and release packaging |

Please keep this attribution and the included `LICENSE.txt` when redistributing the source. Third-party notices remain in their respective directories under `Soulworker Utility/Third Party/`.

## Safety and use

SoulMeter observes SoulWorker client traffic and injects its capture hook into the game process. Use it only where permitted by the game publisher and server rules. It is provided **as-is and at your own risk**; do not discuss or advertise meter use in public in-game chat if that violates local rules.

SoulWorker, its assets and trademarks belong to their respective owners. LithMeter is not affiliated with or endorsed by the game publisher.

