Yakuake is a drop-down terminal emulator based on KDE Konsole technology.


It's a KDE Extragear application released under GPL v2, GPL v3 or any later
version accepted by the membership of KDE e.V.

The current maintainer of Yakuake is Eike Hein <hein@kde.org>.


The Yakuake website is located at https://kde.org/applications/system/org.kde.yakuake

Report bugs and wishes at https://bugs.kde.org/

You can browse the latest and older sources at https://invent.kde.org/utilities/yakuake


### Basic build and installation instructions:

1. Download the source code or clone this repository
2. `cd` to the source code folder
3. `mkdir build`
4. `cd build`
5. `cmake ../` - defaults to `/usr/local` as installation path on UNIX ([docs](https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.html)), optionally use `-DCMAKE_INSTALL_PREFIX=<path to install to>`
6. `make`
7. `sudo make install`

To remove use `sudo make uninstall`

For more, please see the KDE Techbase wiki (https://techbase.kde.org/) and
the CMake documentation (at https://www.cmake.org/).
