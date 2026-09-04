LithMeter developer notes
=========================

Version 1.7.1.30

This source package contains the custom stacked meter UI and the runtime files
published in the release/ directory. The ready-to-run release does not require
BUILD_EXE.cmd or any other command prompt step.

Custom UI work
--------------
- Stacked-only meter with compact defaults and a right-side scrollbar.
- Rows... selections persist in option.xml and stay synchronized.
- Skill dropdowns, add/remove skill blocks, live skill hits and S.Hit/Cast.
- Solid mode plus a whole-meter opacity slider from transparent to dark.
- Language reload, @Lithiaza branding, and edge/corner resizing.
- Legacy social-menu code removed from the public build.
- Clean installs now start with only the solid meter visible; demo data and
  the Options window are opened only when the user requests them.

Developer build
---------------
Open "Soulworker Utility.sln" in Visual Studio 2022, select Release/x64,
and build. Command scripts are intentionally omitted; end users should use
the ready-to-run release package.

When packaging a release, include only the executable, hook DLL, sqlite3.dll,
SWDB.db, Lang/*.json and the clean default release/option.xml. Never publish
personal option.xml files, imgui.ini, PDBs, object files, or Visual Studio
cache/build logs.
