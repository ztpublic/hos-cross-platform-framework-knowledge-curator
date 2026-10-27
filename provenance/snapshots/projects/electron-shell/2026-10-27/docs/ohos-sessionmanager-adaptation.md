# Electron 在 OpenHarmony 大屏设备上的窗口能力适配问题

> 结论先行：远程真机启动崩溃的根因不是缺 `.so`、也不是缺符号，而是
> **该 OpenHarmony 设备只实现了 `SystemCapability.WindowManager.WindowManager.Core`，
> 没有实现 `SystemCapability.Window.SessionManager`**。
> Electron 运行时（ArkTS 壳 + 原生 `libelectron.so`）都依赖 SessionManager，
> 因此在当前设备上无法启动，且**无法通过修改本工程代码修复**。

## 1. 现象

设备：`192.168.137.211:8710`，`OpenHarmony-6.1.0.35`，API 23，`const.build.characteristics = default`。

| 阶段 | 结果 |
| --- | --- |
| 构建 | 成功 |
| 安装 | 成功 |
| 启动 | `aa start` 返回成功，1 秒内进程消失 |
| 崩溃类型 | ArkTS 崩溃（已修）→ 原生 `SIGSEGV`（无法修） |

原生崩溃特征（`/data/log/faultlog/faultlogger/cppcrash-org.fellow99.test.ElectronShell-*.log`）：

```text
Reason:Signal:SIGSEGV(SEGV_MAPERR)@000000000000000000  probably caused by NULL pointer dereference
Fault thread info:
Tid:xxxx, Name:CrBrowserMain
#00 pc 000000000743e758 /data/storage/el1/bundle/libs/arm64/libelectron.so
...
#03 pc 0000000002a6d188 ... (ElectronMain)
```

崩溃前内核日志：

```text
OhosScreen::ConvertDisplay display is invalid, display id: -1, display work area is empty: 1
```

多次启动偏移完全一致（`0x743e758`），可稳定复现。

## 2. 判定依据（SDK 侧）

### 2.1 syscap 对照

| 头文件 / 接口 | @syscap | 该设备是否具备 |
| --- | --- | --- |
| `native/sysroot/usr/include/window_manager/oh_display_manager.h`（`OH_NativeDisplayManager_*`） | `SystemCapability.WindowManager.WindowManager.Core` | ✅ |
| `native/sysroot/usr/include/window_manager/oh_window.h`（`OH_WindowManager_GetWindowProperties` 等） | `SystemCapability.Window.SessionManager` | ❌ |
| `@ohos.window.d.ts`：`setWindowDecorVisible` | `SystemCapability.Window.SessionManager` | ❌ |
| `@ohos.window.d.ts`：`getWindowStatus` | `SystemCapability.Window.SessionManager` | ❌ |
| `@ohos.window.d.ts`：`setWindowTitleButtonVisible` | `SystemCapability.Window.SessionManager` | ❌ |
| `@ohos.window.d.ts`：`setWindowTitleMoveEnabled` | `SystemCapability.Window.SessionManager` | ❌ |
| `@ohos.window.d.ts`：`setSpecificSystemBarEnabled` | `SystemCapability.Window.SessionManager` | ❌ |
| `@ohos.window.d.ts`：`setWindowLayoutFullScreen` | `SystemCapability.WindowManager.WindowManager.Core` | ✅ |

设备 `/system/etc/syscap.json`（共 290 项）中，Window 相关只有：

```text
SystemCapability.WindowManager.WindowManager.Core
```

对照组（本机 2in1，HarmonyOS，同一 HAP 可正常运行）：

```text
SystemCapability.Window.SessionManager
SystemCapability.WindowManager.WindowManager.Core
```

### 2.2 运行时 801 与 syscap 表完全一致

```text
tag: WebAbility --> Failed to listener for window status changes: {"code":801}
tag: WebAbility --> Failed to listener for window rect changes: {"code":801}
tag: WebAbility --> Failed to register callback: {"code":801}
tag: WebAbility --> Failed to set window decor visible: {"code":801}
tag: WebAbility --> Failed to register window title button change callback: {"code":801}
tag: WebAbility --> Failed to get window limits or get ui context:{"code":801}
```

`801 = Capability not supported`，即设备侧未提供对应 syscap。

