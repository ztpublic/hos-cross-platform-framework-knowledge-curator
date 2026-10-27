import org.jetbrains.kotlin.gradle.dsl.ExplicitApiMode

plugins {
    alias(libs.plugins.kotlin.multiplatform)
}

kotlin {
    compilerOptions.freeCompilerArgs.add("-Xexpect-actual-classes")
    explicitApi = ExplicitApiMode.Warning

    applyDefaultHierarchyTemplate()

    jvmToolchain(21)
    jvm()
    ohosArm64()
    ohosX64()

    sourceSets {
        val commonMain by getting {
            dependencies {
                implementation(libs.kotlinx.coroutines.core)
                implementation(libs.ktor.client.core)
                implementation(libs.okio)
            }
        }

        val commonTest by getting {
            dependencies {
                implementation(kotlin("test"))
                implementation(libs.kotlinx.coroutines.test)
                implementation(libs.okio.fakefilesystem)
            }
        }

        val commonJvmMain by creating {
            dependsOn(commonMain)
        }

        val commonJvmTest by creating {
            dependsOn(commonTest)
        }

        val jvmMain by getting {
            dependsOn(commonJvmMain)
        }

        val jvmTest by getting {
            dependsOn(commonJvmTest)
        }

        ohosMain.dependencies {
            implementation(libs.ktor.client.curl)
        }
    }
}
