#!/usr/bin/env bash
#
# Krita HarmonyOS — third-party dependency cross-compiler.
#
# Builds Krita's native dependencies for OHOS arm64 (aarch64-linux-ohos) into a
# project-local prefix that the FULL libentry.so build consumes. This is Krita's
# OWN dependency tree — like the bundled Qt SDK, it lives inside this repo and
# never relies on another project's artifacts.
#
#   sources install to : harmony_pc/deps/src/<name>       (gitignored)
#   builds   go to      : harmony_pc/deps/build/<name>     (gitignored)
#   results  install to : harmony_pc/deps/prefix           (consumed by CMake)
#
# Usage:
#   ./build-deps.sh <dep> [<dep> ...]   # build specific deps, in the given order
#   ./build-deps.sh tier1               # the foundational image/io libs
#   ./build-deps.sh list                # show known deps
#
# Override the OHOS SDK location with OHOS_SDK_NATIVE=/path/to/.../native
#
set -euo pipefail

# --------------------------------------------------------------------------
# Toolchain
# --------------------------------------------------------------------------
OHOS_SDK_NATIVE="${OHOS_SDK_NATIVE:-$HOME/ohos/command-line-tools/sdk/default/openharmony/native}"
TARGET="aarch64-linux-ohos"
OHOS_ARCH="arm64-v8a"

if [ ! -d "$OHOS_SDK_NATIVE" ]; then
    echo "ERROR: OHOS native SDK not found at: $OHOS_SDK_NATIVE" >&2
    echo "       Set OHOS_SDK_NATIVE to your command-line-tools/.../openharmony/native dir." >&2
    exit 1
fi

LLVM_BIN="$OHOS_SDK_NATIVE/llvm/bin"
SYSROOT="$OHOS_SDK_NATIVE/sysroot"
TOOLCHAIN_FILE="$OHOS_SDK_NATIVE/build/cmake/ohos.toolchain.cmake"
CMAKE_BIN="$OHOS_SDK_NATIVE/build-tools/cmake/bin/cmake"
NINJA_BIN="$OHOS_SDK_NATIVE/build-tools/cmake/bin/ninja"

export CC="$LLVM_BIN/clang --target=$TARGET --sysroot=$SYSROOT"
export CXX="$LLVM_BIN/clang++ --target=$TARGET --sysroot=$SYSROOT"
export AR="$LLVM_BIN/llvm-ar"
export RANLIB="$LLVM_BIN/llvm-ranlib"
export STRIP="$LLVM_BIN/llvm-strip"
export LD="$LLVM_BIN/ld.lld"

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$HERE/src"
BUILD_DIR="$HERE/build"
PREFIX="$HERE/prefix"
# This project's own bundled Qt 5 for HarmonyOS SDK (harmony_pc/qtforharmony_sdk).
QT_SDK="$(cd "$HERE/.." && pwd)/qtforharmony_sdk"
# Krita source root (the repo): harmony_pc/deps -> harmony_pc -> repo root.
KRITA_ROOT="$(cd "$HERE/../.." && pwd)"
# Host (macOS) prefix + host Qt5 — used ONLY to build the KDE Frameworks codegen
# tools (kconfig_compiler_kf5, desktoptojson) that the cross build runs on the
# host. The target still links this project's bundled Qt5 OHOS SDK.
HOST_PREFIX="$HERE/host-prefix"
HOST_QT5="${HOST_QT5:-/opt/homebrew/opt/qt@5}"
mkdir -p "$SRC_DIR" "$BUILD_DIR" "$PREFIX" "$HOST_PREFIX"

# Make every dep see the ones built before it.
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
export CFLAGS="-I$PREFIX/include -I$SYSROOT/usr/include -O2 -fPIC"
export CXXFLAGS="$CFLAGS"
export LDFLAGS="-L$PREFIX/lib -L$SYSROOT/usr/lib/$TARGET"

log()  { printf '\033[1;36m[deps]\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m[deps]\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[1;31m[deps] ERROR:\033[0m %s\n' "$*" >&2; exit 1; }

fetch() { # fetch <tarball-name> <url> [fallback-url ...]
    local out="$SRC_DIR/$1"; shift
    if [ -f "$out" ]; then log "cached $(basename "$out")"; return; fi
    local url
    for url in "$@"; do
        log "downloading $(basename "$out") <- $url"
        if curl -fSL --retry 3 --connect-timeout 20 -o "$out.tmp" "$url"; then
            mv "$out.tmp" "$out"; return
        fi
        warn "mirror failed: $url"
        rm -f "$out.tmp"
    done
    die "all mirrors failed for $(basename "$out")"
}

extract() { # extract <tarball-name> <expected-dir>
    local tb="$SRC_DIR/$1" dir="$SRC_DIR/$2"
    if [ -d "$dir" ]; then return; fi
    log "extracting $1"
    tar -C "$SRC_DIR" -xf "$tb"
}