### 2.3 原生层：库和符号都不缺

- `libelectron.so` 共 40 个 `NEEDED` 依赖，逐个比对设备 `/system/lib64`（含 `ndk/`、`platformsdk/`、`chipset-sdk-sp/`），
  **全部存在**（仅 `libadapter.so`、`libffmpeg.so` 属于应用自带）。排除“缺库”。
- `libnative_window_manager.so`、`libnative_display_manager.so` 均在 `/system/lib64/ndk/`，
  且 `OH_WindowManager_GetWindowProperties`、`OH_NativeDisplayManager_GetDefaultDisplayId` 等符号均已导出并有实现。
  排除“缺符号”。
- 但 `libelectron.so` 导入的窗口属性接口只有 `OH_WindowManager_GetWindowProperties`
  （`oh_window.h`，**SessionManager**），它正是内核获取窗口 `displayId` 的入口。

### 2.4 传参正确，问题在内核

应用侧日志（已加诊断打印）：

```text
setDefaultBounds windowRect={"left":0,"top":0,"width":1920,"height":1080}
                 drawableRect={"left":0,"top":0,"width":1920,"height":1080}
                 displayId=0 propDisplayId=0
```

随后内核仍报 `display id: -1`。说明 `-1` 来自 `libelectron` 内部调用
`OH_WindowManager_GetWindowProperties` 失败后的缺省值，与 ArkTS 入参无关。

即：**ArkTS 层可以用 try/catch 兜住 801，但原生层取不到 display 就会空指针崩溃。**

## 3. 本次已落地的代码改动

这些改动只解决“801 直接中断 ArkTS 进程”，不解决原生崩溃，在支持 SessionManager 的设备上行为不变。

| 文件 | 改动 |
| --- | --- |
| `web_engine/src/main/ets/ability/WebAbility.ets` | `setWindowDecorVisible` 加能力判断与 try/catch；`setResizeByDragEnabled` 补 `.catch`；标题按钮/拖拽/模态整块仅在 `2in1`/`tablet` 上调用，`setWindowModal`、`setTitleAndDockHoverShown` 补 `.catch` |
| `web_engine/src/main/ets/components/WebNodeHandleWindow.ets` | `getWindowStatus` 包 try/catch（801 降级）；`displayId` 缺失时回落到默认屏幕；新增 `setDefaultBounds` 入参日志 |
| `web_engine/src/main/ets/components/WebWindow.ets` | `getWindowStatus` 包 try/catch |

## 4. 结论与可选方案

**根因**：设备构建未实现窗口子系统 SessionManager 部分（窗口装饰、自由窗口、窗口属性 NDK），
Electron 运行时（`libelectron.so` 预编译二进制）强依赖它。

| 方案 | 说明 | 是否本仓库可控 |
| --- | --- | --- |
| A. 换/升级设备系统 | 使用实现了 `Window.SessionManager` 的 OpenHarmony/HarmonyOS 构建（手机/平板/2in1 形态），或由厂商在 TV 类产品配置中使能 window session 子系统 | 否 |
| B. 运行时适配 | 由 Electron 运行时对无 SessionManager 设备做 display 降级（用 `OH_NativeDisplayManager_GetDefaultDisplayId` 兜底） | 否（预编译二进制） |
| C. 本机 2in1 验证 | 本机具备两个 syscap，功能验证可走本机 | 是 |

## 5. 复现/验证命令

```bash
# 设备 syscap
hdc -t <serial> shell "grep -o 'SystemCapability.Window[A-Za-z.]*' /system/etc/syscap.json | sort -u"

# 系统版本
hdc -t <serial> shell "param get const.ohos.fullname; param get const.ohos.apiversion"

# 启动并抓关键日志
hdc -t <serial> shell "aa start -a EntryAbility -b org.fellow99.test.ElectronShell"
hdc -t <serial> shell "hilog -x | grep -E 'setDefaultBounds|OhosScreen|code.:801' | tail -20"

# 崩溃日志
hdc -t <serial> shell "ls -t /data/log/faultlog/faultlogger | head -3"
```

> 备注：本机 `devecocli`（Node v24）在该平台 JIT 会崩，需带 `NODE_OPTIONS=--jitless` 执行。
