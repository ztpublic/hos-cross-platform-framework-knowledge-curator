@file:OptIn(ExperimentalForeignApi::class, ExperimentalResourceApi::class)

package com.example.cmp_hello

import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.toComposeImageBitmap
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.addressOf
import kotlinx.cinterop.usePinned
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.IO
import kotlinx.coroutines.withContext
import org.jetbrains.compose.resources.DrawableResource
import org.jetbrains.compose.resources.ExperimentalResourceApi
import org.jetbrains.skia.Image
import platform.resource.OH_ResourceManager_CloseRawFile
import platform.resource.OH_ResourceManager_GetRawFileSize
import platform.resource.OH_ResourceManager_OpenRawFile
import platform.resource.OH_ResourceManager_ReadRawFile

typealias NativeResourceManager = CPointer<cnames.structs.NativeResourceManager>?

var nativeResourceManager: NativeResourceManager = null

private val emptyImageBitmap: ImageBitmap by lazy { ImageBitmap(1, 1) }

@Composable
internal fun rememberLocalImage(id: DrawableResource): ImageBitmap {
    var imageBitmap: ImageBitmap by remember { mutableStateOf(emptyImageBitmap) }
    LaunchedEffect(id) {
        val manager = nativeResourceManager ?: return@LaunchedEffect
        withContext(Dispatchers.IO) {
            val rawFile = OH_ResourceManager_OpenRawFile(manager, id.resourceItemPath()) ?: return@withContext
            val size = OH_ResourceManager_GetRawFileSize(rawFile)

            val buffer = ByteArray(size.toInt())
            buffer.usePinned { pinnedBuffer ->
                OH_ResourceManager_ReadRawFile(rawFile, pinnedBuffer.addressOf(0), size.toULong())
            }
            OH_ResourceManager_CloseRawFile(rawFile)

            imageBitmap = Image.makeFromEncoded(buffer).toComposeImageBitmap()
        }
    }
    return imageBitmap
}
