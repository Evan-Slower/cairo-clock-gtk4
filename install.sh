#!/usr/bin/env bash
set -e
meson setup builddir --prefix=/usr/local --wipe
ninja -C builddir
sudo ninja -C builddir install
echo "Done."
