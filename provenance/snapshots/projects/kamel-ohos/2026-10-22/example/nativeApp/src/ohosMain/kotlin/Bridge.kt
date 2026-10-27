@file:OptIn(ExperimentalForeignApi::class, ExperimentalNativeApi::class)

package example.nativeapp

import io.kamel.core.DataSource
import io.kamel.core.Kamel
import io.kamel.core.Resource
import io.ktor.client.request.*
import kotlin.experimental.ExperimentalNativeApi
import kotlin.io.encoding.Base64
import kotlin.io.encoding.ExperimentalEncodingApi
import kotlin.time.TimeSource
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeoutOrNull

/**
 * `KamelInit` — call once from ArkTS before the first [KamelLoad].
 *
 * Request:  {"cacheDir":"/path","memoryCacheSize":100,"diskCacheBytes":10485760}
 * Response: {"ok":true,"cacheDir":"/path"} | {"ok":false,"error":"..."}
 */
@CName("KamelInit")
fun kamelInit(config: String): String {
    return try {
        val json = MiniJson.parse(config)
        val cacheDir = json.str("cacheDir")
        if (cacheDir.isBlank()) {
            error("cacheDir is required")
        }
        Kamel.configure(
            diskCacheDir = cacheDir,
            memoryCacheSize = json.int("memoryCacheSize", 100).coerceIn(0, 1000),
            httpCacheSize = json.long("diskCacheBytes", 10L * 1024 * 1024).coerceIn(0L, 512L * 1024 * 1024),
            caBundlePath = json.str("caBundlePath").ifBlank { null },
        )
        """{"ok":true,"cacheDir":${jsonString(cacheDir)}}"""
    } catch (t: Throwable) {
        """{"ok":false,"error":${jsonString(t.message ?: t.toString())}}"""
    }
}

/**
 * `KamelLoad` — synchronously loads the bytes for a URL and returns them
 * Base64-encoded together with cache metadata. Run it on a background
 * thread (ArkTS `taskpool`) so the UI thread is not blocked.
 *
 * Request:  {"url":"https://...","headers":"a:1;b:2","timeoutMs":15000}
 *           `headers` entries are `name:value` pairs joined by `;`
 *           (flat on purpose: the bridge JSON parser is intentionally tiny).
 * Response: {"ok":true,"url":"...","source":"memory|disk|network",
 *            "byteCount":123,"elapsedMs":45,"base64":"..."}
 *         | {"ok":false,"url":"...","error":"..."}
 */
@OptIn(ExperimentalEncodingApi::class)
@CName("KamelLoad")
fun kamelLoad(request: String): String {
    val json = MiniJson.parse(request)
    val url = json.str("url")
    val started = TimeSource.Monotonic.markNow()
    return try {
        if (url.isBlank()) {
            error("url is required")
        }
        val headers = parseHeaders(json.str("headers"))
        val timeoutMs = json.long("timeoutMs", 15_000L).coerceIn(1_000L, 120_000L)
        val outcome = runBlocking {
            withTimeoutOrNull(timeoutMs) {
                Kamel.loadBytes(url) {
                    headers.forEach { (name, value) -> header(name, value) }
                }
            }
        }
        val elapsed = started.elapsedNow().inWholeMilliseconds
        when (outcome) {
            null -> failure(url, "timed out after ${timeoutMs}ms", elapsed)
            else -> when (outcome) {
                is Resource.Success -> {
                    val bytes = outcome.value
                    buildString {
                        append("{")
                        append("\"ok\":true,")
                        append("\"url\":${jsonString(url)},")
                        append("\"source\":${jsonString(outcome.source.jsonName())},")
                        append("\"byteCount\":${bytes.size},")
                        append("\"elapsedMs\":$elapsed,")
                        append("\"base64\":${jsonString(Base64.Default.encode(bytes))}")
                        append("}")
                    }
                }
                is Resource.Failure -> failure(url, outcome.exception.message ?: outcome.exception.toString(), elapsed)
                is Resource.Loading -> failure(url, "load ended in loading state", elapsed)
            }
        }
    } catch (t: Throwable) {
        failure(url, t.message ?: t.toString(), started.elapsedNow().inWholeMilliseconds)
    }
}

/**
 * `KamelCacheInfo` — current in-memory cache state, for the demo status row.
 *
 * Response: {"ok":true,"memoryEntries":3,"memoryMax":100,"diskCacheDir":"/path"|null}
 */
@CName("KamelCacheInfo")
fun kamelCacheInfo(): String {
    return try {
        val dir = Kamel.configuredDiskCacheDir
        buildString {
            append("{")
            append("\"ok\":true,")
            append("\"memoryEntries\":${Kamel.memoryCacheEntries},")
            append("\"memoryMax\":${Kamel.memoryCacheMaxSize},")
            append("\"diskCacheDir\":${jsonString(dir ?: "")}")
            append("}")
        }
    } catch (t: Throwable) {
        """{"ok":false,"error":${jsonString(t.message ?: t.toString())}}"""
    }
}

private fun DataSource.jsonName(): String = when (this) {
    DataSource.Memory -> "memory"
    DataSource.Disk -> "disk"
    DataSource.Network -> "network"
    DataSource.None -> "none"
}

private fun parseHeaders(raw: String): List<Pair<String, String>> =
    raw.split(';').mapNotNull { entry ->
        val idx = entry.indexOf(':')
        if (idx <= 0) return@mapNotNull null
        entry.substring(0, idx).trim() to entry.substring(idx + 1).trim()
    }.filter { it.first.isNotEmpty() }

private fun failure(url: String, message: String, elapsedMs: Long): String = buildString {
    append("{")
    append("\"ok\":false,")
    append("\"url\":${jsonString(url)},")
    append("\"error\":${jsonString(message)},")
    append("\"elapsedMs\":$elapsedMs")
    append("}")
}

private fun jsonString(value: String): String {
    val escaped = value.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", "\\n").replace("\r", "\\r")
    return "\"$escaped\""
}

private class MiniJson(private val map: Map<String, String>) {
    fun str(key: String): String = map[key].orEmpty()
    fun int(key: String, default: Int): Int = map[key]?.toIntOrNull() ?: default
    fun long(key: String, default: Long): Long = map[key]?.toLongOrNull() ?: default

    companion object {
        fun parse(raw: String): MiniJson {
            val map = LinkedHashMap<String, String>()
            val body = raw.trim().removePrefix("{").removeSuffix("}")
            val parts = ArrayList<String>()
            val buf = StringBuilder()
            var inStr = false
            var escape = false
            for (ch in body) {
                when {
                    escape -> {
                        buf.append(ch)
                        escape = false
                    }
                    ch == '\\' && inStr -> {
                        buf.append(ch)
                        escape = true
                    }
                    ch == '"' -> {
                        inStr = !inStr
                        buf.append(ch)
                    }
                    ch == ',' && !inStr -> {
                        parts.add(buf.toString())
                        buf.clear()
                    }
                    else -> buf.append(ch)
                }
            }
            if (buf.isNotEmpty()) parts.add(buf.toString())
            for (part in parts) {
                val idx = part.indexOf(':')
                if (idx <= 0) continue
                val key = part.substring(0, idx).trim().trim('"')
                var value = part.substring(idx + 1).trim()
                if (value.startsWith("\"") && value.endsWith("\"") && value.length >= 2) {
                    value = value.substring(1, value.length - 1)
                        .replace("\\\"", "\"")
                        .replace("\\\\", "\\")
                }
                map[key] = value
            }
            return MiniJson(map)
        }
    }
}
