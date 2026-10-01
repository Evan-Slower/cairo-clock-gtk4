#!/bin/sh
set -e

PREFIX=/usr/local

rm -f "$PREFIX/bin/cairo-clock-gtk4"
rm -f "$PREFIX/share/applications/cairo-clock-gtk4.desktop"
rm -f "$PREFIX/share/icons/hicolor/256x256/apps/cairo-clock-gtk4-logo.png"
rm -rf "$PREFIX/share/cairo-clock"
rm -f "$PREFIX/share/locale"/*/LC_MESSAGES/cairo-clock-gtk4.mo
rm -f "$PREFIX/share/glib-2.0/schemas/org.gnome.cairo-clock.gschema.xml"

glib-compile-schemas "$PREFIX/share/glib-2.0/schemas" 2>/dev/null || true
gtk-update-icon-cache "$PREFIX/share/icons/hicolor/" 2>/dev/null || true
update-desktop-database "$PREFIX/share/applications/" 2>/dev/null || true

echo "cairo-clock-gtk4 uninstalled."
