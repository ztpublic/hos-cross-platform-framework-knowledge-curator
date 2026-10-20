# 将开源 Qt 文本编辑器 NotePad--（Ndd）移植到鸿蒙 PC：思路与实践

本文结合当前仓库 `notepad--_ohos`，说明如何把基于 Qt Widgets 的开源项目 **NotePad--**（常称 Ndd）从传统桌面形态，迁移为可在 **鸿蒙 PC / 平板** 上安装的 HAP 应用。核心结论：**先在 Windows 上交叉编译出面向 OpenHarmony 的 `libNotePad--.so`，再在 DevEco Studio 的 ArkTS 工程里加载该库并完成打包与签名。**

---

## 一、为什么鸿蒙上的 Qt 和传统 PC 不一样

| 维度 | 传统 Qt PC 应用 | 鸿蒙上的 Qt 应用（本方案） |
|------|-----------------|---------------------------|
| 产物形态 | 多为独立可执行文件（如 Windows 下 `.exe`） | 业务 UI 与逻辑编译为 **`.so` 动态库** |
| 工程入口 | Qt Creator / CMake 直接生成可执行目标 | CMake 仍负责 **原生库**；**Ability / Stage / 生命周期** 由 DevEco Studio 的 ArkTS 模板承载 |
| 运行方式 | 操作系统直接启动进程 | 系统启动 ArkTS 应用进程，再由 **Qt on OHOS**（如 `libqohos.so`）在运行时 **加载** 你的 `libNotePad--.so` |

因此，移植的“分水岭”不是把 ArkTS 重写一遍，而是：**把原先 `add_executable` 的主程序，在 OHOS 交叉编译条件下改为 `add_library(… SHARED …)`，产出与 ArkTS 侧约定的库名一致的原生库**，其余壳层使用官方或模板提供的 Qt 嵌入式工程即可。

---

## 二、本仓库中的代码布局（便于对照）

- **鸿蒙工程根目录**：`notepad--_ohos/`（DevEco Studio 打开的工程）。
- **Qt 侧源码（NotePad-- 主工程）**：`Doc/src_qt_prj/notepad---main/`  
  > 说明：若你本地或文档中写作 `notepad--_main`，请以 `Doc/src_qt_prj/` 下**实际文件夹名**为准；本仓库为 `notepad---main`。
- **ArkTS 中声明要加载的库名**：`entry/src/main/ets/common/QtAppConstants.ets`  

```1:3:entry/src/main/ets/common/QtAppConstants.ets
export const APP_LIBRARY_NAME = 'libNotePad--.so';
export const LOG_DOMAIN = 0x0000;
export const LOG_TAG = 'ohosNotePad--';
```

- **Qt 运行时初始化入口（模板侧）**：`entry/src/main/ets/qabilitystage/QAbilityStage.ets` 中通过 `qpa.setupQtApplication({ … appName: APP_LIBRARY_NAME … })` 把 `APP_LIBRARY_NAME` 传给原生层，用于加载对应 `.so`。

---

## 三、移植总流程（建议按顺序执行）

1. **准备环境**  
   - DevEco Studio / HarmonyOS SDK（含 OpenHarmony **native** 工具链与 `ohos.toolchain.cmake`）。  
   - **面向鸿蒙的 Qt 5.12** 构建树（示例路径下文以 `C:/Qt/qt-5.12.12-ohos` 表示），需与目标 ABI（如 `arm64-v8a`）一致。

2. **修改 Qt 工程 CMake**  
   在 `OHOS` 条件下将主目标从可执行文件改为共享库（本仓库已改好，可作参考）。

3. **使用 CMake + OHOS 工具链配置并编译**  
   在**单独的 build 目录**中执行 `cmake` 配置与 `cmake --build`，得到 `libNotePad--.so`。

4. **集成到 DevEco 工程**  
   将 `libNotePad--.so` 及 Qt 运行依赖（如各 `libQt5*.so`、平台插件等，按 Qt-for-OHOS 文档放置）放到 entry 模块约定位置，保证与 `abiFilters`（如 `arm64-v8a` / `x86_64`）一致。

5. **确认 ArkTS 常量**  
   `APP_LIBRARY_NAME` 与 CMake `PROJECT_NAME` / 生成的 `lib` 前缀、`--` 等特殊字符命名一致（本工程为 `'libNotePad--.so'`）。

6. **Build HAP → 签名 → 安装到鸿蒙 PC 真机**  
   使用 DevEco Studio 正常打包流程即可。

---

## 四、CMake 侧关键改动（生成 `.so` 而非 `.exe`）

本仓库在 `Doc/src_qt_prj/notepad---main/CMakeLists.txt` 中，用 `OHOS` 宏区分鸿蒙交叉编译：鸿蒙下使用 **`add_library(… SHARED …)`**，其它平台保持原有 `add_executable` 逻辑。