cmake_build() { # cmake_build <src-subdir> <name> [extra cmake args...]
    local src="$SRC_DIR/$1"; shift
    local name="$1"; shift
    local b="$BUILD_DIR/$name"
    rm -rf "$b"; mkdir -p "$b"
    # The OHOS toolchain injects --target / --sysroot via CMAKE_*_COMPILER_TARGET
    # and CMAKE_SYSROOT, which are NOT part of the CMAKE_<LANG>_FLAGS strings.
    # Some projects (e.g. libpng's genout) re-invoke the bare compiler with only
    # ${CMAKE_C_FLAGS}, dropping target/sysroot and failing to find the OHOS
    # multiarch headers (bits/alltypes.h). Adding them to the FLAGS too is
    # harmless for normal compiles (same value, deduped) and fixes those steps.
    # _GNU_SOURCE: many KF/Krita (and musl) sources need GNU/BSD extension
    # declarations (e.g. dladdr in dlfcn.h is guarded by _GNU_SOURCE).
    local tflags="--target=$TARGET --sysroot=$SYSROOT -D_GNU_SOURCE"
    "$CMAKE_BIN" -S "$src" -B "$b" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
        -DOHOS_ARCH="$OHOS_ARCH" -DOHOS_PLATFORM=OHOS \
        -DCMAKE_MAKE_PROGRAM="$NINJA_BIN" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DCMAKE_PREFIX_PATH="$PREFIX" \
        -DCMAKE_C_FLAGS="$tflags" \
        -DCMAKE_CXX_FLAGS="$tflags" \
        -DBUILD_TESTING=OFF \
        "$@"
    "$CMAKE_BIN" --build "$b" --parallel
    "$CMAKE_BIN" --install "$b"
}

# Meson cross build — used for deps whose autotools configure misbehaves on the
# macOS host (e.g. fontconfig: macOS BSD `diff` returns 0 for missing/empty
# files, which breaks config.status's header "unchanged" optimization). Meson
# bypasses autotools/config.status entirely.
MESON_BIN="${MESON_BIN:-/opt/homebrew/bin/meson}"
CROSSFILE="$BUILD_DIR/ohos-cross.ini"
write_crossfile() {
    cat > "$CROSSFILE" <<EOF
[binaries]
c = '$LLVM_BIN/clang'
cpp = '$LLVM_BIN/clang++'
ar = '$LLVM_BIN/llvm-ar'
strip = '$LLVM_BIN/llvm-strip'
pkg-config = 'pkg-config'

[built-in options]
c_args = ['--target=$TARGET', '--sysroot=$SYSROOT', '-I$PREFIX/include', '-D_GNU_SOURCE']
cpp_args = ['--target=$TARGET', '--sysroot=$SYSROOT', '-I$PREFIX/include', '-D_GNU_SOURCE']
c_link_args = ['--target=$TARGET', '--sysroot=$SYSROOT', '-L$PREFIX/lib', '-L$SYSROOT/usr/lib/$TARGET']
cpp_link_args = ['--target=$TARGET', '--sysroot=$SYSROOT', '-L$PREFIX/lib', '-L$SYSROOT/usr/lib/$TARGET']

[host_machine]
system = 'linux'
cpu_family = 'aarch64'
cpu = 'aarch64'
endian = 'little'
EOF
}
meson_build() { # meson_build <src-subdir> <name> [meson -D args...]
    local src="$SRC_DIR/$1"; shift; local name="$1"; shift
    local b="$BUILD_DIR/$name"; rm -rf "$b"
    write_crossfile
    ( unset CC CXX AR RANLIB STRIP LD CFLAGS CXXFLAGS LDFLAGS
      export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
      "$MESON_BIN" setup "$b" "$src" --cross-file "$CROSSFILE" \
          --prefix="$PREFIX" --libdir=lib --buildtype=release \
          --default-library=shared "$@"
      "$MESON_BIN" compile -C "$b"
      "$MESON_BIN" install -C "$b" )
}

verify_arch() { # verify_arch <lib-basename>   e.g. verify_arch libpng
    local f
    # Resolve to a real ELF regular file: follow symlinks (-L), only this dir
    # (-maxdepth 1, so the lib/<name>/ cmake subdir is skipped), prefer the
    # versioned .so, fall back to a static .a.
    f=$(find -L "$PREFIX/lib" -maxdepth 1 -type f -name "$1*.so*" 2>/dev/null | sort | tail -1)
    [ -n "$f" ] || f=$(find -L "$PREFIX/lib" -maxdepth 1 -type f -name "$1*.a" 2>/dev/null | head -1)
    [ -n "$f" ] || die "expected artifact $1 not installed in $PREFIX/lib"
    # Capture first, THEN grep: piping readelf | grep -q under `set -o pipefail`
    # makes grep close the pipe on first match, readelf gets SIGPIPE (exit 141)
    # and pipefail reports the whole pipeline as failed — a false negative.
    local hdr; hdr=$("$LLVM_BIN/llvm-readelf" -h "$f" 2>/dev/null || true)
    if printf '%s' "$hdr" | grep -q "AArch64"; then
        log "OK $(basename "$f") is AArch64"
    else
        die "$f is NOT AArch64"
    fi
}

# ==========================================================================
# Recipes — foundational image / IO tier
# ==========================================================================
dep_png() {
    fetch libpng-1.6.43.tar.gz \
        "https://download.sourceforge.net/libpng/libpng-1.6.43.tar.gz" \
        "https://download.sourceforge.net/project/libpng/libpng16/1.6.43/libpng-1.6.43.tar.gz"
    extract libpng-1.6.43.tar.gz libpng-1.6.43
    cmake_build libpng-1.6.43 png \
        -DPNG_SHARED=ON -DPNG_STATIC=OFF -DPNG_TESTS=OFF -DPNG_TOOLS=OFF \
        -DZLIB_INCLUDE_DIR="$SYSROOT/usr/include" \
        -DZLIB_LIBRARY="$SYSROOT/usr/lib/$TARGET/libz.so"
    verify_arch libpng
}

