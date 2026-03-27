#!/usr/bin/env bash
# =============================================================================
# Oiinux :: 02-kernel.sh
#
# Downloads, configures, and compiles the Linux kernel for Oiinux.
# Uses our cross-compiler from 01-toolchain.sh.
#
# Run AFTER: source oiinux-env.sh && ./scripts/01-toolchain.sh
# =============================================================================

set -euo pipefail

# ── Verify env ─────────────────────────────────────────────────────────────────
: "${OIINUX_SRC:?Run: source oiinux-env.sh first}"
: "${OIINUX_TOOLS:?Run: source oiinux-env.sh first}"
: "${OIINUX_TARGET:?Run: source oiinux-env.sh first}"
: "${OIINUX_JOBS:?Run: source oiinux-env.sh first}"

RED='\033[0;31m'; GRN='\033[0;32m'; YLW='\033[1;33m'
CYN='\033[0;36m'; BLD='\033[1m'; RST='\033[0m'

oiia() { echo -e "${CYN}${BLD}[oiinux]${RST} $*"; }
step() { echo -e "\n${BLD}${YLW}━━━ $* ━━━${RST}\n"; }
ok()   { echo -e "${GRN}[  OK  ]${RST} $*"; }
die()  { echo -e "${RED}[ FAIL ]${RST} $*"; exit 1; }

LOG_DIR="${OIINUX_SRC}/logs"
mkdir -p "${LOG_DIR}"

run_logged() {
    local label="$1"; shift
    local logfile="${LOG_DIR}/${label}.log"
    oiia "Running: ${label} (log: ${logfile})"
    if ! "$@" &> "${logfile}"; then
        die "${label} FAILED. Check ${logfile}"
    fi
    ok "${label} done."
}

# =============================================================================
# VERSION
# =============================================================================
KERNEL_VER="6.9.7"   # Check https://kernel.org for latest stable
KERNEL_DIR="${OIINUX_SRC}/kernel/src/linux-${KERNEL_VER}"
KERNEL_URL="https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-${KERNEL_VER}.tar.xz"
OIINUX_KCONFIG="${OIINUX_SRC}/kernel/config/oiinux.config"

export PATH="${OIINUX_TOOLS}/bin:${PATH}"
export ARCH="x86_64"
export CROSS_COMPILE="${OIINUX_TARGET}-"

# =============================================================================
oiia "Building Oiinux kernel"
oiia "  Version : ${KERNEL_VER}"
oiia "  Arch    : ${ARCH}"
oiia "  Cross   : ${CROSS_COMPILE}"
oiia "  Jobs    : ${OIINUX_JOBS}"

# ── Verify cross-compiler ──────────────────────────────────────────────────────
command -v "${OIINUX_TARGET}-gcc" &>/dev/null \
    || die "Cross-compiler not found! Run 01-toolchain.sh first."
ok "Cross-compiler: $("${OIINUX_TARGET}-gcc" --version | head -1)"

# =============================================================================
step "1/4 — Downloading Linux kernel ${KERNEL_VER}"
# =============================================================================

SRC_TARBALL="${OIINUX_SRC}/kernel/src/linux-${KERNEL_VER}.tar.xz"

if [ ! -d "${KERNEL_DIR}" ]; then
    if [ ! -f "${SRC_TARBALL}" ]; then
        oiia "Downloading kernel (this is a big file ~140MB)..."
        wget -q --show-progress -O "${SRC_TARBALL}" "${KERNEL_URL}" \
            || die "Failed to download kernel"
        ok "Downloaded."
    fi

    oiia "Extracting kernel source (be patient)..."
    tar -xf "${SRC_TARBALL}" -C "${OIINUX_SRC}/kernel/src/"
    ok "Extracted to ${KERNEL_DIR}"
else
    ok "Kernel source already present."
fi

cd "${KERNEL_DIR}"

# =============================================================================
step "2/4 — Generating Oiinux kernel config"
# =============================================================================

if [ -f "${OIINUX_KCONFIG}" ]; then
    oiia "Using saved Oiinux config: ${OIINUX_KCONFIG}"
    cp "${OIINUX_KCONFIG}" .config
    run_logged "kernel-olddefconfig" make olddefconfig
