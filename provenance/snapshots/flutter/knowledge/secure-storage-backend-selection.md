---
{
  "id": "secure-storage-backend-selection",
  "title": "Store credentials using an appropriate OH backend",
  "topic_key": "security.secure-storage-backend-selection",
  "area": "security",
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
  "summary": "Select an OH credential-storage implementation with the required guarantees and preserve those guarantees through fallback, error and migration paths.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "A credential store works on Android/iOS but has no OH implementation, or a fallback stores secrets using ordinary preferences or custom encryption."
}
---

# Store credentials using an appropriate OH backend

## When to use

A credential store works on Android/iOS but has no OH implementation, or a fallback stores secrets using ordinary preferences or custom encryption.

## Recommended approach

Select an OH credential-storage implementation with the required guarantees and preserve those guarantees through fallback, error and migration paths.

## Implementation steps

1. Define what is secret, how long it must persist and what access/deletion behavior the app requires.
2. Dispatch the shared storage interface to the actual OH backend and inspect its underlying protection mechanism.
3. Keep ordinary settings and session convenience state separate from protected credentials. UI masking is only presentation.
4. If the backend is unavailable, return an explicit failure or ask the user to reauthenticate; do not silently return plaintext or pretend persistence succeeded.
5. Plan migration from any old plaintext/custom store and remove obsolete copies only after the new record is verified.

## Conditions and alternatives

An app-specific facade can preserve the same business API across platforms. Secure-storage package names and algorithm labels still require implementation review for the desired guarantees.

If selecting a custom HUKS-backed adapter, verify the chosen algorithm's nonce/IV, authentication data and ciphertext serialization against that SDK's API. Do not assume the native output automatically includes a fixed-size IV prefix. Distinguish an already-existing key from other key-generation failures, and exercise round-trip and tampered-record rejection before relying on the store. An `AES-GCM` label alone does not establish that this protocol is implemented correctly.

For a HUKS GCM API requiring `HUKS_TAG_NONCE` and `HUKS_TAG_AE_TAG` for decryption, retain and pass those values using the documented format; an arbitrary ciphertext prefix assigned to `HUKS_TAG_IV` is not an equivalent protocol. Propagate authentication failure distinctly from a record that was never stored.

### Identify file-backed secret storage by its actual protection

A profile-local file store can deliberately rely on directory/file permissions and complete-write replacement. That supplies filesystem access control and write-integrity behavior; it is not evidence of encryption or hardware-backed key protection. State that scope before selecting it as a fallback, validate key names and path containment, and distinguish missing records from denied access or invalid file types.

Trace the consuming OH host's provider selection. A shared library exporting a file-secret class does not mean the OH runtime registered or uses it. Preserve an explicit unavailable result when the required guarantees cannot be supplied instead of silently activating another platform's file-backed route.

## Common mistakes

A constant device identifier is not secret key material. XOR/Base64 is not authenticated encryption. Preferences storage and masked keys should not be described as encrypted storage merely because they are behind a credential interface.

## Check it works

Test write/read/delete, denied/unavailable backend, restart, migration and error handling. Inspect the actual native protection mechanism and unintended plaintext copies.
