project('oiinux-terminal', 'c',
  default_options: ['c_std=c11'])

gtk4 = dependency('gtk4')
vte = dependency('vte-2.91-gtk4')

executable('oiinux-terminal', 'terminal.c',
  dependencies: [gtk4, vte])
