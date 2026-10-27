# OHOS：FileKit、dialogs、Compose、Coil 接入

当前 CPF 版本的支持范围：

| 模块 | ohosArm64 | ohosX64 |
| --- | --- | --- |
| filekit-core | 支持 | 支持 |
| filekit-dialogs | 支持 | 支持 |
| filekit-dialogs-compose | 支持 | 支持 |
| filekit-coil | 支持 | CPF Coil 3.3.0-1.0.0 未发布该架构 |

应用目录/图片压缩使用 API 16+。不需要改依赖版本。

## 1. 推荐接入：Kotlin 直接调用系统模块

应用依赖 Maven Local 的 `io.github.vinceglb:filekit-dialogs:0.12.0-1.0.0`，
或依赖 `filekit-dialogs-compose`。Core 文件读写和应用目录不需要显式初始化。

在现有宿主 NAPI 入口中把 **UIAbilityContext** 传给 Kotlin，在首次打开选择器之前调用：

```kotlin
import io.github.vinceglb.filekit.FileKit
import io.github.vinceglb.filekit.dialogs.init
import io.github.vinceglb.filekit.dialogs.dispose

// env 和 context 来自宿主现有的 NAPI 回调；必须在 ArkTS UI 线程调用。
FileKit.init(env, context)

// 同一 UIAbility 销毁时，通过现有宿主销毁入口调用。
FileKit.dispose()
```

`init` 会安装 CPF Main dispatcher、持有 context 的 NAPI 引用。系统模块按需加载，
不应在 `napi_module` 的模块注册 `Init(env, exports)` 中调用系统选择器。
`context` 必须是页面所属 UIAbilityContext，不能传 ApplicationContext 或只传 env。

例如宿主原本就有 `initPlatform`，只需让它接收额外参数：

```typescript
// EntryAbility.onCreate；initPlatform 是应用已有的宿主入口，不是库新增的 ArkTS API。
native.initPlatform(this.context);

// EntryAbility.onDestroy；入口内调用 FileKit.dispose()。
native.disposePlatform();
```

在其 C++ NAPI 回调中使用 `napi_get_cb_info` 取出 context 并转交 Kotlin 即可。
也可以让现有 `MainArkUIViewController` 接收 context，然后在创建 Compose 内容前初始化。
**不需要复制 FileKit 的 `.ets` 文件、注册 FileKit 专属 C++ 方法或引入 HAR。**

`FileKit.init(env, context)` 初始化的是 dialogs：文档、照片、保存、目录、相机、
打开文件与 HarmonyOS 分享。库内部使用 `napi_load_module_with_info` 加载系统 API，
直接创建对象、调用方法并将 Promise 转成挂起函数，不经过 JSON dispatcher。
Compose launcher 沿用相同实例；调用方式与其他平台一致。

## 2. Kotlin 调用

```kotlin
val selected = FileKit.openFilePicker(type = FileKitType.Image)
try {
    val bytes = selected?.readBytes()
    // 使用图片；也可以 selected.copyTo(FileKit.filesDir / selected.name) 持久保存。
} finally {
    selected?.close()
}

val destination = FileKit.openFileSaver("report", "txt")
try {
    destination?.writeString("Hello from OHOS") // 直接写入用户选择的位置。
} finally {
    destination?.close()
}
```

文件/图片选择器和保存对话框返回的 `PlatformFile` 持有已复制的文件描述符。
ArkTS 在回调返回后关闭原始描述符；Kotlin 的每个 source/sink 又持有独立副本。
请在文件不再使用时调用 OHOS 的 `PlatformFile.close()`，不用等待 GC 清理。
已经打开的流在 `PlatformFile.close()` 后仍然有效，仍需各自 `close()`/`use`。
不要一边关闭文件、一边在其他线程为它创建新流。

这类文件支持读写、大小、修改时间、MIME、复制到沙箱。它们没有可直接使用的
kotlinx-io 路径；不能恢复临时 URI 权限、创建路径书签或依赖路径序列化还原。
需要长期保存或跨应用启动使用时，先复制到 `FileKit.filesDir`。
对选择器文件执行按路径删除、原子移动、创建目录等操作会明确失败。

`rememberFilePickerLauncher`、`rememberFileSaverLauncher`、
`rememberDirectoryPickerLauncher`、`rememberCameraPickerLauncher` 和
`rememberShareFileLauncher` 都已接入 OHOS。文件需保持打开直到 UI/加载任务不再使用。

## 4. Coil

在应用的 ImageLoader 中注册：

```kotlin
ImageLoader.Builder(context)
    .components { addPlatformFileSupport() }
    .build()
```