dep_jpeg() {
    fetch libjpeg-turbo-3.0.0.tar.gz \
        "https://download.sourceforge.net/libjpeg-turbo/3.0.0/libjpeg-turbo-3.0.0.tar.gz" \
        "https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.0.0/libjpeg-turbo-3.0.0.tar.gz"
    extract libjpeg-turbo-3.0.0.tar.gz libjpeg-turbo-3.0.0
    cmake_build libjpeg-turbo-3.0.0 jpeg \
        -DENABLE_SHARED=ON -DENABLE_STATIC=OFF -DWITH_TURBOJPEG=ON
    verify_arch libjpeg
}

dep_tiff() {
    fetch tiff-4.6.0.tar.gz \
        "https://download.osgeo.org/libtiff/tiff-4.6.0.tar.gz"
    extract tiff-4.6.0.tar.gz tiff-4.6.0
    cmake_build tiff-4.6.0 tiff \
        -Dtiff-tools=OFF -Dtiff-tests=OFF -Dtiff-docs=OFF \
        -DZLIB_INCLUDE_DIR="$SYSROOT/usr/include" \
        -DZLIB_LIBRARY="$SYSROOT/usr/lib/$TARGET/libz.so" \
        -DJPEG_INCLUDE_DIR="$PREFIX/include" \
        -DJPEG_LIBRARY="$PREFIX/lib/libjpeg.so"
    verify_arch libtiff
}

dep_expat() {
    # github (canonical) is the primary, but it is frequently unreachable via
    # some proxies; Debian's source mirror carries 2.5.0 which is fine here.
    fetch expat-2.5.0.tar.gz \
        "http://deb.debian.org/debian/pool/main/e/expat/expat_2.5.0.orig.tar.gz" \
        "https://github.com/libexpat/libexpat/releases/download/R_2_5_0/expat-2.5.0.tar.gz"
    # Debian's .orig is a repo snapshot extracting to libexpat-R_2_5_0/, with the
    # library's CMakeLists under the expat/ subdir.
    extract expat-2.5.0.tar.gz libexpat-R_2_5_0
    cmake_build libexpat-R_2_5_0/expat expat \
        -DEXPAT_BUILD_TESTS=OFF -DEXPAT_BUILD_EXAMPLES=OFF \
        -DEXPAT_BUILD_TOOLS=OFF -DEXPAT_BUILD_DOCS=OFF -DEXPAT_SHARED_LIBS=ON
    verify_arch libexpat
}

dep_lcms2() {
    fetch lcms2-2.16.tar.gz \
        "https://download.sourceforge.net/lcms/lcms2-2.16.tar.gz" \
        "https://github.com/mm2/Little-CMS/releases/download/lcms2.16/lcms2-2.16.tar.gz"
    extract lcms2-2.16.tar.gz lcms2-2.16
    local s="$SRC_DIR/lcms2-2.16" b="$BUILD_DIR/lcms2"
    rm -rf "$b"; cp -R "$s" "$b"
    # Old config.sub doesn't know the *-ohos OS. Use the GNU triple for --host
    # (only drives autotools' cross-compile detection / tool-prefix guessing);
    # the real target comes from CC/CXX (clang --target=aarch64-linux-ohos).
    ( cd "$b" && ./configure --host=aarch64-linux-gnu --prefix="$PREFIX" \
        --disable-static --enable-shared )
    ( cd "$b" && make -j"$(sysctl -n hw.ncpu)" && make install )
    verify_arch liblcms2
}

dep_gif() {
    fetch giflib-5.2.2.tar.gz \
        "https://download.sourceforge.net/giflib/giflib-5.2.2.tar.gz"
    extract giflib-5.2.2.tar.gz giflib-5.2.2
    local b="$BUILD_DIR/giflib"; rm -rf "$b"; cp -R "$SRC_DIR/giflib-5.2.2" "$b"
    # giflib has no cross-friendly build; compile the library object set directly.
    ( cd "$b" && $CC $CFLAGS -c dgif_lib.c egif_lib.c gifalloc.c gif_err.c \
        gif_font.c gif_hash.c openbsd-reallocarray.c quantize.c )
    ( cd "$b" && $CC -shared -Wl,-soname,libgif.so -o "$PREFIX/lib/libgif.so" *.o $LDFLAGS )
    cp "$b/gif_lib.h" "$PREFIX/include/"
    verify_arch libgif
}

# ==========================================================================
# Recipes — higher-level tier (Krita REQUIRED: Boost, Eigen3, Exiv2)
# ==========================================================================
dep_boost() {
    # Krita uses Boost header-only (find_package(Boost) with no COMPONENTS), so
    # no compilation is needed — the release tarball ships pre-generated headers.
    fetch boost_1_85_0.tar.bz2 \
        "https://download.sourceforge.net/boost/boost_1_85_0.tar.bz2" \
        "https://archives.boost.io/release/1.85.0/source/boost_1_85_0.tar.bz2"
    extract boost_1_85_0.tar.bz2 boost_1_85_0
    rm -rf "$PREFIX/include/boost"
    cp -R "$SRC_DIR/boost_1_85_0/boost" "$PREFIX/include/boost"
    # OHOS musl has no pthread cancellation (pthread_setcancelstate), like
    # Android/bionic. Boost only excludes __ANDROID__; also exclude __OHOS__.
    sed -i '' 's/\&\& !defined(__ANDROID__)/\&\& !defined(__ANDROID__) \&\& !defined(__OHOS__)/g' \
        "$PREFIX/include/boost/core/detail/sp_thread_sleep.hpp"
    [ -f "$PREFIX/include/boost/version.hpp" ] \
        && log "OK boost headers installed (header-only)" \
        || die "boost headers missing"
}