```45:55:Doc/src_qt_prj/notepad---main/CMakeLists.txt
if(OHOS) # 鸿蒙交叉编译环境下
    message(STATUS ">>>> 当前模式: 鸿蒙交叉编译 (生成 SHARED 库) <<<<")
    add_library(${PROJECT_NAME} SHARED ${SRC} ${UI_SRC} ${PROJECT_SOURCE_DIR}/src/RealCompare.qrc)
elseif(CMAKE_HOST_WIN32)
    # 原有的 Windows 可执行文件逻辑
    list(APPEND WIN_RCS ${PROJECT_SOURCE_DIR}/src/RealCompareToMinGw.rc)
    add_executable(${PROJECT_NAME} WIN32 ${WIN_RCS} ${SRC} ${UI_SRC} ${PROJECT_SOURCE_DIR}/src/RealCompare.qrc)
else()
    message(STATUS ">>>> 当前模式: 其他平台编译 <<<<")
    add_executable(${PROJECT_NAME} ${SRC} ${UI_SRC} ${PROJECT_SOURCE_DIR}/src/RealCompare.qrc)
endif()
```

要点：

- **`project(NotePad-- …)`** 与 **`add_library(${PROJECT_NAME} SHARED …)`** 共同决定生成的库文件名一般为 **`libNotePad--.so`**（与 `QtAppConstants.ets` 中字符串一致）。
- 仍通过 `find_package(Qt5 …)` 与 `target_link_libraries` 链接 Qt 与 `qscint` 等子工程；鸿蒙下只要 **Qt 与工具链均为 OHOS 目标** 即可链接通过。

---

## 五、编译 `libNotePad--.so`：命令行示例

以下路径需按你本机 **SDK 安装位置**、**Qt OHOS 根目录** 修改。示例与仓库内 `Doc/src_qt_prj/notepad---main/build/build_ohos.bat` 思路一致：指定 **OHOS 的 `cmake.exe`**、**`ohos.toolchain.cmake`**、**`-DOHOS_ARCH`** 以及 **Qt5 的 `CMAKE_PREFIX_PATH` / `Qt5_DIR`**。

在 `notepad---main` 下新建或进入 `build` 目录后执行：

**1）配置（Generate）**

```bat
"D:\oh\DevEcoStudio\sdk\HarmonyOS-NEXT-DB6\openharmony\native\build-tools\cmake\bin\cmake.exe" ^
  -DCMAKE_PREFIX_PATH="C:/Qt/qt-5.12.12-ohos" ^
  -DCMAKE_TOOLCHAIN_FILE="D:/oh/DevEcoStudio/sdk/HarmonyOS-NEXT-DB6/openharmony/native/build/cmake/ohos.toolchain.cmake" ^
  -DQt5_DIR="C:/Qt/qt-5.12.12-ohos/lib/cmake/Qt5" ^
  -DCMAKE_FIND_ROOT_PATH="C:/Qt/qt-5.12.12-ohos" ^
  -DOHOS_ARCH=arm64-v8a ^
  ..
```

**2）构建**

```bat
"D:\oh\DevEcoStudio\sdk\HarmonyOS-NEXT-DB6\openharmony\native\build-tools\cmake\bin\cmake.exe" --build .
```

成功后在 build 目录（或 CMake 配置的输出目录）下可找到 **`libNotePad--.so`**。

可选：本仓库已提供批处理 **`Doc/src_qt_prj/notepad---main/build/build_ohos.bat`**，其中包含 `SDK_PATH`、`QT_ROOT`、`ARCH` 等变量，可按机器路径改完后双击或在命令行执行，完成“配置 + Ninja 构建”两步。

---

## 六、与 DevEco 工程对接时要注意什么

1. **库名与 ArkTS 常量一致**  
   修改或核对 `export const APP_LIBRARY_NAME = 'libNotePad--.so';`，必须与磁盘上的主业务库文件名一致（含 `lib` 前缀与 `.so` 后缀）。

2. **ABI 一致**  
   `entry/build-profile.json5` 中的 `abiFilters` 需包含你编译 `.so` 时使用的架构（如真机多为 `arm64-v8a`，模拟器可能为 `x86_64`）。  
   若只编了 `arm64-v8a`，却打开了 `x86_64` 过滤，需在对应 ABI 目录补齐同名的 `.so` 或调整过滤项。

3. **Qt on OHOS 运行时依赖**  
   `QAbilityStage.ets` 会 `import qpa from 'libqohos.so'`，再通过 `setupQtApplication` 加载你的业务库。除 `libNotePad--.so` 外，通常还需按 Qt-for-OHOS 要求部署 **平台插件、Qt 核心库** 等；若缺失，日志中会提示与 **qpa / nettool.so** 等相关的加载问题，需对照官方说明补齐 `entry` 下各 ABI 的 `libs` 布局。

4. **HAP 打包与签名**  
   原生库随 entry 模块打入 HAP；使用 DevEco Studio 的 **自动签名** 或你的发布证书完成签名后，安装到鸿蒙 PC 真机验证。

---

## 七、小结

- **传统 Qt PC**：CMake/Qt Creator → **可执行文件**。  
- **鸿蒙 Qt 应用（本方案）**：同一套 Qt 业务代码 → CMake 在 **OHOS 工具链 + Qt-for-OHOS** 下 → **`libNotePad--.so`** → DevEco **ArkTS 壳** 加载 → **HAP** 上真机运行。  

当前仓库已把 **CMake 的 OHOS 分支** 与 **ArkTS 的 `APP_LIBRARY_NAME`** 对齐；你只需按机器路径完成 **SDK/Qt 配置**、**编译出 `.so` 并放入工程约定位置**，即可在鸿蒙 PC 上完成端到端验证与后续迭代。

---

*文档生成日期：2026-04-21；与仓库 `notepad--_ohos` 源码结构同步描述。*
