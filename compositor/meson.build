project('oiinux-compositor', 'c',
  default_options: ['c_std=c11'])

wlroots = dependency('wlroots-0.18')
wayland = dependency('wayland-server')
wayland_protocols = dependency('wayland-protocols')
wayland_scanner = find_program('wayland-scanner')

xdg_shell_xml = '/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml'

xdg_shell_h = custom_target('xdg-shell-protocol.h',
  output: 'xdg-shell-protocol.h',
  input: xdg_shell_xml,
  command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'])

xdg_shell_c = custom_target('xdg-shell-protocol.c',
  output: 'xdg-shell-protocol.c',
  input: xdg_shell_xml,
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'])

executable('oiinux-compositor',
  ['main.c', xdg_shell_c, xdg_shell_h],
  dependencies: [wlroots, wayland])