dep_eigen() {
    # Header-only; the CMake install places the headers AND Eigen3Config.cmake so
    # Krita's find_package(Eigen3 NO_MODULE) works.
    fetch eigen-3.4.0.tar.gz \
        "https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz"
    extract eigen-3.4.0.tar.gz eigen-3.4.0
    cmake_build eigen-3.4.0 eigen -DEIGEN_BUILD_DOC=OFF -DEIGEN_BUILD_TESTING=OFF
    [ -f "$PREFIX/share/eigen3/cmake/Eigen3Config.cmake" ] \
        || [ -f "$PREFIX/share/cmake/eigen3/Eigen3Config.cmake" ] \
        && log "OK eigen3 headers + cmake config installed" \
        || warn "eigen3 installed but Eigen3Config.cmake not at expected path"
}

dep_exiv2() {
    # 0.27.7: needs only expat + zlib (0.28 additionally needs brotli + inih).
    fetch exiv2-0.27.7.tar.gz \
        "https://github.com/Exiv2/exiv2/archive/refs/tags/v0.27.7.tar.gz"
    extract exiv2-0.27.7.tar.gz exiv2-0.27.7
    cmake_build exiv2-0.27.7 exiv2 \
        -DEXIV2_ENABLE_XMP=ON -DEXIV2_ENABLE_NLS=OFF -DEXIV2_ENABLE_BMFF=OFF \
        -DEXIV2_BUILD_SAMPLES=OFF -DEXIV2_BUILD_EXIV2_COMMAND=OFF \
        -DEXIV2_BUILD_UNIT_TESTS=OFF -DEXIV2_ENABLE_CURL=OFF \
        -DEXPAT_INCLUDE_DIR="$PREFIX/include" \
        -DEXPAT_LIBRARY="$PREFIX/lib/libexpat.so" \
        -DZLIB_INCLUDE_DIR="$SYSROOT/usr/include" \
        -DZLIB_LIBRARY="$SYSROOT/usr/lib/$TARGET/libz.so"
    verify_arch libexiv2
}

dep_quazip() {
    # QuaZip is a Qt wrapper over zlib's minizip; Krita's KoStore (.kra archive
    # read/write) links it. It needs Qt5 (this project's bundled SDK) + zlib.
    fetch quazip-1.4.tar.gz \
        "https://github.com/stachenov/quazip/archive/refs/tags/v1.4.tar.gz"
    extract quazip-1.4.tar.gz quazip-1.4
    # QuaZip bundles its own minizip and exports its symbols (unzGoToFirstFile,
    # ...) with default visibility. The OHOS *system* zlib (libshared_libz.z.so)
    # ALSO exports a minizip with the same names but an incompatible ioapi/struct
    # ABI. On glibc, -Bsymbolic (DF_SYMBOLIC) makes QuaZip prefer its own copies;
    # the OHOS musl linker IGNORES DF_SYMBOLIC, so at runtime QuaZip's internal
    # minizip calls get interposed by the system zlib's incompatible minizip. The
    # central-directory parse then reads garbage: a bad pointer (SIGSEGV in
    # qiodevice64_seek_file_func) or a garbage uncompressed size that
    # QByteArray::resize()s tens of GiB (RSS killer -> SIGKILL) while loading
    # Krita's resource .bundle archives at startup.
    #
    # Fix: a version script that *localizes* the minizip symbols. Local symbols
    # are non-interposable and bound at link time, so QuaZip always uses its own
    # minizip regardless of the linker's DF_SYMBOLIC support. The C++ API stays
    # exported. (-Bsymbolic kept too; harmless where honoured.)
    cmake_build quazip-1.4 quazip \
        -DCMAKE_PREFIX_PATH="$PREFIX;$QT_SDK" \
        -DCMAKE_FIND_ROOT_PATH="$SYSROOT;$PREFIX;$QT_SDK" \
        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH \
        -DQUAZIP_QT_MAJOR_VERSION=5 \
        -DQUAZIP_ENABLE_TESTS=OFF \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_SHARED_LINKER_FLAGS="-Wl,-Bsymbolic -Wl,--version-script=$HERE/quazip-localize-minizip.map" \
        -DZLIB_INCLUDE_DIR="$SYSROOT/usr/include" \
        -DZLIB_LIBRARY="$SYSROOT/usr/lib/$TARGET/libz.so"
    verify_arch libquazip
    # Sanity 1: the minizip C symbols must now be LOCAL, not GLOBAL — if still
    # exported they can be interposed by the system zlib's incompatible minizip.
    local _qz _syms; _qz="$(ls "$PREFIX"/lib/libquazip1-qt5.so.*.* 2>/dev/null | head -1)"
    [ -z "$_qz" ] && { echo "[deps] ERROR: libquazip not found after build"; exit 1; }
    _syms="$("$LLVM_BIN/llvm-readelf" --dyn-syms "$_qz" 2>/dev/null)"
    if printf '%s' "$_syms" | grep -qE "GLOBAL .* unzGoToFirstFile"; then
        echo "[deps] ERROR: libquazip still EXPORTS unzGoToFirstFile (version script not applied)"; exit 1
    fi
    # Sanity 2: the PUBLIC QuaZip C++ API must STILL be exported, or dependents
    # (libkritaresources) fail to load ("symbol not found"). Guards against an
    # over-broad localize pattern accidentally hiding C++ methods.
    if ! printf '%s' "$_syms" | grep -qE "GLOBAL .* _ZN6QuaZip4open"; then
        echo "[deps] ERROR: libquazip no longer exports QuaZip::open (version script too broad)"; exit 1
    fi
    echo "[deps] OK libquazip: minizip C symbols localized, C++ API still exported"
}

