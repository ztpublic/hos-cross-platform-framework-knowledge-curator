---
{
  "schema_version": 1,
  "id": "native-wgpu-gles-adapter-limits",
  "title": "Clamp wgpu GLES device limits to what the OH adapter reports",
  "topic_key": "rendering.native-wgpu-gles-adapter-limits",
  "area": "rendering",
  "scope": "common",
  "frameworks": [],
  "summary": "On the GL backend, request the smaller of the baseline limit and the adapter limit, because a reduced value can make device creation fail even though the renderer does not need the difference.",
  "when_to_use": "Native renderer initialization returns no player or device after Vulkan is unavailable, with request_device reporting that limit max_compute_workgroups_per_dimension value 65535 is better than allowed 33778.",
  "applicability": "Rust wgpu renderers on OH falling back to GLES. The reported values come from an x86_64 emulator on one host GPU; physical PC GPU limits were not established. Relevant to PC adaptation mainly because the emulator is a common test target.",
  "keywords": [
    "wgpu",
    "GLES",
    "request_device",
    "max_compute_workgroups_per_dimension",
    "emulator",
    "downlevel_defaults",
    "Vulkan unavailable",
    "device limits"
  ],
  "related": [
    "capability-gated-backend-initialization",
    "rust-bridge-target-and-packaging",
    "rendering-budget-and-progressive-ui"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Clamp wgpu GLES device limits to what the OH adapter reports

## When to use

Native renderer initialization returns no player or device after Vulkan is unavailable, with request_device reporting that limit max_compute_workgroups_per_dimension value 65535 is better than allowed 33778.

## Recommended approach

On the GL backend, request the smaller of the baseline limit and the adapter limit, because a reduced value can make device creation fail even though the renderer does not need the difference.

## Implementation steps

1. Log the adapter backend and limits before requesting a device.
2. Start from the baseline, and on Backend::Gl set each limit your shaders do not depend on to the minimum of the baseline and adapter values.
3. Check that actual dispatch counts and shader varyings fit the reduced limits, rather than only passing device creation.
4. Leave other backends' requests unchanged, and add a unit test with the reported limit values.

## Conditions and alternatives

If your workload really needs a higher limit, disable that feature or fail clearly instead. A driver crash under repeated create and destroy on the host, reported in the source investigation, remains unexplained and is not addressed by this change.

## Common mistakes

Requesting adapter limits wholesale on every backend. Treating emulator success as evidence for a device GPU.

## Check it works

Create a device and render on the emulator and on real hardware with both backends available and with Vulkan unavailable. Log the granted limits and repeat creation and destruction.
