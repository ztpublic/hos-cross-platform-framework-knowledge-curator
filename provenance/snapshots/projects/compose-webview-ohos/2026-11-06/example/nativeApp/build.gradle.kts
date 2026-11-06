plugins {
    alias(libs.plugins.kotlin.multiplatform)
}

kotlin {
    applyDefaultHierarchyTemplate()

    compilerOptions {
        optIn.add("kotlin.experimental.ExperimentalNativeApi")
    }

    ohosArm64 {
        binaries.sharedLib {
            baseName = "ohoswebview"
        }
    }

    ohosX64 {
        binaries.sharedLib {
            baseName = "ohoswebview"
        }
    }

    sourceSets {
        ohosMain.dependencies {
            implementation(project(":webview"))
        }
    }
}

fun cxxShared(triple: String): File {
    val deps = file("${System.getProperty("user.home")}/.konan/dependencies")
    val hits = deps.listFiles().orEmpty().map { File(it, "lib/$triple/libc++_shared.so") }.filter { it.isFile }
    check(hits.isNotEmpty()) { "libc++_shared.so for $triple not found under $deps" }
    return hits.first()
}

tasks.register<Copy>("copyKnSoArm64") {
    from(layout.buildDirectory.dir("bin/ohosArm64/debugShared")) {
        include("libohoswebview.so")
    }
    from(cxxShared("aarch64-linux-ohos"))
    into(rootProject.layout.projectDirectory.dir("example/ohos/entry/libs/arm64-v8a"))
}

tasks.register<Copy>("copyKnSoX64") {
    from(layout.buildDirectory.dir("bin/ohosX64/debugShared")) {
        include("libohoswebview.so")
    }
    from(cxxShared("x86_64-linux-ohos"))
    into(rootProject.layout.projectDirectory.dir("example/ohos/entry/libs/x86_64"))
}

tasks.matching { it.name == "linkDebugSharedOhosArm64" }.configureEach {
    finalizedBy("copyKnSoArm64")
}

tasks.matching { it.name == "linkDebugSharedOhosX64" }.configureEach {
    finalizedBy("copyKnSoX64")
}
