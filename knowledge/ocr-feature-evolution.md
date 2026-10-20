---
{
  "schema_version": 1,
  "id": "ocr-feature-evolution",
  "topic_key": "plugins.ocr-feature-evolution",
  "scope": "common",
  "frameworks": [],
  "title": "Replace an unavailable OCR backend while preserving the app contract",
  "area": "plugins",
  "summary": "Keep OCR behind an app interface and reevaluate capability support against the selected backend, model packaging and target device.",
  "when_to_use": "Your OCR package has no usable OH native implementation, or an old platform guard disables OCR even after adopting a different backend.",
  "keywords": [
    "OCR",
    "LiteRT",
    "scu_ocr_lite",
    "模型",
    "文字识别",
    "capability restoration",
    "OCR unsupported",
    "LiteRT replacement",
    "OCR不可用"
  ],
  "status": "scoped",
  "evidence_tier": "imported-guidance",
  "pc_validation": "not-run",
  "applicability": "Shared platform/native contract; select an OH adapter and verify the stated SDK/product capabilities on PC.",
  "related": []
}
---

# Replace an unavailable OCR backend while preserving the app contract

## When to use

Your OCR package has no usable OH native implementation, or an old platform guard disables OCR even after adopting a different backend.

## Recommended approach

Keep OCR behind an app interface and reevaluate capability support against the selected backend, model packaging and target device.

## Implementation steps

1. Identify whether the blocker is native-library availability, initialization, model format, preprocessing or the shared API.
2. Compare a supported alternative against the operations you need: input formats, async execution, language coverage and result structure.
3. Package/load the required model and adapt preprocessing/result conversion behind the existing interface.
4. Replace capability checks tied to the old backend with checks matching the new implementation.
5. Preserve explicit unavailable/error behavior until native initialization and representative recognition work.
6. Keep UI and business code independent of the specific OCR engine so later replacements stay localized.

## Conditions and alternatives

Removing a platform guard is justified only when the new execution path is implemented. Model/plugin compatibility and quality still need validation for the actual target.

## Common mistakes

A backend change can restore an implementation path without matching accuracy, memory use or latency. Do not keep an obsolete universal OH-disabled flag or remove it solely because the package resolves.

## Check it works

Test bundled model loading, recognition/error/cancel behavior, representative scripts/images, memory pressure and output quality.
