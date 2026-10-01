project('cairo-clock-gtk4', 'c',
  version: '1.0',
  meson_version: '>= 0.64.0',
  default_options: [
    'c_std=gnu11',
    'b_pie=true',
    'b_lto=false'
  ]
)

i18n = import('i18n')  # ← i18n

#add_project_arguments('-DGDK_WINDOWING_X11', language: 'c')
add_project_arguments('-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()), language: 'c')  # ← i18n
add_project_arguments('-DCAIROCLOCKLOCALEDIR="' + get_option('prefix') / get_option('localedir') + '"', language: 'c')
gnome = import('gnome')

# Compile cairo-clock.gresource.xml into a C source file
resources = gnome.compile_resources('cairo-clock-resources',
                                    'cairo-clock.gresource.xml',
                                    c_name: 'cairo_clock')

cc = meson.get_compiler('c')
libm_dep = cc.find_library('m', required: false)

pkgdata_dir = join_paths(get_option('prefix'), get_option('datadir'), 'cairo-clock')

gtk4_dep = dependency('gtk4')
gtk4_x11_dep = dependency('gtk4-x11')
rsvg_dep = dependency('librsvg-2.0')
x11_dep = dependency('x11', required: true)

# Configuration
config_data = configuration_data()
config_data.set('HAVE_X11', 1)
config_data.set_quoted('DATA_DIR', pkgdata_dir)

configure_file(
  output: 'config.h',
  configuration: config_data
)

# Subdirectories (Order matters: data first, then src)
subdir('data')
subdir('src')
subdir('themes')
subdir('po')  # ← i18n

gnome.post_install(glib_compile_schemas: true)

# 1. Install the .desktop file
install_data('desktop/cairo-clock-gtk4.desktop',
             install_dir: get_option('datadir') / 'applications')

install_data('pixmaps/cairo-clock-gtk4-logo.png',
             install_dir: get_option('datadir') / 'icons' / 'hicolor' / '256x256' / 'apps')
