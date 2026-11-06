---
{
  "schema_version": 1,
  "id": "ohos-certificate-fingerprint-pinning",
  "title": "Pin a certificate fingerprint on OH with a TLS socket probe",
  "topic_key": "network.ohos-certificate-fingerprint-pinning",
  "area": "network",
  "scope": "common",
  "frameworks": [],
  "summary": "If you need to compare a whole-certificate fingerprint, connect with a TLS socket and hash getRemoteCertificate, since http certificatePinning checks a public-key hash.",
  "when_to_use": "Whole-certificate SHA-1 or SHA-256 pins never match when using the http module's certificatePinning option.",
  "applicability": "OH network kit. A plugin write-up reports this behavior; no plugin source was reviewed. A separate probe connection does not guarantee that later requests reach the same server, so it is weaker than pinning on the real connection.",
  "keywords": [
    "certificate pinning",
    "fingerprint",
    "certificatePinning",
    "constructTLSSocketInstance",
    "getRemoteCertificate",
    "createMd",
    "GET_NETWORK_INFO",
    "2300001",
    "TLS"
  ],
  "related": [
    "native-curl-explicit-ca-bundle",
    "network-adapter-policy-propagation",
    "oauth-browser-callback-correlation"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run"
}
---

# Pin a certificate fingerprint on OH with a TLS socket probe

## When to use

Whole-certificate SHA-1 or SHA-256 pins never match when using the http module's certificatePinning option.

## Recommended approach

If you need to compare a whole-certificate fingerprint, connect with a TLS socket and hash getRemoteCertificate, since http certificatePinning checks a public-key hash.

## Implementation steps

1. First see whether a public-key SHA-256 pin through http certificatePinning meets your need. Prefer that as it applies to the actual request.
2. Otherwise resolve with connection.getAddressesByName, create socket.constructTLSSocketInstance, call bind before connect, and apply a timeout.
3. Read getRemoteCertificate().data and hash it with cryptoFramework.createMd. Compare with the normalized expected value in constant format.
4. Map errors separately: the source lists 2300001 for DNS, 2300003 for timeout and 2301001 to 2301006 for TLS failures. Close the socket every time.
5. Declare ohos.permission.INTERNET and GET_NETWORK_INFO; the source reports Permission denied without the second.

## Conditions and alternatives

Check before use and fail closed for security-relevant calls. Plan for certificate rotation by pinning more than one value.

## Common mistakes

Treating a probe success as protection for later requests. Comparing formats with different case or separators. Failing open on DNS errors.

## Check it works

Test matching and mismatching pins, an expired or rotated certificate, offline state, a timeout and a missing permission.
