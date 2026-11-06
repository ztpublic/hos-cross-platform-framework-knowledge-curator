/**
 * Eazytec is pleased to support the open source community by making CPF-KMP-CMP available.
 * Copyright (C) 2026 Eazytec Co., Ltd. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.cpf.kmp.tools.ohos.debugger

import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths

/**
 * Collects host-side `.so` search directories for LLDB `target.exec-search-paths`.
 *
 * Primary root is DevEco ProcessLibs output under intermediates/libs; split-package fallbacks
 * include kmp HAR libs, entry/libs, and KMP build/bin debugShared outputs.
 */
object OhosExecSearchPathResolver {

    /** DevEco ProcessLibs aggregation dir (always listed first when present). */
    fun processLibsDir(ohRoot: Path, abiDir: String): Path =
        ohRoot.resolve("entry/build/default/intermediates/libs/default/$abiDir")

    fun resolveAbiDir(isWinHost: Boolean, isEmulator: Boolean): String =
        if (isWinHost && isEmulator) "x86_64" else "arm64-v8a"

    fun gradleKonanTargetForAbi(abiDir: String): String = when (abiDir) {
        "x86_64" -> "ohosX64"
        else -> "ohosArm64"
    }

    /**
     * Merge symbol search paths for single-so and split-package Harmony KMP projects.
     * Only existing directories are returned; order is stable and deduplicated.
     */
    fun collectExecSearchPaths(
        ohRoot: Path,
        projectBase: Path?,
        abiDir: String,
        kmpGradleModules: List<String>,
        cmakeLinkDirs: List<String> = emptyList(),
    ): List<String> {
        val normalizedOhRoot = ohRoot.toAbsolutePath().normalize()
        val paths = LinkedHashSet<String>()

        addIfDirExists(paths, processLibsDir(normalizedOhRoot, abiDir))
        discoverKmpHarLibDirs(normalizedOhRoot, abiDir).forEach { addIfDirExists(paths, it) }
        addIfDirExists(paths, normalizedOhRoot.resolve("entry/libs/$abiDir"))
        cmakeLinkDirs.forEach { dir ->
            addIfDirExists(paths, Paths.get(dir))
        }

        if (projectBase != null) {
            val konanTarget = gradleKonanTargetForAbi(abiDir)
            val normalizedBase = projectBase.toAbsolutePath().normalize()
            kmpGradleModules
                .map { it.trim() }
                .filter { it.isNotEmpty() }
                .distinct()
                .forEach { module ->
                    addIfDirExists(
                        paths,
                        normalizedBase.resolve("$module/build/bin/$konanTarget/debugShared"),
                    )
                }
        }

        return paths.toList()
    }

    /** Directories named `kmp*` under harmonyApp that contain `libs/{abi}`. */
    internal fun discoverKmpHarLibDirs(ohRoot: Path, abiDir: String): List<Path> {
        val root = ohRoot.toFile()
        if (!root.isDirectory) return emptyList()
        return root.listFiles()
            .orEmpty()
            .asSequence()
            .filter { it.isDirectory && it.name.startsWith("kmp", ignoreCase = true) }
            .map { it.toPath().resolve("libs/$abiDir") }
            .sortedBy { it.toString().lowercase() }
            .toList()
    }

    private fun addIfDirExists(paths: LinkedHashSet<String>, dir: Path) {
        val normalized = dir.toAbsolutePath().normalize()
        if (Files.isDirectory(normalized)) {
            paths.add(normalized.toString().trimEnd('/', '\\'))
        }
    }
}
