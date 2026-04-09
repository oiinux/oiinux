
# oiinux 

> sigma self developed (may or may not be vibecoded) oiia themed linux os

oiinux is a custom linux distro built from scratch. custom kernel, custom init, custom installer, the whole thing. oiia oiia oiia.

## what it is

- custom linux kernel 6.9.7

- musl libc based (no glibc slop)

- busybox userland

- apk package manager (alpine packages)

- ncurses TUI installer (arch-style)

- xorg + KDE plasma (or whatever DE you want)

- boots in QEMU, installs to real disk

## status

**v0.1.0** — boots, installs, runs. still cooking.

- [x] kernel boots

- [x] networking

- [x] TUI installer with DE selection

- [x] KDE plasma

- [ ] custom oiinux DE (wip)

- [ ] custom bootloader

- [ ] release ISO

## building

```bash

source ~/oiinux/oiinux-env.sh

# compile init

x86_64-oiinux-linux-musl-gcc -static -O2 -o rootfs/init init/init.c

# repack initramfs

cd rootfs && sudo find . | sudo cpio -oH newc | gzip > ../iso/staging/boot/initramfs.cpio.gz

# build ISO

grub-mkrescue -o oiinux-0.1.0.iso iso/staging -- -volid OIINUX

```

## running

```bash

qemu-system-x86_64 -cdrom oiinux-0.1.0.iso -m 2G \

  -netdev user,id=net0 -device e1000,netdev=net0 \

  -vga virtio -display gtk

```

## license

GPL v3 — you can look, you can learn, you cannot steal.

## credits

built by [@oiinux](https://github.com/oiinux) with way too much patience and not enough sleep. (tysm claude)

oiia oiia oiia 

