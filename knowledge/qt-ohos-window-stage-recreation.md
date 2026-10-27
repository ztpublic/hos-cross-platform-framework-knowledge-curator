---
{
  "schema_version": 1,
  "id": "qt-ohos-window-stage-recreation",
  "title": "Guard repeated QPA window-stage creation during scene recovery",
  "topic_key": "lifecycle.qt-ohos-window-stage-recreation",
  "area": "lifecycle",
  "scope": "framework-specific",
  "frameworks": [
    "qt"
  ],
  "summary": "Distinguish first stage creation from repeated creation on the same UIAbility and use the QPA recovery path only for the matching recovered instance.",
  "when_to_use": "After backgrounding a Qt OH app with no visible windows, reopening aborts with a missing io.qt.private.abilityInstanceId or pending auto-started instance error.",
  "applicability": "Qt 6.12 OH QPA with non-idempotent handleAbilityOnWindowStageCreate and a compatible handleAbilityOnWindowStageRestore. Relevant to PC tray-only/background windows; newer QPA implementations may differ.",
  "keywords": [
    "onWindowStageCreate",
    "scene recovery",
    "io.qt.private.abilityInstanceId",
    "handleAbilityOnWindowStageRestore",
    "tray",
    "background reopen"
  ],
  "related": [
    "foreground-recovery-and-save-lifetime",
    "qt-qpa-runtime-template-coherence"
  ],
  "status": "scoped",
  "evidence_tier": "source-reviewed",
  "pc_validation": "not-run"
}
---

# Guard repeated QPA window-stage creation during scene recovery

## When to use

After backgrounding a Qt OH app with no visible windows, reopening aborts with a missing io.qt.private.abilityInstanceId or pending auto-started instance error.

## Recommended approach

Distinguish first stage creation from repeated creation on the same UIAbility and use the QPA recovery path only for the matching recovered instance.

## Implementation steps

1. Log ability identity and window-stage create/destroy/restore ordering so a repeated callback can be distinguished from a fresh process or fresh ability.
2. Keep the first-create marker on the ability instance. Forward the first successful creation to handleAbilityOnWindowStageCreate using the matching runtime/template.
3. If the same ability receives another create callback after stage destruction, route it to the reviewed QPA restore handler instead of re-entering its one-time instance-registration path.
4. Keep actual onWindowStageRestore forwarding and QPA lifecycle ownership coherent. A genuinely new ability must still take the initial create path.
5. Serialize asynchronous permission/startup work with recovery. Mark initialization complete only for the stage that still exists; do not copy an early boolean that can hide a failed or abandoned first initialization.

## Conditions and alternatives

The reviewed guard prevents a specific fatal re-entry. Its source still reports possible blank content after very long background sessions, so it does not establish full scene recovery. Inspect the selected QPA restore semantics before applying this version-specific route.

## Common mistakes

Do not reset the ability marker merely because the stage was destroyed, share one marker across all abilities, or claim a non-crashing blank window is a successful recovery.

## Check it works

Exercise minimize, closing all windows while retaining tray state, short/long background, relaunch and process death. Verify both absence of the fatal error and restored visible/interactable Qt content.
