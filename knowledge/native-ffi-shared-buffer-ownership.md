---
{
  "schema_version": 1,
  "id": "native-ffi-shared-buffer-ownership",
  "topic_key": "native.ffi-shared-buffer-ownership",
  "scope": "framework-specific",
  "frameworks": [
    "flutter"
  ],
  "title": "Keep shared Rust–Dart FFI buffers alive until their last consumer finishes",
  "area": "native",
  "summary": "Separate the byte allocation from its ownership handles, seal producer buffers before sharing, and retain native ownership before asynchronous use or VM delivery.",
  "when_to_use": "A native OH Rust–Dart bridge moves large binary payloads and copying is expensive, or a retained typed-data view outlives its native allocation.",
  "keywords": [
    "FFI",
    "Rust",
    "Bytes",
    "Uint8List",
    "NativeApi.postCObject",
    "NativeFinalizer",
    "Finalizable",
    "external typed data",
    "shared buffer",
    "use after free",
    "zero copy",
    "共享缓冲",
    "悬空指针",
    "重复释放"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Flutter OH with the selected embedding/plugin versions; PC behavior requires the stated acceptance checks.",
  "related": [
    "chunked-native-file-transfer",
    "rust-bridge-target-and-packaging"
  ]
}
---

# Keep shared Rust–Dart FFI buffers alive until their last consumer finishes

## When to use

A native OH Rust–Dart bridge moves large binary payloads and copying is expensive, or a retained typed-data view outlives its native allocation.

## Recommended approach

Separate the byte allocation from its ownership handles. Give every asynchronous consumer an independently retained immutable allocation, and release each handle exactly once through the applicable explicit or VM finalizer path. A pointer and length alone do not convey ownership.

## Implementation steps

1. Version the native ABI and match Dart function signatures, pointer widths, frame layout, allocator and release callbacks. Resolve this against the packaged native library; see [Rust bridge packaging](rust-bridge-target-and-packaging.md). Only accept handles originating in the live native runtime, never arbitrary external numeric addresses.
2. Define a producer lifecycle such as `allocate → fill → seal → share → release`. Permit mutation only before sealing. A refcounted Rust allocation such as `Bytes` can retain immutable storage while separate handles track its consumers; do not expose its mutable pointer after publication.
3. For ordinary Dart bytes, make any necessary initial copy explicit. For a previously shared buffer or subrange, preserve its owner, offset and length, and reject handles from another runtime or allocator domain. Validate frame and range arithmetic before reading memory.
4. During Dart-to-Rust submission, keep the Dart owner reachable through synchronous native retention. Rust must retain the allocation before the FFI call returns if queued work will use it later. The asynchronous job owns that reference independently from the request's metadata allocation.
5. For Rust-to-Dart delivery through `NativeApi.postCObject`, use the selected VM's external typed-data ABI and a native release callback with an ownership token. If posting fails, release ownership that was not transferred. If a received view is adopted under a new owner, retain it while its original delivery receipt is still alive.
6. Give a Dart external view a finalizer tied to its own native handle, and expose an unmodifiable view when the protocol promises immutable bytes. Use weak identity bookkeeping if caching shared views; a strong global view cache prevents collection and release.
7. Keep connection lifetime separate from buffer lifetime. Closing a connection should fail pending requests, cancel subscriptions and release its session owner once, while already-retained buffers remain valid for their consumers. Native queued tasks must hold the runtime state they still need.
8. Test cleanup on every failed ownership transfer. Do not release a transferred handle twice, free a queued buffer immediately after submission, or attach two release paths to the same unretained owner.

## Conditions and alternatives

This requires a compatible native Dart VM/FFI backend, matching packaged library and trusted in-process ABI. It is not a pointer transport for a browser, a different process or an arbitrary MethodChannel peer. Keep a supported copying or [bounded chunk protocol](chunked-native-file-transfer.md) where shared ownership cannot be guaranteed.

An immutable Dart wrapper alone does not make native memory immutable. Every alias and native writer must follow the same sealing rule. Finalizer timing is not a deadline for releasing scarce sessions; use explicit connection cleanup and keep finalizers as the ownership backstop appropriate to that resource.

## Common mistakes

A byte slice can outlive its parent view unless it retains the allocation owner. Reusing a pointer-shaped integer after native release is unsafe even if the address has not changed. A successfully shared buffer does not prove that serialization, initial ingress or every downstream consumer avoids copying.

## Check it works

Test delayed native consumption after Dart drops its reference, subranges, empty buffers, foreign-runtime rejection, malformed envelopes, failed port delivery, disposal with work in flight and collection after the last view is dropped. Instrument retain/release counts and memory over repeated large transfers. Measure copy counts and end-to-end performance before making zero-copy or speed claims.
