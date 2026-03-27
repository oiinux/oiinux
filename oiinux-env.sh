# Oiinux build environment — source this before building anything
# Usage: source oiinux-env.sh

export OIINUX_SRC="/home/cigany/oiinux"
export OIINUX_TOOLS="/home/cigany/oiinux/toolchain/installed"
export OIINUX_ROOTFS="/home/cigany/oiinux/rootfs"
export OIINUX_TARGET="x86_64-oiinux-linux-musl"
export OIINUX_JOBS="16"

# Put our cross-compiler first in PATH
export PATH="${OIINUX_TOOLS}/bin:${PATH}"

echo "🐱 Oiinux build environment loaded."
echo "   Target : ${OIINUX_TARGET}"
echo "   Tools  : ${OIINUX_TOOLS}"
echo "   Jobs   : ${OIINUX_JOBS}"
