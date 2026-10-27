---
{
  "schema_version": 1,
  "id": "qt-ohos-terminal-direct-key-input",
  "title": "Route hardware keys directly to an OH Qt terminal widget",
  "topic_key": "input.qt-ohos-terminal-direct-key-input",
  "area": "input",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "For a terminal that consumes hardware KeyPress events, disable input-method registration on the actual focused terminal widget and its relevant children, then verify the focus chain.",
  "when_to_use": "Printable hardware keyboard characters do not reach an OH Qt terminal even though shortcuts or mouse input work.",
  "applicability": "Qt 6 OH terminal widgets affected by IMF interception in the reviewed QPA/runtime generation. This scoped workaround can disable IME composition and is not a general text-editor fix.",
  "keywords": [
    "WA_InputMethodEnabled",
    "IMF",
    "KeyPress",
    "QTermWidget",
    "TerminalDisplay",
    "hardware keyboard",
    "IME"
  ],
  "related": [
    "qt6-ohos-subwindow-input",
    "qt-close-event-origin"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Route hardware keys directly to an OH Qt terminal widget

## When to use

Printable hardware keyboard characters do not reach an OH Qt terminal even though shortcuts or mouse input work.

## Recommended approach

For a terminal that consumes hardware KeyPress events, disable input-method registration on the actual focused terminal widget and its relevant children, then verify the focus chain.

## Implementation steps

1. Log focus and KeyPress versus input-method events to establish whether IMF is consuming the hardware path.
2. On the terminal subtree only, apply `setAttribute(Qt::WA_InputMethodEnabled, false)` to the container and actual focusable descendants.
3. Find descendants by type/tree membership rather than assuming objectName equals className; revisit dynamically created children.
4. After asynchronous native window activation, restore focus to the terminal using a lifecycle-aware signal where possible. Treat a fixed delay as a source-specific workaround.

## Conditions and alternatives

If users require CJK or other composed input, repair/implement the input-method contract instead of applying this workaround indiscriminately. PTY availability and shell execution are separate from key routing.

## Common mistakes

Disabling IME only on the parent misses the real TerminalDisplay child; enabling all text widgets for direct keys breaks composition elsewhere.

## Check it works

Test printable ASCII, control keys, shortcuts, focus after reopen, and the intended composition policy on a PC keyboard. Check dynamically added terminal tabs.
