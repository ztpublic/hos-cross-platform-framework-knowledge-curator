# ohosTexStudio 编译手册

> **基线**：HarmonyOS API 20（6.0.0.x）+ 开源 Qt 5.15.12（OHOS arm64）
> **最近实测**：2026-08-26，Ubuntu 22.04 x86_64 从零全量构建通过
> **最终产物**：`texstudio-hap/output/texstudio-signed.hap`（约 550M，含 miktex.hnp + openssh.hnp）

按本手册逐步执行，可从零构建出可安装的 HAP 包。下文 `NEXT` 指本仓克隆根目录（命令中请替换为实际路径）。

## 快速开始（TL;DR）

工具链与签名材料就绪后（首次请先完成第 0~3 步），完整构建只需：

```bash
export NEXT=/path/to/texstudionext
export OHOS_TOOLS_HOME="$NEXT/toolchain/command-line-tools"
export OHOS_NDK="$OHOS_TOOLS_HOME/sdk/default/openharmony/native"
export OHOS_QT5_HOME="$NEXT/toolchain/qt-sdk/Qt5.15.12-ohos20-arm64-v8a"
export PATH="$OHOS_NDK/build-tools/cmake/bin:$OHOS_NDK/llvm/bin:$OHOS_TOOLS_HOME/sdk/default/openharmony/toolchains:$PATH"

cd $NEXT/miktex     && ./build.sh all    && cd $NEXT   # Layer 1：几十分钟
cd $NEXT/texstudio  && ./build.sh build  && cd $NEXT   # Layer 2
bash $NEXT/texstudio-hap/build_hap.sh                  # Layer 3：打包+签名
```

✔ 成功标志：`texstudio-hap/output/texstudio-signed.hap` 生成且脚本末行验签输出 `verify-app success`。

## 第 0 步：主机环境

| 项 | 要求 |
|---|---|
| 操作系统 | Linux x86_64（实测 Ubuntu 22.04） |
| 磁盘 | 工作分区预留 ≥60G |
| 内存 | ≥16G 建议；不足时见第 4 步 JOBS/OOM 说明 |
| 权限 | sudo（装 apt 依赖）；构建本身不需要 root |
| 设备 | 一台鸿蒙设备（`hdc list targets` 可见），仅安装验证需要 |

依赖（以下为从零构建实测所需，一次装齐）：

```bash
# 工具组
sudo apt install git cmake ninja-build python3 perl make g++ \
  openjdk-17-jdk-headless zip unzip xz-utils curl patchelf pkg-config \
  autoconf automake libtool bison flex gettext xsltproc
# miktex native 宿主构建所需的 dev 库（同为实测所需）
sudo apt install libapr1-dev libaprutil1-dev libboost-all-dev libgraphite2-dev \
  libcurl4-openssl-dev libssl-dev libpng-dev libjpeg-dev zlib1g-dev libfreetype-dev \
  libharfbuzz-dev libicu-dev libpopt-dev liblzma-dev libexpat1-dev libfontconfig1-dev \
  libx11-dev libxext-dev libxi-dev libxrender-dev libxcb1-dev libxau-dev libxdmcp-dev \
  libffi-dev libsqlite3-dev libxml2-dev libxslt1-dev
```

> 备注：bison 若为无 sudo 的用户级安装，需额外 `export M4=$(which m4)` 并设置 BISON_PKGDATADIR。

## 第 1 步：获取代码

```bash
git clone --recurse-submodules <本仓地址> texstudionext
cd texstudionext
export NEXT=$PWD
```

✔ 成功标志：`miktex/`、`texstudio/`、`texstudio-hap/` 三个子模块目录非空。

## 第 2 步：工具链准备

### 2.1 command-line-tools（华为官方 6.0.0.878，开发者联盟网站下载）

```bash
mkdir -p $NEXT/toolchain
unzip commandline-tools-linux-x64-6.0.0.878.zip -d $NEXT/toolchain/
```

6.0.0.878 的 `sdk/default/openharmony` 下 ets/js/native/previewer/toolchains 已平铺，**无需软链**。仅旧版 SDK（目录未平铺时）需要：

```bash
# 仅旧版 SDK 需要，6.0.0.878 请跳过
cd $NEXT/toolchain/command-line-tools/sdk/default/openharmony
for n in ets js native previewer toolchains; do ln -sfn ../22/$n $n; done
```

### 2.1.1 解压后必须修复执行位与符号链接（实测踩坑，勿跳过）

