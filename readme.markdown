# OpenMW Mod Manager

This project aims to provide a GUI-driven tool for managing and detecting conflicts with mods for the open source re-implementation of *The Elder Scrolls III: Morrowind*, [OpenMW](https://openmw.org).

Key features include:

* A simple user-interface that displays data repositories scattered across various locations.
* The ability to disable/enable data repositories with a quick click.
* The ability to quickly add data repositories through the native file system.
* Recognition of mod sub-components for complicated data.
* Conflict detection, to show how the order of data repositories matters.

Planned features include:

* Detailed conflict reporting.
* Enabling/disabling content without using the OpenMW launcher.
* Enabling/disabling BSAs without using the OpenMW launcher.
* Updating conflict detection to compare to BSAs.
* Interfaces to other tools.

This tool was inspired by [Wrye Mash](http://www.uesp.net/wiki/Tes3Mod:Wrye_Mash) and [Mod Organizer](https://github.com/TanninOne/modorganizer).

## Source

This tool suffers from major spaghetti code, as it was written as an introduction to Qt. I'm sorry.
