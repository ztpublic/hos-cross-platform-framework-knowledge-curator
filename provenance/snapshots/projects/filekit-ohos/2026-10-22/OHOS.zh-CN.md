# HarmonyOS / OpenHarmony 移植

[English](OHOS.md)

这个分支包含 FileKit 的实验性 OHOS 移植。由于该移植依赖 CPF Kotlin/Native 工具链，
并且可能独立于上游 FileKit 演进，因此相关说明单独放在这里。

## 支持矩阵

| 模块 | `ohosArm64` | `ohosX64` |
| --- | --- | --- |
| `filekit-core` | 支持 | 支持 |
| `filekit-dialogs` | 支持 | 支持 |
| `filekit-dialogs-compose` | 支持 | 支持 |
| `filekit-coil` | 支持 | 暂未发布 |

当前移植基于 CPF Kotlin `2.2.21-1.0.0`。Core 使用 OHOS 应用上下文提供的沙箱目录。
Dialogs 通过 Kotlin/Native N-API 调用系统选择器，并返回基于复制后文件描述符的
`PlatformFile`。

## 应用接入

宿主 UIAbility 必须在第一次调用选择器之前初始化 FileKit，并且初始化必须发生在
ArkTS/UI 线程。应用通过已有的 N-API 入口把当前 `napi_env` 和页面所属的
`UIAbilityContext` 传给 Kotlin：

```kotlin
import io.github.vinceglb.filekit.FileKit
import io.github.vinceglb.filekit.dialogs.dispose
import io.github.vinceglb.filekit.dialogs.init

FileKit.init(env, context)

// 在同一个 UIAbility 的销毁回调中调用：
FileKit.dispose()
```

这里的 `context` 必须是页面的 `UIAbilityContext`，不能使用 ApplicationContext。
这种模式不需要复制 FileKit 的 `.ets` 文件，也不需要注册 FileKit 专用 dispatcher。

选择器返回的 `PlatformFile` 使用完后必须调用 `close()`。如果需要跨应用启动保存，
请先把文件复制到 `FileKit.filesDir`。

## 本地发布

```sh
./gradlew publishToMavenLocal -PRELEASE_SIGNING_ENABLED=false
```

消费端需要在远程仓库之前加入 `mavenLocal()`。版本号来自 `gradle.properties` 中的
`VERSION_NAME`。

## 验证状态

详细的宿主示例和手动测试说明见
[`samples/ohos-integration/README.md`](samples/ohos-integration/README.md)。当前已完成两种
OHOS 架构的编译，并在 ARM64 HarmonyOS 模拟器上验证了系统模块加载、保存/读取、文档选择、
照片选择取消、错误传播、请求串行化和资源释放。

相机拍摄、目录选择、分享、真实设备权限差异仍需要在对应设备上验证。
