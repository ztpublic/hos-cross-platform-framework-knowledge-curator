---
{
  "schema_version": 1,
  "id": "native-curl-explicit-ca-bundle",
  "title": "Supply an accessible CA bundle to a native curl client",
  "topic_key": "network.native-curl-explicit-ca-bundle",
  "area": "network",
  "scope": "common",
  "frameworks": [],
  "summary": "Give the selected libcurl-based client an app-readable PEM trust bundle and keep certificate and hostname validation enabled.",
  "when_to_use": "HTTPS fails with CURL_PEER_FAILED_VERIFICATION in an OH sandbox even though the network is reachable and the server certificate should be trusted.",
  "applicability": "OH native clients whose selected curl build cannot discover an accessible default trust store. Applies to PC network access under the same sandbox/build conditions; other network engines may use system trust directly.",
  "keywords": [
    "CURL_PEER_FAILED_VERIFICATION",
    "CURLOPT_CAINFO",
    "caInfo",
    "PEM",
    "CA bundle",
    "Ktor Curl",
    "TLS",
    "certificate verification failed"
  ],
  "related": [
    "network-adapter-policy-propagation",
    "application-path-namespace"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Supply an accessible CA bundle to a native curl client

## When to use

HTTPS fails with CURL_PEER_FAILED_VERIFICATION in an OH sandbox even though the network is reachable and the server certificate should be trusted.

## Recommended approach

Give the selected libcurl-based client an app-readable PEM trust bundle and keep certificate and hostname validation enabled.

## Implementation steps

1. Inspect the actual engine, curl error and trust-store configuration. Separate missing roots from hostname mismatch, expiry, clock errors and an incomplete server chain.
2. Package a maintained PEM root bundle as a resource. Copy its bytes to an application-context filesDir path readable by native code before constructing the client.
3. Configure CURLOPT_CAINFO with that path, or the selected Ktor Curl engine's engine { caInfo = bundlePath } adapter. Keep peer and hostname verification active.
4. Propagate the path through the real native initializer and rebuild the client when the trust configuration changes. Report installation/configuration errors explicitly.
5. Version and update the bundle with the app. Treat private enterprise roots as explicit trust policy; do not disable validation to make a failing request succeed.

## Conditions and alternatives

This is conditional on the selected curl engine and package. A path visible to hdc shell may be inaccessible to the application. A custom bundle can change trust relative to the platform store; verify intended public and enterprise endpoints independently.

## Common mistakes

Do not set verification flags off, confuse a resource URI with an fopen-readable path, or assume INTERNET permission also supplies root certificates.

## Check it works

Test a valid chain, an untrusted root, wrong hostname and expired certificate. Confirm the native client succeeds only for permitted chains and still works after restart and a bundle update.