# ==========================================================================
# Recipes — KDE Frameworks (tier3)
#
# Krita REQUIRES KF5 components: Config, CoreAddons, GuiAddons, WidgetsAddons,
# ItemViews, I18n, Completion. Cross-compiling KF needs the codegen tools
# (kconfig_compiler_kf5, desktoptojson) to RUN on the host during the build, so
# we build HOST copies (against brew host Qt5) and drop them over the arm64 ones
# the target build installs, at the fixed path the KF imported targets point to.
# ==========================================================================
KF_VER="5.116.0"
KF_BRANCH="5.116"

kf_fetch() { # kf_fetch <name>   e.g. kconfig
    fetch "$1-$KF_VER.tar.xz" \
        "https://download.kde.org/stable/frameworks/$KF_BRANCH/$1-$KF_VER.tar.xz"
    extract "$1-$KF_VER.tar.xz" "$1-$KF_VER"
}

# Native (macOS/host) cmake build into HOST_PREFIX with brew host Qt5. Clears the
# OHOS cross env so it produces host-runnable binaries.
host_build() { # host_build <src-subdir> <name> [extra cmake args...]
    local src="$SRC_DIR/$1"; shift; local name="$1"; shift
    local b="$BUILD_DIR/host-$name"; rm -rf "$b"; mkdir -p "$b"
    ( unset CC CXX AR RANLIB STRIP LD CFLAGS CXXFLAGS LDFLAGS PKG_CONFIG_PATH
      "$CMAKE_BIN" -S "$src" -B "$b" -G Ninja \
          -DCMAKE_MAKE_PROGRAM="$NINJA_BIN" \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX="$HOST_PREFIX" \
          -DCMAKE_PREFIX_PATH="$HOST_PREFIX;$PREFIX;$HOST_QT5" \
          -DBUILD_TESTING=OFF "$@"
      "$CMAKE_BIN" --build "$b" --parallel
      "$CMAKE_BIN" --install "$b" )
}

# Target (OHOS) KF build into PREFIX, against the bundled Qt SDK. Host codegen
# tools are picked up from the fixed $PREFIX/bin path (see dep_kconfig).
kf_build() { # kf_build <src-subdir> <name> [extra cmake args...]
    local s="$1" n="$2"; shift 2
    cmake_build "$s" "$n" \
        -DCMAKE_PREFIX_PATH="$PREFIX;$QT_SDK" \
        -DCMAKE_FIND_ROOT_PATH="$SYSROOT;$PREFIX;$QT_SDK" \
        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH \
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=BOTH \
        -DKDE_INSTALL_LIBDIR=lib -DKDE_INSTALL_USE_QT_SYS_PATHS=OFF \
        -DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=OFF \
        -DKF_IGNORE_PLATFORM_CHECK=TRUE \
        "$@"
}

dep_ecm() {
    kf_fetch extra-cmake-modules
    # ECM is pure CMake modules (arch-independent); install into PREFIX so both
    # host and target builds find it.
    host_build extra-cmake-modules-$KF_VER ecm -DCMAKE_INSTALL_PREFIX="$PREFIX"
    [ -f "$PREFIX/share/ECM/cmake/ECMConfig.cmake" ] \
        && log "OK ECM installed" || die "ECM not installed"
}

# Host codegen tools: kconfig_compiler_kf5 (KConfig) + desktoptojson (KCoreAddons)
dep_host_tools() {
    kf_fetch kcoreaddons
    kf_fetch kconfig
    host_build kcoreaddons-$KF_VER hkcoreaddons
    host_build kconfig-$KF_VER hkconfig
    # kconfig_compiler_kf5 installs under libexec (lib/libexec/kf5), not bin.
    find "$HOST_PREFIX" -name kconfig_compiler_kf5 -type f | grep -q . \
        && log "OK host kconfig_compiler_kf5" || die "missing host kconfig_compiler_kf5"
    find "$HOST_PREFIX" -name desktoptojson -type f | grep -q . \
        && log "OK host desktoptojson" || die "missing host desktoptojson"
}

# Replace an arm64 codegen tool the target build just installed with the host
# one, at the SAME relative path (so the KF imported target keeps resolving).
_use_host_tool() { # _use_host_tool <tool-name>
    local src; src=$(find "$HOST_PREFIX" -name "$1" -type f 2>/dev/null | head -1)
    [ -n "$src" ] || { warn "host tool $1 not found under $HOST_PREFIX"; return; }
    local rel="${src#$HOST_PREFIX/}" dst
    dst="$PREFIX/$rel"
    mkdir -p "$(dirname "$dst")"
    cp -f "$src" "$dst"
    log "swapped in host $1 -> $rel"
}

dep_kcoreaddons() { kf_fetch kcoreaddons; kf_build kcoreaddons-$KF_VER kcoreaddons \
    -DHAVE_DLADDR=ON;  # dladdr is in OHOS musl libc; force it (CMakeLists mishandles the empty-var case)
    _use_host_tool desktoptojson; verify_arch libKF5CoreAddons; }
dep_kconfig()     { kf_fetch kconfig;     kf_build kconfig-$KF_VER kconfig \
    -DKCONFIG_USE_DBUS=OFF;  # OHOS has no D-Bus (like Android)
    _use_host_tool kconfig_compiler_kf5;  verify_arch libKF5ConfigCore; }
