# data/meson.build
fs = import('fs')
gnome = import('gnome') # 1. Import the GNOME module

# 2. Copy UI files for local testing (Keep your existing logic)
fs.copyfile('cairo-clock-gtk4.ui')
fs.copyfile('menus.ui')

# 3. Compile schemas LOCALLY (The Automatic Way)
# This replaces your manual custom_target.
# It automatically finds *.gschema.xml in this directory (data/)
# and compiles them to the build directory.
gnome.compile_schemas(
  build_by_default: true,
  depend_files: files('org.gnome.cairo-clock.gschema.xml') # Explicitly track this file
)

# 4. Install the SOURCE XML file to the system directory
# (Required for final installation, separate from local testing)
install_data('org.gnome.cairo-clock.gschema.xml',
             install_dir: join_paths(get_option('datadir'), 'glib-2.0', 'schemas'))

# 5. Install UI files (Optional, keep if your app loads them from install dir)
install_data('cairo-clock-gtk4.ui', 'menus.ui',
             install_dir: join_paths(get_option('datadir'), 'cairo-clock'))
