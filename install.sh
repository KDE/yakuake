#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"

mode="local"
build_type="Release"
build_dir="build"
prefix=""
with_x11="ON"
jobs="$(nproc)"
skip_install=0

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Build and install Yakuake from this checkout.

Modes:
  --mode local         Install into ~/.local (default)
  --mode system        Install into the default system prefix using sudo
  --mode custom        Install into a custom prefix set with --prefix

Build options:
  --build-type TYPE    CMake build type: Release, Debug, RelWithDebInfo, MinSizeRel
  --build-dir DIR      Build directory to use (default: build)
  --jobs N             Parallel build jobs (default: nproc)
  --with-x11           Build with X11 integration (default)
  --wayland-only       Disable X11 integration
  --skip-install       Configure and build, but do not install

Install options:
  --prefix DIR         Install prefix for --mode custom, or override the mode default

Other:
  -h, --help           Show this help text

Examples:
  $(basename "$0")
  $(basename "$0") --mode system
  $(basename "$0") --mode custom --prefix /opt/yakuake
  $(basename "$0") --build-type Debug --skip-install
  $(basename "$0") --wayland-only --mode local
EOF
}

die() {
  printf 'Error: %s\n' "$1" >&2
  exit 1
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --mode)
      [[ $# -ge 2 ]] || die "--mode requires a value"
      mode="$2"
      shift 2
      ;;
    --build-type)
      [[ $# -ge 2 ]] || die "--build-type requires a value"
      build_type="$2"
      shift 2
      ;;
    --build-dir)
      [[ $# -ge 2 ]] || die "--build-dir requires a value"
      build_dir="$2"
      shift 2
      ;;
    --jobs)
      [[ $# -ge 2 ]] || die "--jobs requires a value"
      jobs="$2"
      shift 2
      ;;
    --prefix)
      [[ $# -ge 2 ]] || die "--prefix requires a value"
      prefix="$2"
      shift 2
      ;;
    --with-x11)
      with_x11="ON"
      shift
      ;;
    --wayland-only)
      with_x11="OFF"
      shift
      ;;
    --skip-install)
      skip_install=1
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

case "$mode" in
  local)
    default_prefix="$HOME/.local"
    ;;
  system)
    default_prefix=""
    ;;
  custom)
    default_prefix=""
    [[ -n "$prefix" ]] || die "--mode custom requires --prefix"
    ;;
  *)
    die "invalid mode: $mode (expected local, system, or custom)"
    ;;
esac

if [[ -z "$prefix" ]]; then
  prefix="$default_prefix"
fi

cmake_args=(
  -S "$script_dir"
  -B "$script_dir/$build_dir"
  -DCMAKE_BUILD_TYPE="$build_type"
  -DWITH_X11="$with_x11"
)

if [[ -n "$prefix" ]]; then
  cmake_args+=( -DCMAKE_INSTALL_PREFIX="$prefix" )
fi

printf 'Configuring with mode=%s, build_type=%s, build_dir=%s, WITH_X11=%s\n' \
  "$mode" "$build_type" "$build_dir" "$with_x11"
if [[ -n "$prefix" ]]; then
  printf 'Install prefix: %s\n' "$prefix"
else
  printf 'Install prefix: default system prefix\n'
fi

cmake "${cmake_args[@]}"
cmake --build "$script_dir/$build_dir" -j"$jobs"

if [[ "$skip_install" -eq 1 ]]; then
  printf 'Skipping install as requested.\n'
  exit 0
fi

if [[ "$mode" == "system" ]]; then
  sudo cmake --install "$script_dir/$build_dir"
else
  cmake --install "$script_dir/$build_dir"
fi