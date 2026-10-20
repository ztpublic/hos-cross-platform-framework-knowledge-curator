---
{
  "id": "window-mode-and-fullscreen-state",
  "title": "Handle fullscreen and PiP across phone and desktop-style windows",
  "topic_key": "window.window-mode-and-fullscreen-state",
  "area": "window",
  "keywords": [
    "PiP",
    "miniwindow",
    "2in1",
    "fullscreen",
    "orientation",
    "freeWindowMode",
    "窗口",
    "fullscreen race",
    "PiP wrong size",
    "free window",
    "root navigator",
    "nested navigator",
    "小窗缩放异常"
  ],
  "summary": "Model device category, native window mode, orientation and transition state as separate inputs, and base layout on the relevant combination.",
  "status": "scoped",
  "schema_version": 2,
  "when_to_use": "Fullscreen/PiP behavior works on one OH device but mis-scales a free window, leaves system bars wrong, or races an orientation change."
}
---

# Handle fullscreen and PiP across phone and desktop-style windows

## When to use

Fullscreen/PiP behavior works on one OH device but mis-scales a free window, leaves system bars wrong, or races an orientation change.

## Recommended approach

Model device category, native window mode, orientation and transition state as separate inputs, and base layout on the relevant combination.

## Implementation steps

1. Obtain native window-mode information where available instead of classifying every floating window as a media miniwindow.
2. Map fullscreen intent to an appropriate native action: phone orientation/system-bar behavior may differ from desktop maximize/recover.
3. Choose an explicit transition policy: wait for observed viewport changes, or switch layout immediately and tolerate temporary letterboxing.
4. Bound channel waits and reconcile observed state afterward; a missing reply does not prove the visual operation failed.
5. Enable PiP only when its required surface/context is ready. If one manual-entry sequence is defective, restrict that sequence rather than disabling all PiP.
6. Remove native listeners on detach.

## Conditions and alternatives

Window APIs and available modes vary by product and SDK. Match the workaround to the failing mode/backend combination.

### Keep fullscreen scope and native window controls separate

Separate fullscreen/orientation intent from system-bar visibility. A desktop maximize/recover operation and hiding status/navigation bars are different commands. Await the actual native operation and reconcile observed state; an immediate channel acknowledgement before a native Promise finishes is not completion evidence.

A fullscreen Flutter page also needs the intended navigator scope. A launch route inside a sidebar's nested navigator inherits that pane's bounds. Push the destination on the root navigator when it must cover the full application, and remove the temporary launch route from its original navigator explicitly. If already on the root, replacement can avoid an extra back-stack entry. Preserve route settings, mounted checks and return behavior; a fullscreen flag cannot escape an ancestor pane's layout.

Where supported, `WindowStage.setWindowRectAutoSave` can delegate main-window rectangle/state restoration to the system. Gate it by the installed SDK and device capability, report rejection, and keep it separate from application content state and manually forced geometry.

## Common mistakes

Device type alone cannot identify current window state. A device supporting OH apps does not imply it supports running the development toolchain.

## Check it works

Test phone, tablet/free-window and desktop-style modes, rapid entry/exit, rotation, system bars, manual/automatic PiP and callback loss.