else
    oiia "No saved config found — generating from defconfig + customizations"

    # Start from x86_64 default config
    run_logged "kernel-defconfig" make defconfig

    # Apply Oiinux-specific settings using kernel's config script
    # We want: minimal, fast boot, musl-compatible, virtio for VM testing
    ./scripts/config \
        --set-str  LOCALVERSION    "-oiinux" \
        --enable   CONFIG_64BIT \
        --enable   CONFIG_SMP \
        --enable   CONFIG_MODULES \
        --enable   CONFIG_MODULE_UNLOAD \
        --enable   CONFIG_DEVTMPFS \
        --enable   CONFIG_DEVTMPFS_MOUNT \
        --enable   CONFIG_EXT4_FS \
        --enable   CONFIG_BTRFS_FS \
        --enable   CONFIG_VFAT_FS \
        --enable   CONFIG_PROC_FS \
        --enable   CONFIG_SYSFS \
        --enable   CONFIG_TMPFS \
        --enable   CONFIG_INOTIFY_USER \
        --enable   CONFIG_UNIX \
        --enable   CONFIG_INET \
        --enable   CONFIG_NET \
        --enable   CONFIG_PACKET \
        --enable   CONFIG_NETFILTER \
        --enable   CONFIG_BLK_DEV_INITRD \
        --enable   CONFIG_VIRTIO \
        --enable   CONFIG_VIRTIO_PCI \
        --enable   CONFIG_VIRTIO_BLK \
        --enable   CONFIG_VIRTIO_NET \
        --enable   CONFIG_VIRTIO_CONSOLE \
        --enable   CONFIG_DRM \
        --enable   CONFIG_FB \
        --enable   CONFIG_FRAMEBUFFER_CONSOLE \
        --enable   CONFIG_USB \
        --enable   CONFIG_USB_HID \
        --enable   CONFIG_HID_GENERIC \
        --enable   CONFIG_INPUT_KEYBOARD \
        --enable   CONFIG_INPUT_MOUSE \
        --disable  CONFIG_WERROR || true

    run_logged "kernel-olddefconfig" make olddefconfig

    # Save our config for next time
    cp .config "${OIINUX_KCONFIG}"
    ok "Config saved to ${OIINUX_KCONFIG}"
fi

oiia ""
oiia "TIP: Run 'make menuconfig' in ${KERNEL_DIR} to customize further!"
oiia "     This opens a TUI with every single kernel option explained."
oiia ""

# =============================================================================
step "3/4 — Compiling the kernel (this WILL take a while)"
# =============================================================================

oiia "Starting kernel build with ${OIINUX_JOBS} jobs..."
oiia "On the i5-6600 mini PC this might take 30-60 minutes."
oiia "On your main PC: probably 5-15 minutes."
echo ""

# We don't use run_logged here because we want live output for the kernel
# (it's the most satisfying part to watch)
make \
    ARCH="${ARCH}" \
    CROSS_COMPILE="${CROSS_COMPILE}" \
    -j"${OIINUX_JOBS}" \
    || die "Kernel build failed! Check output above."

ok "Kernel compiled!"

# =============================================================================
step "4/4 — Installing kernel + modules"
# =============================================================================

ROOTFS="${OIINUX_ROOTFS}"
ISO_STAGING="${OIINUX_SRC}/iso/staging"
mkdir -p "${ISO_STAGING}/boot" "${ROOTFS}"

# Copy the compressed kernel image (bzImage)
BZIMAGE="${KERNEL_DIR}/arch/x86_64/boot/bzImage"
[ -f "${BZIMAGE}" ] || die "bzImage not found at ${BZIMAGE}"

cp "${BZIMAGE}" "${ISO_STAGING}/boot/oiinux-kernel"
ok "Kernel image → ${ISO_STAGING}/boot/oiinux-kernel"

# Install kernel modules into rootfs
oiia "Installing kernel modules..."
make \
    ARCH="${ARCH}" \
    CROSS_COMPILE="${CROSS_COMPILE}" \
    INSTALL_MOD_PATH="${ROOTFS}" \
    modules_install \
    &> "${LOG_DIR}/kernel-modules-install.log"
ok "Modules installed to ${ROOTFS}/lib/modules/"

# Save kernel version string
KERNEL_RELEASE=$(make -s kernelrelease)
echo "${KERNEL_RELEASE}" > "${OIINUX_SRC}/kernel/config/kernel-release"
ok "Kernel release: ${KERNEL_RELEASE}"

# =============================================================================
echo ""
echo -e "${GRN}${BLD}Kernel built! 🐱${RST}"
echo ""
echo -e "  Image   : ${CYN}${ISO_STAGING}/boot/oiinux-kernel${RST}"
echo -e "  Release : ${CYN}${KERNEL_RELEASE}${RST}"
echo -e "  Modules : ${CYN}${ROOTFS}/lib/modules/${KERNEL_RELEASE}/${RST}"
echo ""
echo -e "  Quick boot test (requires QEMU):"
echo -e "  ${CYN}qemu-system-x86_64 -kernel ${ISO_STAGING}/boot/oiinux-kernel -m 512M${RST}"
echo ""
echo -e "  It'll panic (no rootfs yet) — that's FINE. Kernel booted = WIN. 🎉"
echo ""
echo -e "  Next: Build the root filesystem and init system."
echo ""
