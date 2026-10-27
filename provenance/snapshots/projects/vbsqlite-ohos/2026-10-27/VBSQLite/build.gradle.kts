import org.jetbrains.kotlin.gradle.ExperimentalKotlinGradlePluginApi
import org.jetbrains.kotlin.gradle.plugin.KotlinHierarchyTemplate
import org.jetbrains.kotlin.gradle.plugin.KotlinSourceSetTree
import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeTarget
import org.jetbrains.kotlin.konan.target.HostManager

plugins {
    alias(libs.plugins.kotlinMultiplatform)
    alias(libs.plugins.kotlinCocoapods)
    alias(libs.plugins.androidLibrary)
    alias(libs.plugins.kuiklyOpen)
}

apply(mapOf("from" to rootProject.file("multi-mvn-push.gradle")))

val enableOHOS = findProperty("HARMONY_ENABLED")?.toString()?.toBoolean() ?: false

@OptIn(org.jetbrains.kotlin.gradle.ExperimentalKotlinGradlePluginApi::class)
kotlin {
    explicitApi()
    // 使用默认层级结构模板
    KotlinHierarchyTemplate.default
    val nativeTargets = mutableListOf(
        iosX64(),
        iosArm64(),
        iosSimulatorArm64(),
    )
    if (enableOHOS) {
        nativeTargets.add(ohosArm64())
    }
    nativeTargets.forEach {
        it.setupNativeConfig()
    }

    targets.configureEach {
        compilations.configureEach {
            compilerOptions.configure {
                freeCompilerArgs.add("-Xexpect-actual-classes")
            }
        }
    }

    androidTarget {
        @OptIn(ExperimentalKotlinGradlePluginApi::class)
        instrumentedTestVariant.sourceSetTree.set(KotlinSourceSetTree.test)
        compilations.all {
            kotlinOptions {
                jvmTarget = "1.8"
            }
        }

        // 只发布 release 版本到 Maven
        publishLibraryVariants("release")
    }

    cocoapods {
        summary = "Some description for the Shared Module"
        homepage = "Link to the Shared Module homepage"
        version = "1.0.0"
        name = "VBSQLite"
        ios.deploymentTarget = "11.0"
        podfile = project.file("../iosApp/Podfile")
        publishDir = project.file("../framework")

        framework {
            baseName = "VBSQLite"
            isStatic = true
        }

        specRepos {
            url("https://git.woa.com/TencentVideoShared/VideoBase.git")
            url("https://mirrors.tencent.com/repository/generic/pod-go/T-CocoaPods/Specs/")
        }
    }

    sourceSets {
        val commonMain by getting {
            dependencies {
                api(libs.kotlinx.atomic)
                // core 不在 commonMain 声明，避免传播到 ohos cinterop configuration
                // android 的 core 在 androidMain 声明，native 的 core 在下方 targets.withType 中声明
            }
        }
        val androidMain by getting {
            dependencies {
                compileOnly("androidx.annotation:annotation:1.7.0")
                compileOnly("com.tencent.kuikly-open:core:${libs.versions.kuikly.open.get()}")
            }
        }
    }

    // 给所有 native 目标添加 kuikly core compileOnly 依赖（KSP 生成的 KuiklyCoreEntry 需要）
    targets.withType<KotlinNativeTarget>().configureEach {
        val coreVersion = if (name.startsWith("ohos")) {
            "${libs.versions.kuikly.open.get()}-ohos"
        } else {
            libs.versions.kuikly.open.get()
        }
        compilations.getByName("main") {
            defaultSourceSet {
                dependencies {
                    compileOnly("com.tencent.kuikly-open:core:$coreVersion")
                }
            }
        }
    }
}

android {
    namespace = "com.tencent.kmm.component.VBSQLite"
    compileSdk = 33
    defaultConfig {
        minSdk = 21
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }
}


tasks.configureEach {
    if (name == "kspCommonMainKotlinMetadata") {
        enabled = false
    }
}


tasks.configureEach {
    if (name.startsWith("ksp") && name != "kspCommonMainKotlinMetadata") {
        doLast {
            fileTree(layout.buildDirectory.dir("generated/ksp")).matching {
                include("**/KuiklyCoreEntry.kt")
            }.forEach { file ->
                file.delete()
                logger.lifecycle("Deleted KuiklyCoreEntry.kt from VBSQLite KSP output: ${file.path}")
            }
        }
    }
}

// kuiklyOpen 插件可能自动给 ohos 目标添加 core，但原始版本没有 ohos variant
// 用 resolutionStrategy 将 core 版本替换为 -ohos 版本
if (enableOHOS) {
    configurations.matching { name.lowercase().contains("ohos") }.configureEach {
        resolutionStrategy.eachDependency {
            if (requested.group == "com.tencent.kuikly-open" && requested.name == "core"
                && !requested.version.orEmpty().endsWith("-ohos")) {
                useVersion("${requested.version}-ohos")
            }
        }
    }
}


fun KotlinNativeTarget.setupNativeConfig() {
    val main by compilations.getting

    // 需要在shared里调来自外部C的时候加这个
    val interop by main.cinterops.creating {
        includeDirs("src/include")
    }

    compilations.forEach {
        it.kotlinOptions.freeCompilerArgs += when {
            HostManager.hostIsMac -> listOf(
                "-linker-options",
                "-lsqlite3.h -L${projectDir}/src/libs/"
            )

            else -> throw RuntimeException("暂不支持")
        }
        // 抑制 NativeApi 提示
        it.compilerOptions.options.optIn.addAll(
            "kotlinx.cinterop.ExperimentalForeignApi",
            "kotlin.experimental.ExperimentalNativeApi",
        )
    }
}