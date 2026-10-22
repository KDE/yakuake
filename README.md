Yakuake: session save/load (fork changes)
==========================================

This fork adds session management to Yakuake: you can save your whole
workspace (tabs, split layouts, working directories and even the programs
that were running) to a file and bring it back later.

NEW FEATURES
------------

* Save Session...
  Main menu -> "Sessions" section -> "Save Session...". Stores every open tab
  to a JSON file (default folder: ~/.config/yakuake/sessions/, ".json" is
  appended automatically, you get an overwrite warning for existing files).
  What is saved:
    - all tabs, in their current order and which one was active,
    - the split layout of each tab (horizontal/vertical splits and their
      relative sizes),
    - the working directory of every terminal,
    - the command line of the program running in each terminal (if any).

* Load Session...
  Main menu -> "Sessions" section -> "Load Session...". Recreates the saved
  tabs, splits and working directories, and re-activates the tab that was
  active when you saved.

* Add or Replace
  When you load a session while other tabs are open, Yakuake asks whether to
  "Add" the loaded tabs to the open ones or "Replace" them.

* Restoring running programs
  If the saved session contained running programs (e.g. htop, vim, a
  long-running script), a "Restore Running Programs" dialog lets you choose,
  per program:
    - Run                       - start it right away,
    - Only place in command line - type the command but don't press Enter,
    - Do not restore            - skip it.
  Closing the dialog restores none of them, so nothing is executed by accident.

* Save only selected tabs (Ctrl + left click)
  Ctrl + left click on tabs marks them (highlighted in the tab bar), like in
  a browser; the currently open tab becomes part of the selection when you
  start. "Save Session..." then saves only the marked tabs and the dialog
  title shows how many ("Save Session (3 selected tabs)"). A normal click
  clears the selection. With no marked tabs, everything is saved.

Commits covered: 519a17b, 1b1df43, b5e13c3, 8a126d8, 2ae86db (HEAD).


BUILD AND INSTALL INTO A LOCAL FOLDER (no root needed)
-------------------------------------------------------

Requirements: a C++ compiler, CMake, Extra CMake Modules and the KDE
Frameworks 6 / Qt 6 development packages. On Fedora, roughly:

  sudo dnf builddep yakuake        # pulls in all build dependencies
  # or on Debian/Ubuntu: sudo apt build-dep yakuake

Build and install into ~/.local/yakuake-custom (any folder you like):

  git clone <this repository> yakuake
  cd yakuake
  mkdir build && cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release \
           -DCMAKE_INSTALL_PREFIX=$HOME/.local/yakuake-custom
  make -j"$(nproc)"
  make install                     # no sudo, it's your home folder

Run it (close any running stock Yakuake first):

  $HOME/.local/yakuake-custom/bin/yakuake

If it can't find its skins/resources, point it to the prefix first:

  export XDG_DATA_DIRS=$HOME/.local/yakuake-custom/share:/usr/local/share:/usr/share
  export QT_PLUGIN_PATH=$HOME/.local/yakuake-custom/lib64/plugins:$QT_PLUGIN_PATH

Optional: add $HOME/.local/yakuake-custom/bin to your PATH, or symlink it:

  ln -s $HOME/.local/yakuake-custom/bin/yakuake ~/.local/bin/yakuake

Uninstall: from the build folder run "make uninstall", or just delete
~/.local/yakuake-custom.