随后使用官方 `coil3.compose.AsyncImage(model = platformFile, imageLoader = loader, ...)`。
OHOS 的新 Fetcher 流式读取普通文件及选择器描述符，不会在 mapper 中把整个图片读入内存。
旧的 `rememberPlatformFileCoilModel`/FileKit `AsyncImage` 兼容接口仍可用，但对
URI 文件会生成完整 ByteArray；新代码应使用注册后的官方 Coil API。

## 5. 系统行为和限制

- 文件类型、扩展名、多选上限会传给对应系统选择器。系统不支持自定义对话框标题，
  `title` 不生效；照片选择器不使用文档目录提示。
- 取消文档/照片选择返回 `null`/`Cancelled`，权限/平台错误按异常返回，不当成取消。
- 同时只允许一个系统交互；第二个请求会明确报错。取消 Kotlin 协程不会强行关闭
  已显示的系统 UI，其晚到结果会被丢弃，描述符由桥接关闭。
- 目录选择受设备能力限制，主要面向二合一设备。只有系统返回可直接访问的真实目录
  才会返回 `PlatformFile`；不把文件或不能访问的 URI 假装成目录。
- 相机使用 CameraPicker 并写入指定沙箱目标；相机结果码和真机取消行为仍需验证。
- `openFileWithDefaultApplication` 的错误是异步返回的，可使用
  `FileKitOpenFileSettings(onError = { ... })` 接收。
- 分享按需加载 HarmonyOS ShareKit 展示系统分享面板，返回不代表目标应用已接收完数据。
  OpenHarmony 设备若没有此系统模块，只有分享调用失败，不影响文档/照片选择器。
- `core.saveImageToGallery` 仍使用前一版的独立 handler；本桥接没有自动替它接相册写入。

系统 URI 读写依据：
[选择用户文件](https://github.com/openharmony/docs/blob/master/en/application-dev/file-management/select-user-file.md)、
[保存用户文件](https://github.com/openharmony/docs/blob/master/en/application-dev/file-management/save-user-file.md)。

## 直接 NAPI 模式验证（2026-09-15）

- `filekit-dialogs` 和 `filekit-dialogs-compose` 的 OHOS ARM64/X64 编译通过。
- dialogs 的 ARM64/X64 与多平台元数据已发布到 Maven Local，版本 `0.12.0-1.0.0`。
- Hachimi 消费该版本后，Kotlin shared library、C++、ArkTS 和签名 HAP 构建通过。
- ARM64 HarmonyOS 模拟器：七个系统模块及所需导出成员加载通过；文档保存窗口实际打开，
  保存 UTF-8 文本后用 Kotlin 持有的文件描述符读回校验通过；照片窗口实际打开，关闭后返回 null。
- 文档选择并读取同一文件通过；故意触发系统文件错误后仍能继续选择文件；协程取消后拒绝重叠请求，
  释放后的晚到回调安全结束，重复 dispose 通过。完整连续调用顺序已复测通过。
- 测试中曾观察到一次应用退出，未获得明确故障原因；单独文档选择与完整连续调用后续均未复现。
  不将这次观察归因为 FileKit 已修复的缺陷。
- 相机拍摄、目录选择、分享面板、多选和实机权限差异仍需在对应设备验证，模块加载成功不等于这些流程都已验证。

`tests/DirectNapiSmoke.kt` 是应用环境下的手动回归入口；初始化 FileKit 并展示主窗口后，
在 UI 线程调用 `startFileKitSmoke(env)`。按 HiLog `FileKitSmoke` 提示操作系统选择器。
此测试会创建 `filekit-direct-smoke.txt`，结束后需自行删除。不要将入口放入正式启动流程。

## 基础文件操作验证

使用 JDK 21：

```sh
./gradlew :filekit-coil:compileKotlinOhosArm64 \
  :filekit-dialogs-compose:compileKotlinOhosArm64 \
  :filekit-dialogs-compose:compileKotlinOhosX64 \
  :filekit-core:compileTestKotlinOhosArm64 \
  :filekit-core:compileTestKotlinOhosX64 --offline

./gradlew :filekit-core:linkDebugTestOhosArm64 \
  :filekit-core:linkDebugTestOhosX64 \
  -Pkotlin.native.cacheKind.ohosArm64=none \
  -Pkotlin.native.cacheKind.ohosX64=none --offline

```

已完成：以上编译/链接和直接 NAPI 模式验证。宿主测试使用 SDK 编译器和系统 API mock，
**不等同于真机测试**。可用 `OHOS_ETS_SDK` 指定测试用 SDK 的 `ets` 目录。

此前基础文件操作 Native 测试仅编译和链接，未执行；以上直接 NAPI 模式另有模拟器验证。接入应用后需要验证：选择和取消、
权限拒绝、多选、保存后在文件管理器检查内容、相机、打开文件、分享、URI 图片加载、
UIAbility 销毁时未完成请求的取消。`OhosDescriptorTest` 可在设备 shell 中使用可写
`TMPDIR` 运行，默认 `/data/local/tmp`。
