
import android.databinding.tool.ext.capitalizeUS
import org.jetbrains.kotlin.gradle.ExperimentalKotlinGradlePluginApi
import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins {
    alias(libs.plugins.kotlinMultiplatform)
    alias(libs.plugins.androidApplication)
    alias(libs.plugins.composeMultiplatform)
    alias(libs.plugins.composeCompiler)
}

kotlin {
    androidTarget {
        @OptIn(ExperimentalKotlinGradlePluginApi::class) 
        compilerOptions {
            jvmTarget.set(JvmTarget.JVM_11)    
        }
    }

    listOf(
        iosX64(),      
        iosArm64(),    
        iosSimulatorArm64()
    ).forEach { iosTarget ->
        iosTarget.binaries.framework {
            baseName = "ComposeApp" 
            isStatic = true         
        }
        iosTarget.compilations.getByName("main") {
            compilerOptions.configure {
                freeCompilerArgs.add("-Xbinary=sanitizer=address")
            }
        }
    }
    

    // 配置OHOS（华为鸿蒙）多架构目标
    listOf(
        ohosArm64(),   // 真机 arm64
//        ohosX64()      // 模拟器/开发机 x64
    ).forEach { ohosTarget ->
        ohosTarget.binaries.sharedLib {
            baseName = "kn"
            export(libs.compose.multiplatform.export)
            linkerOpts("-lz")
            debuggable = true
        }
        ohosTarget.compilations.getByName("main") {
            compilerOptions.configure {
                // Address sanitizer is not supported for OHOS
                // freeCompilerArgs.add("-Xbinary=sanitizer=address")
            }
            val resource by cinterops.creating {
                defFile(file("src/ohosMain/cinterop/resource.def"))
                includeDirs(file("src/ohosMain/cinterop/include"))
            }
        }
    }

    sourceSets {
        androidMain.dependencies {
            implementation(libs.androidx.activity.compose)
            implementation(libs.androidx.lifecycle.viewmodelCompose)
            implementation(libs.androidx.lifecycle.runtimeCompose)
            implementation(libs.androidx.collection)
            implementation(libs.ktor.okhttp)
        }
        commonMain.dependencies {
            implementation(compose.runtime)
            implementation(compose.foundation)
            implementation(compose.material)
            implementation(compose.material3)
            implementation(compose.ui)
            implementation(compose.components.resources)
            implementation(compose.components.uiToolingPreview)

            implementation(libs.atomicFu)
            implementation(libs.kotlinx.coroutines.core)
            implementation(libs.kotlinx.datetime)
            implementation(libs.kotlinx.io.core)
            implementation(libs.kotlinx.serialization.core)
            implementation(libs.kotlinx.serialization.json)
            implementation(libs.hash.sha1)
            implementation(libs.hash.md)
            implementation(libs.encoding.core)
            implementation(libs.encoding.base64)

            implementation(libs.ktor.core)
            implementation(libs.ktor.logging)
            implementation(libs.ktor.negotiation)
            implementation(libs.ktor.serialization.json)

            implementation(libs.coil.ktor)
            implementation(libs.coil.core)
            implementation(libs.coil)
            implementation("androidx.annotation:annotation:1.8.0-KBA-001")

            implementation(libs.androidx.navigation.compose)
//            implementation(libs.resources)
        }

        val iosMain = sourceSets.create("iosMain").apply {
            dependsOn(commonMain.get())
        }
        iosMain.dependencies {
            implementation(libs.ktor.darwin)
        }
        val iosX64Main by getting {
            dependsOn(iosMain)
        }
        val iosArm64Main by getting {
            dependsOn(iosMain)
        }
        val iosSimulatorArm64Main by getting {
            dependsOn(iosMain)
        }

        // OHOS 共享（对应目录 src/ohosMain/，arm64/x64 共用）
        val ohosMain = sourceSets.create("ohosMain").apply {
            dependsOn(commonMain.get())
        }
        ohosMain.dependencies {
            api(libs.compose.multiplatform.export)
        }
        val ohosArm64Main by getting {
            dependsOn(ohosMain)
        }
        ohosArm64Main.dependencies {
            implementation(libs.ktor.curl.ohosarm64)
        }
//        val ohosX64Main by getting {
//            dependsOn(ohosMain)
//        }
    }
}

android {
    namespace = "com.example.cmp_hello"                      
    compileSdk = libs.versions.android.compileSdk.get().toInt()  

    defaultConfig {
        applicationId = "com.example.cmp_hello"                  
        minSdk = libs.versions.android.minSdk.get().toInt()     
        targetSdk = libs.versions.android.targetSdk.get().toInt()
        versionCode = 1                                        
        versionName = "1.0"                                    
    }
    packaging {
        resources {
            excludes += "/META-INF/{AL2.0,LGPL2.1}"
        }
    }
    buildTypes {
        getByName("release") {
            isMinifyEnabled = false            
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11     
        targetCompatibility = JavaVersion.VERSION_11    
    }
}

dependencies {
    debugImplementation(libs.compose.ui.tooling)    
}

// OHOS 渲染模式二选一：自渲染（Skia）或统一渲染（ohrender）。切换时只保留其一，另一行注释掉。
compose {
    ohos {
        skia("0.9.22.2-OH.0.1.2-07")           // 自渲染：Compose 用 Skia 自绘
        // ohrender("0.9.22.2-ohrende")       // 统一渲染：与 ArkUI 统一渲染管线；启用时注释上一行
    }
}


// 为不同类型(debug、release)OHOS构建注册Copy任务并发布到Harmony App目录
arrayOf("debug", "release").forEach { type ->
    tasks.register<Copy>("publish${type.capitalizeUS()}BinariesToHarmonyApp") {
        group = "harmony" // 归类到harmony任务组
        dependsOn("link${type.capitalizeUS()}SharedOhosArm64")
//        dependsOn("link${type.capitalizeUS()}SharedOhosArm64", "link${type.capitalizeUS()}SharedOhosX64")
        duplicatesStrategy = DuplicatesStrategy.INCLUDE
        into(rootProject.file("harmonyApp"))
        from("build/bin/ohosArm64/${type}Shared/libkn_api.h") {
            into("entry/src/main/cpp/include/")
        }
        from(project.file("build/bin/ohosArm64/${type}Shared/libkn.so")) {
            into("entry/libs/arm64-v8a/")
        }
//        from("build/bin/ohosX64/${type}Shared/libkn_api.h") {
//            into("entry/src/main/cpp/include/")
//        }
//        from(project.file("build/bin/ohosX64/${type}Shared/libkn.so")) {
//            into("entry/libs/x86_64/")
//        }
        val composeResourcePackage = "${rootProject.name}.${project.name.lowercase()}.generated.resources"
        from("src/commonMain/composeResources") {
            into("entry/src/main/resources/rawfile/composeResources/$composeResourcePackage/")
        }
    }
} 