unzip 会丢大量文件执行位（实测全树约 1416 个，含 es2abc/js2abc、hvigor/bin/*、toolchains/hdc、native/llvm 的工具与 .so；首个症状是 hvigor 构建报 `es2abc: Permission denied`）：

```bash
# 按文件类型批量补执行位（ELF 与 shebang 脚本）
find $NEXT/toolchain/command-line-tools -type f \
  -exec sh -c 'file -b "$1" | grep -qE "ELF|script" && chmod +x "$1"' _ {} \;
# 关键入口单独确认应有 x 位
ls -l $NEXT/toolchain/command-line-tools/tool/node/bin/node \
      $NEXT/toolchain/command-line-tools/hvigor/bin/hvigorw \
      $NEXT/toolchain/command-line-tools/sdk/default/openharmony/toolchains/hdc
```

更隐蔽的是 NDK 符号链接损坏：`native/llvm/bin/clang` 等（实测 36 个）解压后变成内容为目标文件名的普通文本文件（如 clang 是 8 字节文本 "clang-15"），交叉编译时报 `clang-15: error: no input files`。检测与修复：

```bash
OHOS_NDK_BIN=$NEXT/toolchain/command-line-tools/sdk/default/openharmony/native/llvm/bin
# 检测：应为 ELF 却是小文本文件的"伪软链"
find $OHOS_NDK_BIN -maxdepth 1 -type f -size -100c -exec sh -c \
  'head -c1 "$1" | grep -qE "[a-zA-Z./]" && ! file "$1" | grep -q ELF && echo "$1"' _ {} \;
# 修复：文件内容即目标名，逐个重建符号链接（示例）
cd $OHOS_NDK_BIN && ln -sf clang-15 clang
```

✔ 成功标志：`tool/node/bin/node --version` 正常输出版本号；`bash .agents/scripts/verify_arch.sh` 检查 24 PASS。

### 2.2 Qt SDK

仓内自带（`toolchain/qt-sdk/` 已入库），无需操作：

```bash
ls $NEXT/toolchain/qt-sdk/Qt5.15.12-ohos20-arm64-v8a/   # 确认存在
```

### 2.3 签名材料（不入库，需自行准备）

默认使用 debug 套，放置于：

```
toolchain/signing/debug/filewatcher.p12   （密钥库）
toolchain/signing/debug/debug.cer         （调试证书）
toolchain/signing/debug/debug.p7b         （调试 profile）
```

release 套放 `toolchain/signing/release/`，文件名对应 `release.cer` / `release.p7b`。
debug 套默认凭据（build_hap.sh 脚本默认值）：别名 `filewatcher-debug-key`，密码 `123456`。

## 第 3 步：环境变量

```bash
export OHOS_TOOLS_HOME="$NEXT/toolchain/command-line-tools"
export OHOS_NDK="$OHOS_TOOLS_HOME/sdk/default/openharmony/native"
export OHOS_QT5_HOME="$NEXT/toolchain/qt-sdk/Qt5.15.12-ohos20-arm64-v8a"
export PATH="$OHOS_NDK/llvm/bin:$OHOS_TOOLS_HOME/sdk/default/openharmony/toolchains:$PATH"
# cmake ≥3.23：miktex/CMakePresets.json 要求，Ubuntu 源 cmake 3.22.1 会在 ohos preset 配置时失败。
# 用 NDK 自带 cmake（实测 3.28.2）前置 PATH：
export PATH="$OHOS_NDK/build-tools/cmake/bin:$PATH"
```

✔ 成功标志：`cmake --version` ≥ 3.23；`which hdc node` 均指向 `$OHOS_TOOLS_HOME` 下。

## 第 4 步：三层构建（顺序不可换）

```bash
# Layer 1：MiKTeX（首次全量构建约几十分钟；all = native 宿主工具 + ohos 交叉编译 + ghostscript）
cd $NEXT/miktex && ./build.sh all && cd $NEXT
# ✔ 成功标志：miktex/build-ohos-arm64/sandbox/miktex/bin/ohos-aarch64/ 下约 316 个二进制，
#   且 build-ohos-arm64/ghostscript-stage/bin/gs 存在

# Layer 2：TeXstudio C++ 引擎
cd $NEXT/texstudio && ./build.sh build && cd $NEXT
# ✔ 成功标志：texstudio/build-ohos-arm64/libtexstudio.so 生成（aarch64，约 92M）

# Layer 3：ArkTS HAP 打包 + 签名
bash $NEXT/texstudio-hap/build_hap.sh
# ✔ 成功标志：texstudio-hap/output/texstudio-signed.hap 生成，验签输出 verify-app success；
#   bash .agents/scripts/verify_arch.sh 检查 25 PASS（miktex.hnp 引擎完整性）
```

注意事项：

- **内存/JOBS**：低内存机器（如 15G 内存、nproc=16）建议 `JOBS=8 ./build.sh all`，但 `JOBS` 仅作用于 native 宿主工具阶段（miktex/build.sh，另经环境继承作用于 ghostscript 的 make -j）；ohos-arm64 交叉编译走 `cmake --build --preset ohos-arm64`，其 buildPresets 写死 `"jobs": 0`（ninja 满核），不读 `JOBS`。ohos 阶段 OOM 时调 `JOBS` 无效，需改用 `cmake --build --preset ohos-arm64 --parallel 8` 断点续编（直接复跑该命令即可，**禁全量删 build 目录**）。
- **网络**：miktex ohos 阶段构建时（add_custom_command + gen-bootstrapping-files，build 期而非 configure 期执行）会用 curl 从 ctan.org 下载 7 个 bootstrapping 包（miktex-config-2.9/dvips/fontconfig/misc/zzdb1-2.9/zzdb3-2.9/tetex）。网络不通时 `MIKTEX_REPOSITORY` 镜像需在**首次 configure 前**设置（configure 时写入 cache，后续修改不生效）：`export MIKTEX_REPOSITORY=<镜像URL>`。
- **ghostscript**：源码树无预生成 configure，build 脚本自动跑 autogen.sh（依赖 autoconf/automake/libtool，见第 0 步）；源码内 autogen.sh 若同样丢了执行位需先 chmod +x。
- **HNP 打包暂存区外移（2026-08-26 重构）**：`build_hap.sh` 不再写 miktex 仓工作树。`[0/9]` 步骤每次将 `texstudio-hap/output/hnp_staging/` 整体重建：vendored 资产（词典/perl5/perl5xs/texmfs 静态数据）从 `miktex/hnp/assets/` 只读拷贝落位，引擎按 hnp.json install.links 清单从 miktex 构建产物全量拷入，缺失即 fail-loud 退出（P-76，verify 检查 25 校验包内容完整性）。打包前有垃圾护栏：bin/ 含 `*.a` 静态库、测试二进制（gmp-test 等）或重复的 th_en_US_v2.dat 时拒绝打包。miktex-texindy 无 CMake 目标架构产物，从 `assets/prebuilt/` 仓库预编译副本拷入。

## 第 5 步：安装与冒烟验证（需要设备）

包名（bundleName）配置在 `texstudio-hap/AppScope/app.json5`，不要硬编码，用以下方式读取：

```bash
BUNDLE=$(grep -o '"bundleName"[[:space:]]*:[[:space:]]*"[^"]*"' $NEXT/texstudio-hap/AppScope/app.json5 | cut -d'"' -f4)
echo "$BUNDLE"

# 安装（HNP 内容更新后须先卸载再装：hdc uninstall "$BUNDLE"）
hdc install -r $NEXT/texstudio-hap/output/texstudio-signed.hap
hdc shell "aa force-stop $BUNDLE; aa start -a EntryAbility -b $BUNDLE"

# 冒烟：触发一次引擎编译
hdc shell "aa start -a EntryAbility -b $BUNDLE --ps test_mode build_view"

# 查看运行日志（沙箱路径若不同，先 hdc shell ls 实测）
FILES=/data/app/el2/100/base/$BUNDLE/haps/entry/files
hdc shell "ls $FILES/"
hdc file recv $FILES/texstudio_all.log /tmp/
```

✔ 全链路通过标志：日志中出现 `Output written on test.pdf`。

## 第 6 步：release 包（可选）

签名材料放 `toolchain/signing/release/`（`filewatcher.p12` / `release.cer` / `release.p7b`），然后：

```bash
SIGN_TYPE=release KEY_ALIAS=<release密钥别名> KEY_PWD=<密码> KEYSTORE_PWD=<密码> \
  bash $NEXT/texstudio-hap/build_hap.sh
```

更多环境变量见 `texstudio-hap/build-texstudio-hap.md`。

## 增量构建（日常开发）

`build_hap.sh` 只做 ArkTS 编译 + 打包 + 签名，**不编译 C++**。修改 texstudio/miktex 的 C++ 源码后需先增量重编再打包：

```bash
cd $NEXT/texstudio/build-ohos-arm64 && ninja libtexstudio.so && cd $NEXT
bash $NEXT/texstudio-hap/build_hap.sh
```

## 常见问题（症状 → 原因 → 修复）

| 症状 | 原因 | 修复 |
|---|---|---|
| 改了 C++ 但行为没变 | build_hap.sh 不编译 C++ | 先 `ninja libtexstudio.so`（见增量构建） |
| HNP 安装报"软件包损坏"或命令找不到 | `hdc install -r` 不重解 HNP | 先 `hdc uninstall "$BUNDLE"` 再装 |
| hvigor 报 `es2abc: Permission denied` | unzip 丢执行位 | 按第 2.1.1 节批量修复 |
| miktex ohos 阶段报 `clang-15: error: no input files` | NDK 符号链接解压后变成文本文件 | 按第 2.1.1 节检测并 `ln -s` 重建 |
| cmake preset 配置报版本不足 | CMakePresets 要求 cmake ≥3.23，Ubuntu 源 3.22.1 不够 | PATH 前置 NDK 自带 cmake（见第 3 步） |
| miktex.hnp 缺引擎、verify 检查 25 报缺失 | 构建产物不完整 | 重跑 `miktex/build.sh ohos`；检查 25 列出的缺失清单逐一定位 |
| 打包报"HNP bin/ 含垃圾文件" | 垃圾护栏触发（暂存区/拷贝源被污染） | 护栏列出的文件不应进包，从来源处移除后重跑 build_hap.sh |

## 参考文档

- 各层细节：`miktex/build-miktex.md`、`texstudio/build-texstudio.md`、`texstudio-hap/build-texstudio-hap.md`
- 规则与事故库：`AGENTS.md`（P 规则）、`.agents/retros/incident_log.md`
- 实操速查（设备/日志/命令）：`CLAUDE.md`
- Qt SDK 想自己源码构建：Qt 补丁与源码构建脚本由工具链仓维护
