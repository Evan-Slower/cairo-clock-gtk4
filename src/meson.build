# src/meson.build

# Define the X11 macro for clangd and the compiler
#add_project_arguments('-DGDK_WINDOWING_X11', language: 'c')

# Define sources in this directory
# Added license_data.c and callbacks.c to the list
sources = files('cairo-clock-gtk4.c')

# Define include directories relative to THIS file (src/)
# '.' means the src/ directory itself
# '..' means the project root (where config.h is generated)
src_inc = include_directories('.')
root_inc = include_directories('..')

executable('cairo-clock-gtk4',
  sources,
  resources,
  dependencies: [gtk4_dep, gtk4_x11_dep, rsvg_dep, x11_dep, libm_dep],
  include_directories: [src_inc, root_inc],
  install: true
)
