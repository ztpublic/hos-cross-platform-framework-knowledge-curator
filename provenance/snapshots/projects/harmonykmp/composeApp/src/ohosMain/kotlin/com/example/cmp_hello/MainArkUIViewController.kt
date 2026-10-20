package com.example.cmp_hello

import androidx.compose.ui.window.ComposeArkUIViewController
import com.example.cmp_hello.global.registerGlobalCrashHandler
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.coroutines.initMainHandler
import kotlin.experimental.ExperimentalNativeApi
import platform.ohos.napi_env
import platform.ohos.napi_value

@OptIn(ExperimentalNativeApi::class, ExperimentalForeignApi::class)
@CName("MainArkUIViewController")
fun MainArkUIViewController(env: napi_env): napi_value {
    registerGlobalCrashHandler()
    initMainHandler(env)
    return ComposeArkUIViewController(env) {
        OhosRootApp()
    }
}

@OptIn(ExperimentalNativeApi::class, ExperimentalForeignApi::class)
@CName("initResourceManager")
fun initResourceManager(resourceManager: NativeResourceManager) {
    nativeResourceManager = resourceManager
}
