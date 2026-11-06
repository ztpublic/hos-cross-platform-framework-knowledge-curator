---
{
  "schema_version": 1,
  "id": "rust-ohos-tls-without-openssl",
  "title": "Choose rustls for Rust HTTPS crates targeting OH",
  "topic_key": "native-toolchain.rust-ohos-tls-without-openssl",
  "area": "native-toolchain",
  "scope": "common",
  "frameworks": [],
  "summary": "Disable reqwest default features and select rustls-tls, so the dependency graph does not need an OpenSSL build for the OH target.",
  "when_to_use": "A Rust or Tauri project for an OH target fails building openssl-sys or native-tls, typically with OpenSSL headers or libraries not found.",
  "applicability": "Rust dependencies for aarch64-unknown-linux-ohos and similar targets. The lockfile confirms the selection in one project; the failure text is a claim in its documentation rather than an observed log. ring still needs an OH C cross-compiler.",
  "keywords": [
    "reqwest",
    "rustls-tls",
    "native-tls",
    "openssl-sys",
    "default-features",
    "ring",
    "cargo",
    "OHOS target",
    "HTTPS"
  ],
  "related": [
    "rust-ohos-target-cfg",
    "ohos-native-toolchain-inputs",
    "native-curl-explicit-ca-bundle"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Choose rustls for Rust HTTPS crates targeting OH

## When to use

A Rust or Tauri project for an OH target fails building openssl-sys or native-tls, typically with OpenSSL headers or libraries not found.

## Recommended approach

Disable reqwest default features and select rustls-tls, so the dependency graph does not need an OpenSSL build for the OH target.

## Implementation steps

1. Run cargo tree -i openssl-sys and -i native-tls for the OH target to find what brings them in.
2. Set reqwest = { version, default-features = false, features = ["json", "rustls-tls"] }, keeping the other features you need explicitly.
3. Repeat for other crates that have a native-tls default. Features are additive, so one dependency can bring it back.
4. Configure the C cross-compiler for ring, and check the lockfile no longer lists openssl or native-tls.

## Conditions and alternatives

A cross-built OpenSSL is valid if you must share it with other native code. TLS trust roots differ by feature choice; verify which root store you get, and that it works on the device.

## Common mistakes

Changing only the top-level crate while a transitive dependency keeps native-tls. Removing default features and losing features such as json or gzip. Assuming a successful build means certificates validate.

## Check it works

Build for the OH target with a clean cache, check the lockfile, then make HTTPS requests on a device to public and private CA endpoints and a bad-certificate endpoint.