dep_kguiaddons()  { kf_fetch kguiaddons;  kf_build kguiaddons-$KF_VER kguiaddons \
    -DWITH_WAYLAND=OFF -DWITH_X11=OFF -DWITH_DBUS=OFF; verify_arch libKF5GuiAddons; }
dep_kwidgetsaddons() { kf_fetch kwidgetsaddons; kf_build kwidgetsaddons-$KF_VER kwidgetsaddons
    verify_arch libKF5WidgetsAddons; }
dep_kitemviews()  { kf_fetch kitemviews;  kf_build kitemviews-$KF_VER kitemviews
    verify_arch libKF5ItemViews; }
# OHOS musl ships libintl.h but no libintl, and libc doesn't export gettext as a
# dynamic symbol, so ECM's FindLibIntl fails. Provide a tiny stub libintl (the
# proxy-libintl approach KDE uses on Android/Windows): gettext returns the msgid
# untranslated. Enough to satisfy KI18n's link + the runtime English UI.
dep_libintl() {
    local b="$BUILD_DIR/libintl"; rm -rf "$b"; mkdir -p "$b"
    cat > "$b/libintl.h" <<'EOF'
#ifndef _LIBINTL_H
#define _LIBINTL_H 1
#ifdef __cplusplus
extern "C" {
#endif
char *gettext(const char *msgid);
char *dgettext(const char *domainname, const char *msgid);
char *dcgettext(const char *domainname, const char *msgid, int category);
char *ngettext(const char *msgid1, const char *msgid2, unsigned long int n);
char *dngettext(const char *domainname, const char *msgid1, const char *msgid2, unsigned long int n);
char *dcngettext(const char *domainname, const char *msgid1, const char *msgid2, unsigned long int n, int category);
char *textdomain(const char *domainname);
char *bindtextdomain(const char *domainname, const char *dirname);
char *bind_textdomain_codeset(const char *domainname, const char *codeset);
#ifdef __cplusplus
}
#endif
#endif
EOF
    cat > "$b/libintl.c" <<'EOF'
/* proxy-libintl style stubs: pass msgid through untranslated. */
char *gettext(const char *m){ return (char*)m; }
char *dgettext(const char *d, const char *m){ (void)d; return (char*)m; }
char *dcgettext(const char *d, const char *m, int c){ (void)d;(void)c; return (char*)m; }
char *ngettext(const char *s, const char *p, unsigned long n){ return (char*)(n==1?s:p); }
char *dngettext(const char *d, const char *s, const char *p, unsigned long n){ (void)d; return (char*)(n==1?s:p); }
char *dcngettext(const char *d, const char *s, const char *p, unsigned long n, int c){ (void)d;(void)c; return (char*)(n==1?s:p); }
char *textdomain(const char *d){ return (char*)(d?d:"messages"); }
char *bindtextdomain(const char *d, const char *dir){ (void)d; return (char*)dir; }
char *bind_textdomain_codeset(const char *d, const char *c){ (void)d; return (char*)c; }
EOF
    $CC $CFLAGS -shared -Wl,-soname,libintl.so -o "$PREFIX/lib/libintl.so" "$b/libintl.c"
    cp "$b/libintl.h" "$PREFIX/include/libintl.h"
    verify_arch libintl
}

dep_ki18n()       { kf_fetch ki18n;        kf_build ki18n-$KF_VER ki18n \
    -DBUILD_WITH_QML=OFF \
    -DLibIntl_INCLUDE_DIRS="$PREFIX/include" -DLibIntl_LIBRARIES="$PREFIX/lib/libintl.so";
    verify_arch libKF5I18n; }
dep_kcompletion() { kf_fetch kcompletion;  kf_build kcompletion-$KF_VER kcompletion
    verify_arch libKF5Completion; }

# --- freetype + harfbuzz: Krita's text stack (both REQUIRED) ------------------
dep_freetype() {
    # First pass without HarfBuzz to break the freetype<->harfbuzz cycle.
    fetch freetype-2.13.2.tar.xz \
        "https://download.savannah.gnu.org/releases/freetype/freetype-2.13.2.tar.xz" \
        "https://download.sourceforge.net/freetype/freetype-2.13.2.tar.xz"
    extract freetype-2.13.2.tar.xz freetype-2.13.2
    cmake_build freetype-2.13.2 freetype \
        -DBUILD_SHARED_LIBS=ON \
        -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BROTLI=ON -DFT_DISABLE_BZIP2=ON \
        -DFT_REQUIRE_ZLIB=ON -DFT_REQUIRE_PNG=ON \
        -DZLIB_INCLUDE_DIR="$SYSROOT/usr/include" \
        -DZLIB_LIBRARY="$SYSROOT/usr/lib/$TARGET/libz.so" \
        -DPNG_PNG_INCLUDE_DIR="$PREFIX/include" -DPNG_LIBRARY="$PREFIX/lib/libpng.so"
    verify_arch libfreetype
}
dep_fontconfig() {
    fetch fontconfig-2.14.2.tar.xz \
        "https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.14.2.tar.xz"
    extract fontconfig-2.14.2.tar.xz fontconfig-2.14.2
    # Built with Meson (autotools config.status is broken on the macOS host).
    meson_build fontconfig-2.14.2 fontconfig \
        -Ddoc=disabled -Dtests=disabled -Dtools=disabled -Dnls=disabled \
        -Dcache-build=disabled
    verify_arch libfontconfig
}

