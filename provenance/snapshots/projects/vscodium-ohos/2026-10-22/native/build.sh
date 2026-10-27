#!/bin/bash
# Cross-compile pure N-API native modules for aarch64-unknown-linux-ohos
# (Electron 37 / Node 22.17.0 runtime, V8 symbols NOT exported -> N-API only).
#
# Produces:
#   spdlog.node      (@vscode/spdlog 0.15.2)
#   keymapping.node  (OHOS stub for @vscodium/native-keymap)
#
# Deps (already fetched into deps/ by the commands at the bottom of this file):
#   node-v22.17.0 headers, node-addon-api 7.1.0, @vscode/spdlog 0.15.2 sources
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
DEPS="$HERE/deps"
ROOT="$(cd "$HERE/../.." && pwd)"

OHOS_NATIVE="${OHOS_NATIVE:-/home/nanqu/Library/Huawei/command-line-tools/sdk/default/openharmony/native}"
CXX="$OHOS_NATIVE/llvm/bin/aarch64-unknown-linux-ohos-clang++"
SYSROOT="$OHOS_NATIVE/sysroot"
RUNTIME="$ROOT/third_party/electron-runtime"

NODE_INC="$DEPS/node-v22.17.0/include/node"
NAPI_INC="$DEPS/node-addon-api"
SPDLOG_SRC="$DEPS/spdlog-src/package"

COMMON_FLAGS=(
  --target=aarch64-unknown-linux-ohos
  --sysroot="$SYSROOT"
  -fPIC -shared -O2 -std=c++17 -fvisibility=hidden
  -DNDEBUG
  -I"$NODE_INC" -I"$NAPI_INC"
  -DNAPI_DISABLE_CPP_EXCEPTIONS
  -DNODE_API_SWALLOW_UNTHROWABLE_EXCEPTIONS
)
LINK_FLAGS=(
  -L"$RUNTIME" -lelectron
)

echo "== building spdlog.node =="
"$CXX" "${COMMON_FLAGS[@]}" \
  -I"$SPDLOG_SRC/deps/spdlog/include" \
  "$SPDLOG_SRC/src/main.cc" "$SPDLOG_SRC/src/logger.cc" \
  "${LINK_FLAGS[@]}" -Wl,-soname,spdlog.node \
  -o "$HERE/spdlog.node"

echo "== building keymapping.node =="
"$CXX" "${COMMON_FLAGS[@]}" \
  "$HERE/src/keymapping.cc" \
  "${LINK_FLAGS[@]}" -Wl,-soname,keymapping.node \
  -o "$HERE/keymapping.node"

echo "== building pty.node (node-pty 1.1.0-beta33, optional) =="
"$CXX" "${COMMON_FLAGS[@]}" \
  "$DEPS/node-pty-src/package/src/unix/pty.cc" \
  "${LINK_FLAGS[@]}" -lutil -Wl,-soname,pty.node \
  -o "$HERE/pty.node"

STRIP="$OHOS_NATIVE/llvm/bin/llvm-strip"
for f in spdlog.node keymapping.node pty.node; do
  [ -f "$HERE/$f" ] && "$STRIP" --strip-unneeded "$HERE/$f"
done

echo "== verify =="
"$HERE/verify.sh"

# --- how deps/ was populated ---------------------------------------------
# curl -sL -o deps/node-headers.tar.gz \
#   https://registry.npmmirror.com/-/binary/node/v22.17.0/node-v22.17.0-headers.tar.gz
# tar xzf deps/node-headers.tar.gz -C deps/
# npm pack node-addon-api@7.1.0 --pack-destination deps/
# mkdir deps/node-addon-api && tar xzf deps/node-addon-api-7.1.0.tgz -C deps/node-addon-api --strip-components=1
# npm pack @vscode/spdlog@0.15.2 --pack-destination deps/
# mkdir deps/spdlog-src && tar xzf deps/vscode-spdlog-0.15.2.tgz -C deps/spdlog-src
# npm pack node-pty@1.1.0-beta33 --pack-destination deps/
# mkdir deps/node-pty-src && tar xzf deps/node-pty-1.1.0-beta33.tgz -C deps/node-pty-src
