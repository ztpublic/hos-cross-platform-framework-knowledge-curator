package io.kamel.core

import io.kamel.core.cache.Cache
import io.kamel.core.cache.LruCache
import io.kamel.core.cache.createKamelHttpClient
import io.kamel.core.config.ResourceConfig
import io.kamel.core.fetcher.HttpUrlFetcher
import io.kamel.core.mapper.StringMapper
import io.ktor.client.*
import io.ktor.client.request.*
import io.ktor.http.*
import io.ktor.utils.io.*
import kotlinx.coroutines.InternalCoroutinesApi
import kotlinx.coroutines.internal.SynchronizedObject
import kotlinx.coroutines.internal.synchronized
import kotlin.coroutines.CoroutineContext
import kotlin.coroutines.EmptyCoroutineContext

/**
 * Non-Compose entry point added by the OpenHarmony adaptation.
 *
 * `KamelConfig` and the whole decode/display pipeline live in
 * `src/composeMain` and are not compiled for the `ohosArm64`/`ohosX64`
 * targets. This object keeps the reusable core of the library usable from
 * Kotlin/Native on HarmonyOS: string-to-[Url] mapping, an in-memory LRU
 * byte cache, an HTTP fetcher with a disk-backed Ktor [HttpCache], and
 * [HttpUrlFetcher] for the network transfer itself.
 *
 * Decoding is intentionally out of scope here; on HarmonyOS the raw bytes
 * are handed to the ArkTS side which decodes them with the system image
 * kit (`image.createImageSource`).
 *
 * Call [configure] once (with the application cache directory) before the
 * first [loadBytes] call. The HTTP engine is resolved from the classpath,
 * e.g. `ktor-client-curl` on HarmonyOS.
 */
@OptIn(InternalCoroutinesApi::class)
public object Kamel {

    public const val DefaultMemoryCacheSize: Int = 100

    public const val DefaultHttpCacheSize: Long = 10L * 1024 * 1024 // 10 MiB

    private val lock = SynchronizedObject()

    private var diskCacheDir: String? = null
    private var memoryCacheSize: Int = DefaultMemoryCacheSize
    private var httpCacheSize: Long = DefaultHttpCacheSize
    private var caBundlePath: String? = null

    private var memoryCache: Cache<Url, ByteArray>? = null
    private var httpClient: HttpClient? = null

    /**
     * Initializes the loader. Must be called before the first [loadBytes].
     *
     * @param diskCacheDir an existing writable directory used for the HTTP disk cache.
     * @param memoryCacheSize number of entries kept in the in-memory LRU cache.
     * @param httpCacheSize maximum size in bytes of the HTTP disk cache.
     * @param caBundlePath optional PEM file with CA root certificates for the
     *   HTTPS engine. Ignored on JVM. On HarmonyOS the curl engine has no
     *   access to the system trust store, so a bundle file must be provided
     *   (e.g. a Mozilla CA bundle shipped in the application's rawfile and
     *   copied into the sandbox) or HTTPS calls fail with a peer
     *   verification error.
     */
    public fun configure(
        diskCacheDir: String,
        memoryCacheSize: Int = DefaultMemoryCacheSize,
        httpCacheSize: Long = DefaultHttpCacheSize,
        caBundlePath: String? = null,
    ): Unit = synchronized(lock) {
        val clientStale = this.diskCacheDir != diskCacheDir || this.caBundlePath != caBundlePath
        val memorySizeChanged = this.memoryCacheSize != memoryCacheSize
        this.diskCacheDir = diskCacheDir
        this.memoryCacheSize = memoryCacheSize
        this.httpCacheSize = httpCacheSize
        this.caBundlePath = caBundlePath
        if (clientStale) {
            httpClient?.close()
            httpClient = null
        }
        if (memorySizeChanged) {
            memoryCache = null
        }
    }

    /** Directory passed to [configure], or `null` if [configure] has not been called yet. */
    public val configuredDiskCacheDir: String?
        get() = synchronized(lock) { diskCacheDir }

    /** Directory passed to [configure]; throws if [configure] has not been called yet. */
    internal fun requireDiskCacheDir(): String =
        synchronized(lock) { diskCacheDir }
            ?: error("Kamel.configure(diskCacheDir = ...) must be called before the first load")

    /** Removes every entry from the in-memory cache. */
    public fun clearMemoryCache(): Unit = synchronized(lock) { memoryCache?.clear() }

    /** Current number of entries in the in-memory cache. */
    public val memoryCacheEntries: Int
        get() = synchronized(lock) { memoryCache?.size ?: 0 }

    /** Configured maximum number of in-memory cache entries. */
    public val memoryCacheMaxSize: Int
        get() = synchronized(lock) { memoryCacheSize }

    /**
     * Loads [data] (an http/https URL, or a `file://` URL via the shared mappers)
     * and returns the raw resource bytes.
     *
     * Memory cache hits return [Resource.Success] with [DataSource.Memory];
     * everything else goes through [HttpUrlFetcher] (progress events are
     * collapsed into the final result) with the disk-backed HTTP cache enabled.
     */
    public suspend fun loadBytes(
        data: String,
        request: HttpRequestBuilder.() -> Unit = {},
    ): Resource<ByteArray> {
        val url = StringMapper.map(data)

        synchronized(lock) { memoryCache?.get(url) }?.let { cached ->
            return Resource.Success(cached, DataSource.Memory)
        }

        val resourceConfig = RawResourceConfig(
            requestData = HttpRequestBuilder().apply(request).build(),
            coroutineContext = EmptyCoroutineContext,
        )

        var outcome: Resource<ByteArray>? = null
        HttpUrlFetcher(client()).fetch(url, resourceConfig).collect { resource ->
            when (resource) {
                is Resource.Loading -> outcome = resource
                is Resource.Success<ByteReadChannel> -> {
                    val bytes = resource.value.toByteArray()
                    val cache = synchronized(lock) {
                        memoryCache ?: LruCache<Url, ByteArray>(memoryCacheSize).also { memoryCache = it }
                    }
                    cache[url] = bytes
                    outcome = Resource.Success(bytes, resource.source)
                }
                is Resource.Failure -> outcome = resource
            }
        }
        return outcome ?: Resource.Failure(
            IllegalStateException("Fetcher for $url completed without a result")
        )
    }

    private fun client(): HttpClient = synchronized(lock) {
        httpClient ?: createKamelHttpClient(
            httpCacheSize = httpCacheSize,
            caBundlePath = caBundlePath,
        ).also { httpClient = it }
    }

    private class RawResourceConfig(
        override val requestData: HttpRequestData,
        override val coroutineContext: CoroutineContext,
    ) : ResourceConfig
}