dep_harfbuzz() {
    fetch harfbuzz-8.5.0.tar.gz \
        "https://codeload.github.com/harfbuzz/harfbuzz/tar.gz/refs/tags/8.5.0"
    extract harfbuzz-8.5.0.tar.gz harfbuzz-8.5.0
    cmake_build harfbuzz-8.5.0 harfbuzz \
        -DHB_HAVE_FREETYPE=ON -DHB_HAVE_GLIB=OFF -DHB_HAVE_ICU=OFF \
        -DHB_BUILD_TESTS=OFF -DHB_BUILD_UTILS=OFF -DBUILD_SHARED_LIBS=ON \
        -DFREETYPE_INCLUDE_DIRS="$PREFIX/include/freetype2" \
        -DFREETYPE_LIBRARY="$PREFIX/lib/libfreetype.so"
    verify_arch libharfbuzz
}

# --- fribidi: Unicode bidirectional text (REQUIRED by text) -------------------
dep_fribidi() {
    fetch fribidi-1.0.13.tar.gz \
        "https://codeload.github.com/fribidi/fribidi/tar.gz/refs/tags/v1.0.13"
    extract fribidi-1.0.13.tar.gz fribidi-1.0.13
    meson_build fribidi-1.0.13 fribidi \
        -Ddocs=false -Dtests=false -Dbin=false
    verify_arch libfribidi
}

