#!/usr/bin/env bash

set -euo pipefail

with_x11=1
run_update=1
dry_run=0
use_build_dep=1

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Install the Debian/Trixie packages needed to build Yakuake from source.

Options:
  --with-x11           Install X11 build dependencies (default)
  --wayland-only       Skip X11-specific development packages
  --no-update          Do not run apt update before installing packages
  --no-build-dep       Skip apt build-dep yakuake and install explicit packages only
  --dry-run            Print the commands that would run without executing them
  -h, --help           Show this help text

Examples:
  $(basename "$0")
  $(basename "$0") --wayland-only
  $(basename "$0") --dry-run
  $(basename "$0") --no-build-dep --wayland-only
EOF
}

die() {
  printf 'Error: %s\n' "$1" >&2
  exit 1
}

run_cmd() {
  printf '+ %s\n' "$*"
  if [[ "$dry_run" -eq 0 ]]; then
    "$@"
  fi
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --with-x11)
      with_x11=1
      shift
      ;;
    --wayland-only)
      with_x11=0
      shift
      ;;
    --no-update)
      run_update=0
      shift
      ;;
    --no-build-dep)
      use_build_dep=0
      shift
      ;;
    --dry-run)
      dry_run=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "unknown option: $1"
      ;;
  esac
done

if ! command -v apt-get >/dev/null 2>&1; then
  die "apt-get not found; this script currently supports Debian/apt-based systems only"
fi

if [[ "$EUID" -eq 0 ]]; then
  sudo_cmd=()
else
  sudo_cmd=(sudo)
fi

base_packages=(
  build-essential
  cmake
  extra-cmake-modules
  pkg-config
  qt6-base-dev
  qt6-base-private-dev
  qt6-svg-dev
  qt6-wayland-dev
  plasma-wayland-protocols
  kwayland-dev
  libkf6archive-dev
  libkf6colorscheme-dev
  libkf6config-dev
  libkf6configwidgets-dev
  libkf6coreaddons-dev
  libkf6crash-dev
  libkf6dbusaddons-dev
  libkf6globalaccel-dev
  libkf6i18n-dev
  libkf6iconthemes-dev
  libkf6kio-dev
  libkf6newstuff-dev
  libkf6notifications-dev
  libkf6notifyconfig-dev
  libkf6parts-dev
  libkf6statusnotifieritem-dev
  libkf6widgetsaddons-dev
  libkf6windowsystem-dev
  libkf6xmlgui-dev
)

x11_packages=(
  libxcb-randr0-dev
  libxcb-util-dev
)

packages=("${base_packages[@]}")
if [[ "$with_x11" -eq 1 ]]; then
  packages+=("${x11_packages[@]}")
fi

printf 'Preparing Yakuake build dependencies for %s mode.\n' "$( [[ "$with_x11" -eq 1 ]] && printf 'X11 + Wayland' || printf 'Wayland-only' )"

if [[ "$run_update" -eq 1 ]]; then
  run_cmd "${sudo_cmd[@]}" apt-get update
fi

if [[ "$use_build_dep" -eq 1 ]]; then
  if apt-cache showsrc yakuake >/dev/null 2>&1; then
    run_cmd "${sudo_cmd[@]}" apt-get build-dep -y yakuake
  else
    printf 'Source package metadata for yakuake is not available; skipping apt build-dep.\n'
    printf 'Enable deb-src entries if you want apt build-dep support.\n'
  fi
fi

run_cmd "${sudo_cmd[@]}" apt-get install -y "${packages[@]}"

printf 'Dependency setup complete.\n'