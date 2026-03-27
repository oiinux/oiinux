# 🐱 Oiinux

> A Linux-based operating system built from scratch. Named after the oiia cat.

## Project Structure

```
oiinux/
├── toolchain/      # Cross-compilation toolchain (GCC, binutils, musl)
├── kernel/         # Linux kernel source + our config
├── userspace/      # All userspace packages (built from source)
├── init/           # Oiinux init system (PID 1)
├── rootfs/         # The assembled root filesystem
├── iso/            # Final bootable ISO output
├── scripts/        # Build automation scripts
│   ├── 00-setup.sh
│   ├── 01-toolchain.sh
│   └── 02-kernel.sh
├── packages/       # Package build definitions (.oii format, someday)
└── branding/       # Oiia cat artwork, boot splash, wallpapers
```

## Build Order

1. `./scripts/00-setup.sh`   — set up directories and env vars
2. `./scripts/01-toolchain.sh` — build cross-compiler + musl libc
3. `./scripts/02-kernel.sh`  — configure and compile the kernel
4. (more scripts coming as the project grows)

## Requirements

- x86_64 Linux build host (Arch or Debian/Ubuntu recommended)
- At minimum 8GB RAM (more = faster builds)
- At minimum 50GB free disk space (100GB+ recommended)
- Packages: `build-essential`, `bison`, `flex`, `libssl-dev`, `bc`, `git`, `wget`, `xz-utils`, `python3`, `libelf-dev`, `libncurses-dev`

## Philosophy

- **Everything from source.** No prebuilt binaries.
- **musl libc** over glibc. Cleaner, smaller, ours.
- **We own it.** If it runs on Oiinux, we built it.
- **oiia. 🐱**

## Targets

- [x] Project scaffold
- [ ] Working cross-compilation toolchain
- [ ] Bootable kernel (says hello)
- [ ] Minimal root filesystem
- [ ] Shell login
- [ ] Init system
- [ ] Package manager
- [ ] Networking
- [ ] Graphical interface
- [ ] oiia cat boot animation
- [ ] World domination

## Team

- your name here
- your friend's name here

---

*"from the kernel up, for the oiia cat"*
