#!/usr/bin/env bash
# =============================================================================
# Oiinux :: 00-setup.sh
# Creates the full project directory structure and installs build dependencies.
# Run this ONCE on your build machine before anything else.
# =============================================================================

set -euo pipefail

# ── Colors ────────────────────────────────────────────────────────────────────
RED='\033[0;31m'
GRN='\033[0;32m'
YLW='\033[1;33m'
CYN='\033[0;36m'
BLD='\033[1m'
RST='\033[0m'

oiia() { echo -e "${CYN}${BLD}[oiinux]${RST} $*"; }
ok()   { echo -e "${GRN}[  OK  ]${RST} $*"; }
warn() { echo -e "${YLW}[ WARN ]${RST} $*"; }
die()  { echo -e "${RED}[ FAIL ]${RST} $*"; exit 1; }

# =============================================================================
# CONFIG — edit these if needed
# =============================================================================

# Root of the Oiinux source tree (where this repo lives)
export OIINUX_SRC="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Where the cross-compilation toolchain will be installed
export OIINUX_TOOLS="${OIINUX_SRC}/toolchain/installed"

# Where the final root filesystem is assembled
export OIINUX_ROOTFS="${OIINUX_SRC}/rootfs"

# Target triplet for cross-compilation
export OIINUX_TARGET="x86_64-oiinux-linux-musl"

# Number of parallel make jobs (adjust for your hardware)
# High-perf PC: nproc; Mini PC i5-6600: max 4
export OIINUX_JOBS=$(nproc)

# =============================================================================
echo ""
echo -e "${CYN}${BLD}"
echo "   ██████╗ ██╗██╗███╗   ██╗██╗   ██╗██╗  ██╗"
echo "  ██╔═══██╗██║██║████╗  ██║██║   ██║╚██╗██╔╝"
echo "  ██║   ██║██║██║██╔██╗ ██║██║   ██║ ╚███╔╝ "
echo "  ██║   ██║██║██║██║╚██╗██║██║   ██║ ██╔██╗ "
echo "  ╚██████╔╝██║██║██║ ╚████║╚██████╔╝██╔╝ ██╗"
echo "   ╚═════╝ ╚═╝╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝  ╚═╝"
echo -e "${RST}"
echo -e "${BLD}  from the kernel up. for the oiia cat. 🐱${RST}"
echo ""
# =============================================================================

oiia "Setting up Oiinux build environment..."
oiia "Source root : ${OIINUX_SRC}"
oiia "Toolchain   : ${OIINUX_TOOLS}"
oiia "Rootfs      : ${OIINUX_ROOTFS}"
oiia "Target      : ${OIINUX_TARGET}"
oiia "Jobs        : ${OIINUX_JOBS}"
echo ""

# ── Detect distro ─────────────────────────────────────────────────────────────
detect_distro() {
    if [ -f /etc/arch-release ]; then
        echo "arch"
    elif [ -f /etc/debian_version ]; then
        echo "debian"
    else
        echo "unknown"
    fi
}

DISTRO=$(detect_distro)
oiia "Detected build host: ${DISTRO}"

# ── Install dependencies ───────────────────────────────────────────────────────
install_deps() {
    oiia "Installing build dependencies..."

    if [ "$DISTRO" = "arch" ]; then
        sudo pacman -Syu --noconfirm \
            base-devel \
            bc \
            bison \
            flex \
            git \
            gmp \
            libelf \
            libmpc \
            mpfr \
            ncurses \
            openssl \
            python \
            texinfo \
            wget \
            xz
        ok "Arch dependencies installed."

    elif [ "$DISTRO" = "debian" ]; then
        sudo apt-get update -y
        sudo apt-get install -y \
            bc \
            bison \
            build-essential \
            flex \
            gawk \
            git \
            libelf-dev \
            libgmp-dev \
            libmpc-dev \
            libmpfr-dev \
            libncurses-dev \
            libssl-dev \
            python3 \
            texinfo \
            wget \
            xz-utils
        ok "Debian/Ubuntu dependencies installed."

    else
        warn "Unknown distro — skipping auto-install. Install these manually:"
        warn "  gcc, g++, make, bison, flex, git, wget, bc, python3,"
        warn "  libssl-dev, libelf-dev, libncurses-dev, gmp, mpfr, mpc, texinfo"
    fi
}

install_deps

# ── Create directory tree ──────────────────────────────────────────────────────
oiia "Creating directory structure..."

DIRS=(
    "${OIINUX_SRC}/toolchain/src"
    "${OIINUX_SRC}/toolchain/build/binutils"
    "${OIINUX_SRC}/toolchain/build/gcc-pass1"
    "${OIINUX_SRC}/toolchain/build/musl"
    "${OIINUX_SRC}/toolchain/build/gcc-pass2"
    "${OIINUX_SRC}/toolchain/installed"
    "${OIINUX_SRC}/kernel/src"
    "${OIINUX_SRC}/kernel/config"
    "${OIINUX_SRC}/userspace/src"
    "${OIINUX_SRC}/userspace/build"
    "${OIINUX_SRC}/init/src"
    "${OIINUX_SRC}/rootfs"
    "${OIINUX_SRC}/iso/staging"
    "${OIINUX_SRC}/packages"
    "${OIINUX_SRC}/branding"
    "${OIINUX_SRC}/logs"
)

for d in "${DIRS[@]}"; do
    mkdir -p "$d"
done

ok "Directory tree created."

# ── Write env file ─────────────────────────────────────────────────────────────
ENV_FILE="${OIINUX_SRC}/oiinux-env.sh"
oiia "Writing environment file: ${ENV_FILE}"

cat > "${ENV_FILE}" <<EOF
# Oiinux build environment — source this before building anything
# Usage: source oiinux-env.sh

export OIINUX_SRC="${OIINUX_SRC}"
export OIINUX_TOOLS="${OIINUX_TOOLS}"
export OIINUX_ROOTFS="${OIINUX_ROOTFS}"
export OIINUX_TARGET="${OIINUX_TARGET}"
export OIINUX_JOBS="${OIINUX_JOBS}"

# Put our cross-compiler first in PATH
export PATH="\${OIINUX_TOOLS}/bin:\${PATH}"

echo "🐱 Oiinux build environment loaded."
echo "   Target : \${OIINUX_TARGET}"
echo "   Tools  : \${OIINUX_TOOLS}"
echo "   Jobs   : \${OIINUX_JOBS}"
EOF

chmod +x "${ENV_FILE}"
ok "Environment file written."

# ── Init git if needed ─────────────────────────────────────────────────────────
if [ ! -d "${OIINUX_SRC}/.git" ]; then
    oiia "Initializing git repository..."
    cd "${OIINUX_SRC}"
    git init
    git add .
    git commit -m "🐱 oiinux: initial scaffold"
    ok "Git initialized."
else
    ok "Git already initialized."
fi

# ── Check disk space ───────────────────────────────────────────────────────────
FREE_GB=$(df -BG "${OIINUX_SRC}" | awk 'NR==2 {gsub("G",""); print $4}')
oiia "Free disk space: ${FREE_GB}GB"
if [ "${FREE_GB}" -lt 50 ]; then
    warn "Less than 50GB free! Full build needs ~80-100GB. Consider freeing space."
else
    ok "Disk space looks good."
fi

# ── Done ───────────────────────────────────────────────────────────────────────
echo ""
echo -e "${GRN}${BLD}Setup complete! 🐱${RST}"
echo ""
echo -e "  Next step: ${CYN}source oiinux-env.sh && ./scripts/01-toolchain.sh${RST}"
echo ""
