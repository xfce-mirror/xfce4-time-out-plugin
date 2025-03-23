plugin_sources = [
  'time-out-countdown.c',
  'time-out-countdown.h',
  'time-out-fadeout.c',
  'time-out-fadeout.h',
  'time-out-lock-screen.c',
  'time-out-lock-screen.h',
  'time-out.c',
  'time-out.h',
  xfce_revision_h,
]

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

plugin_lib = shared_module(
  'time-out',
  plugin_sources,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-time-out-plugin'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    glib,
    gtk,
    libxfce4panel,
    libxfce4ui,
    libxfce4util,
    x11,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / plugin_install_subdir,
)

i18n.merge_file(
  input: 'xfce4-time-out-plugin.desktop.in',
  output: 'xfce4-time-out-plugin.desktop',
  po_dir: '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
