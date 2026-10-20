---
{
  "schema_version": 1,
  "id": "file-secret-store-guarantees",
  "title": "Identify file-backed secret storage by its actual protection",
  "topic_key": "security.file-secret-store-guarantees",
  "area": "security",
  "scope": "common",
  "frameworks": [],
  "summary": "Describe filesystem access control and replacement guarantees accurately and select the provider only when they meet policy.",
  "when_to_use": "A file-backed credential provider is mistaken for encrypted or hardware-protected storage.",
  "keywords": [
    "secure storage",
    "credentials",
    "密钥",
    "SharedPreferences",
    "keystore",
    "device ID",
    "secure storage unavailable",
    "credential fallback",
    "file secret store",
    "private file permissions",
    "provider registration",
    "密钥存储",
    "明文回退"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "An app-specific facade can preserve the same business API across platforms. Secure-storage package names and algorithm labels still require implementation review for the desired guarantees.",
  "related": [
    "secure-storage-backend-selection"
  ]
}
---

# Identify file-backed secret storage by its actual protection

## When to use

A file-backed credential provider is mistaken for encrypted or hardware-protected storage.

## Recommended approach

Describe filesystem access control and replacement guarantees accurately and select the provider only when they meet policy.

## Implementation steps

A profile-local file store can deliberately rely on directory/file permissions and complete-write replacement. That supplies filesystem access control and write-integrity behavior; it is not evidence of encryption or hardware-backed key protection. State that scope before selecting it as a fallback, validate key names and path containment, and distinguish missing records from denied access or invalid file types.

Trace the consuming OH host's provider selection. A shared library exporting a file-secret class does not mean the OH runtime registered or uses it. Preserve an explicit unavailable result when the required guarantees cannot be supplied instead of silently activating another platform's file-backed route.

## Conditions and alternatives

An app-specific facade can preserve the same business API across platforms. Secure-storage package names and algorithm labels still require implementation review for the desired guarantees.

If selecting a custom HUKS-backed adapter, verify the chosen algorithm's nonce/IV, authentication data and ciphertext serialization against that SDK's API. Do not assume the native output automatically includes a fixed-size IV prefix. Distinguish an already-existing key from other key-generation failures, and exercise round-trip and tampered-record rejection before relying on the store. An `AES-GCM` label alone does not establish that this protocol is implemented correctly.

For a HUKS GCM API requiring `HUKS_TAG_NONCE` and `HUKS_TAG_AE_TAG` for decryption, retain and pass those values using the documented format; an arbitrary ciphertext prefix assigned to `HUKS_TAG_IV` is not an equivalent protocol. Propagate authentication failure distinctly from a record that was never stored.

## Common mistakes

Preserve the operation-specific ownership, units and failure conditions described above when adapting this contract.

## Check it works

Verify access controls, key/path validation, missing versus denied records, interrupted replacement and actual OH provider selection. Confirm no encryption claim is made without that mechanism.
