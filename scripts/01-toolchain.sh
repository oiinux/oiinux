#!/usr/bin/env bash
# =============================================================================
# Oiinux :: 01-toolchain.sh
#
# Builds the cross-compilation toolchain:
#   binutils  → assembler, linker, objdump, etc.
#   gcc pass1 → minimal C compiler (no libc yet)
#   musl libc → our C standard library
#   gcc pass2 → full C/C++ compiler against musl
#
# This will take 30-90 minutes depending on your hardware.
# On the i5-6600 mini PC: grab a coffee. Or five.
#
# Run AFTER: source oiinux-env.sh
# =============================================================================

set -euo pipefail

# ── Verify env ─────────────────────────────────────────────────────────────────
: "${OIINUX_SRC:?Run: source oiinux-env.sh first}"
: "${OIINUX_TOOLS:?Run: source oiinux-env.sh first}"
: "${OIINUX_TARGET:?Run: source oiinux-env.sh first}"
: "${OIINUX_JOBS:?Run: source oiinux-env.sh first}"

# ── Colors ────────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GRN='\033[0;32m'; YLW='\033[1;33m'
CYN='\033[0;36m'; BLD='\033[1m'; RST='\033[0m'

oiia()  { echo -e "${CYN}${BLD}[oiinux]${RST} $*"; }
step()  { echo -e "\n${BLD}${YLW}━━━ $* ━━━${RST}\n"; }
ok()    { echo -e "${GRN}[  OK  ]${RST} $*"; }
die()   { echo -e "${RED}[ FAIL ]${RST} $*"; exit 1; }

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
# VERSIONS — bump these when new releases come out
# =============================================================================
BINUTILS_VER="2.42"
GCC_VER="14.1.0"
MUSL_VER="1.2.5"

BINUTILS_URL="https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VER}.tar.xz"
GCC_URL="https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VER}/gcc-${GCC_VER}.tar.xz"
MUSL_URL="https://musl.libc.org/releases/musl-${MUSL_VER}.tar.gz"

SRC="${OIINUX_SRC}/toolchain/src"
BLD="${OIINUX_SRC}/toolchain/build"
PFX="${OIINUX_TOOLS}"

# =============================================================================
oiia "Building Oiinux cross-compilation toolchain"
oiia "  Target  : ${OIINUX_TARGET}"
oiia "  Prefix  : ${PFX}"
oiia "  Jobs    : ${OIINUX_JOBS}"
oiia "  Versions: binutils=${BINUTILS_VER}, gcc=${GCC_VER}, musl=${MUSL_VER}"
echo ""

# ── Helper: download + extract ─────────────────────────────────────────────────
fetch() {
    local url="$1"
    local filename="${url##*/}"
    local dest="${SRC}/${filename}"

    if [ -f "${dest}" ]; then
        ok "Already downloaded: ${filename}"
    else
        oiia "Downloading: ${filename}"
        wget -q --show-progress -O "${dest}" "${url}" || die "Failed to download ${url}"
        ok "Downloaded: ${filename}"
    fi

    oiia "Extracting: ${filename}"
    tar -xf "${dest}" -C "${SRC}/" 2>/dev/null || true
    ok "Extracted: ${filename}"
}

# =============================================================================
step "1/5 — Downloading sources"
# =============================================================================

fetch "${BINUTILS_URL}"
fetch "${GCC_URL}"
fetch "${MUSL_URL}"

# GCC needs its prerequisites (gmp, mpfr, mpc, isl)
oiia "Downloading GCC prerequisites..."
cd "${SRC}/gcc-${GCC_VER}"
if [ ! -d "gmp" ]; then
    run_logged "gcc-prereqs" ./contrib/download_prerequisites
else
    ok "GCC prerequisites already present."
fi

# =============================================================================
step "2/5 — Building binutils (assembler, linker)"
# =============================================================================

mkdir -p "${BLD}/binutils"
cd "${BLD}/binutils"

run_logged "binutils-configure" \
    "${SRC}/binutils-${BINUTILS_VER}/configure" \
        --target="${OIINUX_TARGET}" \
        --prefix="${PFX}" \
        --with-sysroot="${PFX}/${OIINUX_TARGET}" \
        --disable-nls \
        --disable-multilib \
        --disable-werror

run_logged "binutils-make"    make -j"${OIINUX_JOBS}"
run_logged "binutils-install" make install

ok "binutils installed to ${PFX}"

# =============================================================================
step "3/5 — Building GCC pass 1 (no libc — just enough to compile musl)"
# =============================================================================

