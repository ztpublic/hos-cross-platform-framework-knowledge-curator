package io.kamel.core.cache

import io.ktor.client.HttpClient
import io.ktor.client.engine.curl.Curl
import io.ktor.client.plugins.cache.HttpCache

internal actual fun createKamelHttpClient(httpCacheSize: Long, caBundlePath: String?): HttpClient =
    HttpClient(Curl) {
        engine {
            // curl has no access to the HarmonyOS system trust store, so HTTPS
            // requires an explicit CA bundle (CURLOPT_CAINFO).
            if (!caBundlePath.isNullOrBlank()) {
                caInfo = caBundlePath
            }
        }
        install(HttpCache) {
            publicStorage(httpCacheStorage(httpCacheSize))
        }
    }