# --- libunibreak: Unicode line/word/grapheme breaking (REQUIRED by text) -----
dep_libunibreak() {
    fetch libunibreak-5.1.tar.gz \
        "https://codeload.github.com/adah1972/libunibreak/tar.gz/refs/tags/libunibreak_5_1"
    extract libunibreak-5.1.tar.gz libunibreak-libunibreak_5_1
    local s="$SRC_DIR/libunibreak-libunibreak_5_1/src"
    local b="$BUILD_DIR/libunibreak"; rm -rf "$b"; mkdir -p "$b"
    # Tiny C library — compile its sources directly (its autotools configure
    # trips the same macOS config.status bug as fontconfig). Use the exact
    # SOURCES list from src/Makefile.am: linebreakdata.c IS a standalone unit
    # (it defines lb_prop_bmp), while emoji/grapheme/wordbreak data.c are
    # #included by their owning .c and must NOT be compiled standalone.
    local _csrc=""
    for u in unibreakbase unibreakdef linebreak linebreakdata linebreakdef \
             emojidef graphemebreak wordbreak; do
        _csrc="$_csrc $s/$u.c"
    done
    ( cd "$b" && $CC $CFLAGS -c $_csrc )
    ( cd "$b" && $CC -shared -Wl,-soname,libunibreak.so -o "$PREFIX/lib/libunibreak.so" *.o $LDFLAGS )
    cp "$s"/*.h "$PREFIX/include/"
    cat > "$PREFIX/lib/pkgconfig/libunibreak.pc" <<EOF
prefix=$PREFIX
libdir=\${prefix}/lib
includedir=\${prefix}/include
Name: libunibreak
Description: Unicode line/word/grapheme breaking
Version: 5.1
Libs: -L\${libdir} -lunibreak
Cflags: -I\${includedir}
EOF
    verify_arch libunibreak
}

# --- immer / zug / lager: header-only libs Krita REQUIRES (undo/state) -------
dep_xsimd() {
    fetch xsimd-11.1.0.tar.gz \
        "https://codeload.github.com/xtensor-stack/xsimd/tar.gz/refs/tags/11.1.0"
    extract xsimd-11.1.0.tar.gz xsimd-11.1.0
    cmake_build xsimd-11.1.0 xsimd -DBUILD_TESTS=OFF
    [ -d "$PREFIX/include/xsimd" ] && log "OK xsimd headers" || die "xsimd not installed"
}
dep_immer() {
    fetch immer.tar.gz "https://codeload.github.com/arximboldi/immer/tar.gz/refs/tags/v0.8.1"
    extract immer.tar.gz immer-0.8.1
    cmake_build immer-0.8.1 immer \
        -Dimmer_BUILD_TESTS=OFF -Dimmer_BUILD_EXAMPLES=OFF \
        -Dimmer_BUILD_DOCS=OFF -Dimmer_BUILD_EXTRAS=OFF
    [ -d "$PREFIX/include/immer" ] && log "OK immer headers" || die "immer not installed"
}
dep_zug() {
    fetch zug.tar.gz "https://codeload.github.com/arximboldi/zug/tar.gz/refs/heads/master"
    extract zug.tar.gz zug-master
    cmake_build zug-master zug \
        -Dzug_BUILD_TESTS=OFF -Dzug_BUILD_EXAMPLES=OFF
    [ -d "$PREFIX/include/zug" ] && log "OK zug headers" || die "zug not installed"
}
dep_lager() {
    fetch lager.tar.gz "https://codeload.github.com/arximboldi/lager/tar.gz/refs/heads/master"
    extract lager.tar.gz lager-master
    cmake_build lager-master lager \
        -Dlager_BUILD_TESTS=OFF -Dlager_BUILD_EXAMPLES=OFF \
        -Dlager_BUILD_DOCS=OFF -Dlager_BUILD_DEBUGGER_EXAMPLES=OFF
    [ -d "$PREFIX/include/lager" ] && log "OK lager headers" || die "lager not installed"
}

# ==========================================================================
# Krita itself (tier4) — configure/build the real app against the cross deps.
# ==========================================================================
KRITA_PREFIX="$HERE/krita-prefix"

dep_kritaconf() {
    local b="$BUILD_DIR/krita"; mkdir -p "$b"
    local tflags="--target=$TARGET --sysroot=$SYSROOT -D_GNU_SOURCE"
    "$CMAKE_BIN" -S "$KRITA_ROOT" -B "$b" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
        -DOHOS_ARCH="$OHOS_ARCH" -DOHOS_PLATFORM=OHOS \
        -DCMAKE_MAKE_PROGRAM="$NINJA_BIN" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$KRITA_PREFIX" \
        -DCMAKE_PREFIX_PATH="$PREFIX;$QT_SDK" \
        -DCMAKE_FIND_ROOT_PATH="$SYSROOT;$PREFIX;$QT_SDK" \
        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH \
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=BOTH \
        -DCMAKE_C_FLAGS="$tflags" -DCMAKE_CXX_FLAGS="$tflags" \
        -DKF_IGNORE_PLATFORM_CHECK=TRUE \
        -DBUILD_FOR_OHOS=ON \
        -DBUILD_TESTING=OFF \
        -DKRITA_ENABLE_PCH=OFF \
        -DENABLE_PYTHON_2=OFF -DENABLE_UPDATERS=OFF \
        "$@"
    log "Krita configure done -> $b"
}

dep_krita() {
    dep_kritaconf "$@"
    "$CMAKE_BIN" --build "$BUILD_DIR/krita" --parallel
    "$CMAKE_BIN" --install "$BUILD_DIR/krita"
}

# ==========================================================================
# Package — assemble the HAP payload: every runtime .so into entry/libs/arm64-v8a
# and Krita's runtime data into entry resources/rawfile.
# ==========================================================================
do_package() {
    local HP="$(cd "$HERE/.." && pwd)"           # harmony_pc
    local LIBS="$HP/entry/libs/arm64-v8a"
    local RAW="$HP/entry/src/main/resources/rawfile"
    local KB="$BUILD_DIR/krita/bin"              # Krita build output
    mkdir -p "$LIBS" "$RAW"

    [ -f "$KB/libentry.so" ] || die "libentry.so not built — run: $0 krita (and the OHOS libentry CMake branch)"

    log "packaging libentry.so + Krita libs + plugins"
    # -a preserves the SONAME symlink chain (libX.so -> .so.20 -> .so.20.0.0);
    # the glob must include versioned files (*.so*), not just *.so.
    cp -a "$KB"/*.so* "$LIBS/"                   # libentry + libkrita* + plugins

    log "packaging dependency .so (deps/prefix)"
    cp -a "$PREFIX"/lib/*.so* "$LIBS/" 2>/dev/null || true

    log "packaging OHOS C++ runtime (libc++_shared.so)"
    # Every C++ .so (QPA plugin, Qt, KF, Krita) needs the OHOS libc++ runtime;
    # it is NOT on the device by default for app libs, so bundle it.
    cp -f "$OHOS_SDK_NATIVE/llvm/lib/$TARGET/libc++_shared.so" "$LIBS/" 2>/dev/null \
        || warn "libc++_shared.so not found in SDK"

    log "packaging Qt5 runtime libs"
    local q
    for q in Core Gui Widgets Network Concurrent Svg PrintSupport Sql Xml \
             DBus Qml Quick QuickWidgets QuickControls2 QuickTemplates2 QmlModels \
             OpenGL XmlPatterns Multimedia MultimediaWidgets; do
        cp -f "$QT_SDK"/lib/libQt5$q.so* "$LIBS/" 2>/dev/null || true
    done
    # Qt image-format + sqldriver plugins Krita needs at runtime.
    find "$QT_SDK/plugins" -name "*.so" \( -path "*imageformats*" -o -path "*sqldrivers*" \) \
        -exec cp -f {} "$LIBS/" \; 2>/dev/null || true

    log "packaging Krita runtime data -> rawfile/share"
    rm -rf "$RAW/share"
    cp -R "$KRITA_PREFIX/share" "$RAW/share" 2>/dev/null || \
        warn "no $KRITA_PREFIX/share — run: $0 krita (install) first"

    local n; n=$(ls "$LIBS"/*.so* 2>/dev/null | wc -l | tr -d ' ')
    log "OK packaged $n native libs into entry/libs/arm64-v8a"
}

# ==========================================================================
# Dispatcher
# ==========================================================================
KNOWN="png jpeg tiff expat lcms2 gif boost eigen exiv2 quazip \
ecm host_tools libintl kcoreaddons kconfig kguiaddons kwidgetsaddons kitemviews ki18n kcompletion \
freetype fontconfig harfbuzz fribidi libunibreak xsimd immer zug lager kritaconf krita"
TIER1="png jpeg gif tiff expat lcms2"
TIER2="boost eigen exiv2 quazip"
TIER3="ecm host_tools libintl kcoreaddons kconfig kguiaddons kwidgetsaddons kitemviews ki18n kcompletion"

[ $# -ge 1 ] || { echo "usage: $0 <dep...|tier1|list>"; echo "known: $KNOWN"; exit 1; }

case "${1:-}" in
    list) echo "$KNOWN"; exit 0 ;;
    tier1) set -- $TIER1 ;;
    tier2) set -- $TIER2 ;;
    tier3) set -- $TIER3 ;;
    package) do_package; exit 0 ;;
esac

for d in "$@"; do
    fn="dep_$d"
    if ! declare -F "$fn" >/dev/null; then die "unknown dep '$d' (known: $KNOWN)"; fi
    log "==== building $d ===="
    "$fn"
    log "==== done $d ===="
done
log "all requested deps installed into: $PREFIX"