mkdir -p "${BLD}/gcc-pass1"
cd "${BLD}/gcc-pass1"

run_logged "gcc-pass1-configure" \
    "${SRC}/gcc-${GCC_VER}/configure" \
        --target="${OIINUX_TARGET}" \
        --prefix="${PFX}" \
        --with-sysroot="${PFX}/${OIINUX_TARGET}" \
        --enable-languages=c \
        --disable-multilib \
        --disable-nls \
        --disable-shared \
        --disable-threads \
        --disable-libatomic \
        --disable-libgomp \
        --disable-libquadmath \
        --disable-libssp \
        --disable-libvtv \
        --disable-libstdcxx \
        --without-headers

run_logged "gcc-pass1-make"    make -j"${OIINUX_JOBS}" all-gcc all-target-libgcc
run_logged "gcc-pass1-install" make install-gcc install-target-libgcc

ok "GCC pass 1 installed to ${PFX}"

# ── Put toolchain in PATH now ──────────────────────────────────────────────────
export PATH="${PFX}/bin:${PATH}"
oiia "Toolchain added to PATH. Verifying..."
"${OIINUX_TARGET}-gcc" --version | head -1 || die "Cross-compiler not found after pass 1!"
ok "Cross-compiler works: $("${OIINUX_TARGET}-gcc" --version | head -1)"

# =============================================================================
step "4/5 — Building musl libc (our C standard library)"
# =============================================================================

# Create sysroot include dir
mkdir -p "${PFX}/${OIINUX_TARGET}/include"

cd "${SRC}/musl-${MUSL_VER}"

run_logged "musl-configure" \
    ./configure \
        --prefix="${PFX}/${OIINUX_TARGET}" \
        --host="${OIINUX_TARGET}" \
        CC="${OIINUX_TARGET}-gcc" \
        CFLAGS="-O2"

run_logged "musl-make"    make -j"${OIINUX_JOBS}"
run_logged "musl-install" make install

ok "musl libc installed to ${PFX}/${OIINUX_TARGET}"

# ── Create libc.so symlink that GCC expects ────────────────────────────────────
MUSL_LIB="${PFX}/${OIINUX_TARGET}/lib"
if [ ! -f "${MUSL_LIB}/libc.so" ]; then
    ln -sf libc.so "${MUSL_LIB}/libc.so" 2>/dev/null || true
fi

# =============================================================================
step "5/5 — Building GCC pass 2 (full C + C++ with musl)"
# =============================================================================

mkdir -p "${BLD}/gcc-pass2"
cd "${BLD}/gcc-pass2"

run_logged "gcc-pass2-configure" \
    "${SRC}/gcc-${GCC_VER}/configure" \
        --target="${OIINUX_TARGET}" \
        --prefix="${PFX}" \
        --with-sysroot="${PFX}/${OIINUX_TARGET}" \
        --enable-languages=c,c++ \
        --disable-multilib \
        --disable-nls \
        --enable-shared \
        --enable-threads=posix \
        --enable-c99 \
        --enable-long-long

run_logged "gcc-pass2-make"    make -j"${OIINUX_JOBS}"
run_logged "gcc-pass2-install" make install

ok "GCC pass 2 installed to ${PFX}"

# =============================================================================
step "Verification"
# =============================================================================

oiia "Verifying toolchain..."

echo "#include <stdio.h>
int main() {
    printf(\"oiia! toolchain works.\\n\");
    return 0;
}" > /tmp/oiia_test.c

"${OIINUX_TARGET}-gcc" \
    -static \
    /tmp/oiia_test.c \
    -o /tmp/oiia_test \
    || die "Test compile failed!"

file /tmp/oiia_test | grep -q "x86-64" || die "Binary is wrong architecture!"
ok "Test binary compiled successfully: $(file /tmp/oiia_test)"

# Clean up
rm -f /tmp/oiia_test.c /tmp/oiia_test

# =============================================================================
echo ""
echo -e "${GRN}${BLD}Toolchain complete! 🐱${RST}"
echo ""
echo -e "  Binutils : ${CYN}${OIINUX_TARGET}-ld --version${RST}"
echo -e "  Compiler : ${CYN}${OIINUX_TARGET}-gcc --version${RST}"
echo -e "  libc     : ${CYN}musl ${MUSL_VER}${RST}"
echo ""
echo -e "  Next step: ${CYN}./scripts/02-kernel.sh${RST}"
echo ""
