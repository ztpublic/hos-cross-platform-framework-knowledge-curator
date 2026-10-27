# Native code map (HarmonyOS C++ layer)

Where each part of the HarmonyOS C++ gesture layer lives, under `harmony/gesture_handler/src/main/cpp/`, and how the pieces behave. Read it before your first change to the native pipeline. The ArkTS side under `src/main/ets/` is only the turbo module that hands work over to C++.

## How a handler reaches C++

`ets/rnoh/RNGestureHandlerModule.ts` accepts the handlers named in `NATIVE_HANDLER_NAMES` and warns about any other (e.g. Hover), which is then ignored. It keeps each handler's config until attach, then talks to C++ with four messages:

| Message | Sent from | Handled by |
|---|---|---|
| `RNGH::NATIVE_GH_ATTACH` | `attachGestureHandler` | `NativeGestureManager::onAttach` |
| `RNGH::NATIVE_GH_UPDATE` | `updateGestureHandler`, only when the config changed | `onUpdate` |
| `RNGH::NATIVE_GH_DROP` | `dropGestureHandler` | `onDrop` |
| `RNGH::NATIVE_GH_SET_STATE` | `setGestureHandlerState` (the StateManager) | `onSetState` |

Nothing flows back to ArkTS: events reach JS from C++ (see "Emitting to JS"). The root view sends RNOH's own `CANCEL_TOUCHES` from `cancelReactNativeTouches` when a handler locks RN's touches.

## The core idea

ArkUI recognizers detect, RNGH arbitrates. Each node with handlers gets one PARALLEL gesture group: two sentinel recognizers plus one recognizer per handler with an ArkUI counterpart. NativeView, Manual and ForceTouch have none and use the raw touch stream (`evaluateForceTouch` reads the pressure from it).

Everything ArkUI cannot express runs as a C++ state machine on the MAIN thread: BEGAN at touch-down, threshold failures, timing windows, activation gates and the relations. `NativeGestureManager` is one class split by topic across five `NativeGestureManager*.cpp` files; `GestureConfig`, `PointerSampling` and `RecognizerSpec` stand alone.

## Files

All paths are relative to `harmony/gesture_handler/src/main/cpp/`.

| File | Responsibility | Key functions |
|---|---|---|
| `NativeGestureManager.h` | The class declaration, its state maps, and the static helpers shared across its files. | none |
| `NativeGestureManager.cpp` | The ArkTS message pipeline, attach and teardown, the definitions of the static callback maps, and state-change emission. | `handleArkTSMessage` `onAttach` `onUpdate` `onDrop` `onSetState` `tryAttach` `finishAttach` `detachAndDisposeRecognizer` `removeHandlerFromNode` `postDelayed` `findHandler` `statePayload` `emitStateChange` `emitUpdate` |
| `native/NativeGestureHandler.h` | Plain data: `GestureType`, `TrackedTouch`, `NativeGestureHandler` (the per-handler record), `NodeContext` (per ArkUI node), `ScrollGuard`, `ScrollAsk`, plus small inline readers of the record. | `actingPointer` `panTranslationX` `panTranslationY` `trackVelocity` |
| `native/PointerVelocityTracker.h` | The handler's velocity: a least-squares fit over the last 100 ms of continuous motion, as upstream's VelocityTracker. | `add` `velocity` `shift` `reset` |
| `native/NativeGestureManagerTouch.cpp` | The raw touch stream: sentinel callbacks, the node's own touch feed, per-handler down, move and up, cross-tree adoption, layout, the hitSlop response region and the SVG hit test. | `onSentinelPressEvent` `onSentinelPanEvent` `handleSessionDown` `handleSessionMove` `handleSessionUp` `closeSession` `onNodeTouchEvent` `handleNodeTouch` `landPlatformFingers` `handleTouchDown` `handleTouchMove` `handleTouchUp` `refreshPanTracking` `resolvePendingFailure` `adoptCoveredHandlers` `refreshLayout` `updateResponseRegion` `applyHitSlopRegionWithRetry` `svgElementIsHit` |
| `native/NativeGestureManagerTrackedTouches.cpp` | RNGH touch events (`needsPointerData`): matching an event's fingers against the handler's tracker, and the two emission entry points. | `syncTrackedTouches` `reconcile` `readPointers` `landTrackedTouch` `liftTrackedTouchNear` `liftTrackedTouches` `cancelTrackedTouches` `emitTouchPayload` `emitEvent` |
| `native/NativeGestureManagerOrchestration.cpp` | The state machine: transitions, relations, timers, gates and recognizer callbacks. | `beginHandler` `requestActivation` `activateHandler` `endHandler` `failHandler` `cancelCompetitorsOf` `notifyWaitersAbout` `anyWaitTarget` `hasSucceededWaitTarget` `isWaitingOnUndecidedTargets` `isCancelledByActiveHandler` `setRootViewHandlingTouches` `armTapWindow` `armDurationTimer` `armLongPressTimer` `armPanLongPressTimer` `ensureSessionFromRecognizerEvent` `handleRecognizerAccept` `handleRecognizerUpdate` `handleRecognizerFinish` `handleFlingPanEvent` `evaluateMoveGates` `panActivationGatesSatisfied` |
| `native/NativeGestureManagerScroll.cpp` | Gestures versus native scroll: the guards, the judge and the frame hold. | `guardScrollNode` `guardAncestorScrolls` `guardDescendantScrolls` `onGestureInterrupt` `onParallelInnerGesture` `judgeNativeScroll` `panWouldHaveActivatedBefore` `setScrollFramesHeld` |
| `native/GestureConfig.h`, `native/GestureConfig.cpp` | The wire contract with JS: frozen enums (`RNGHState`, `RNGHTouchEventType`, `RNGHPointerType`), upstream defaults, tuning constants and the config readers. | `cfgNum` `cfgBool` `cfgHas` `cfgTags` `asInt` `containsTag` `hitSlopInsets` `SlopInsets::contains` `msSince` `panHasActivationGates` `panFailOffsetsBreached` `flingDirectionMatches` |
| `native/PointerSampling.h`, `native/PointerSampling.cpp` | Reading ArkUI input, and the lazily resolved ArkUI module APIs. | `sampleRawInputEvent` `windowMidpointOfFirstTwoPointers` `panTrackWindowPoint` `isPlaceholderPointer` `rnghPointerType` `getGestureAPI` `getGestureAPI2` `getNodeAPI` `interrupterUserData` |
| `native/RecognizerSpec.h`, `native/RecognizerSpec.cpp` | One stateless strategy per gesture kind: which recognizer to build, which callbacks to subscribe, which config change forces a recreate, and what the kind adds to the payload. | `findRecognizerSpec` `buildPayload`, the `RecognizerSpec` virtuals |
| `ScrollFramesHold.h`, `ScrollFramesHold.cpp` | Proxies a ScrollView's scroll-node delegate so a held scroll keeps the touch without moving. See "The scroll hold". | `installOn` `setHeld` `ownerAlive` |
| `RnohReactNativeHarmonyGestureHandlerPackage.h`, `RnohReactNativeHarmonyGestureHandlerPackage.cpp` | RNOH package wiring: the turbo module, component descriptors, JSI binders, and `NativeGestureManager` as the ArkTS message handler. | `createArkTSMessageHandlers` |
| `GestureHandlerPackage.h` | Deprecated alias of `RnohReactNativeHarmonyGestureHandlerPackage`. | none |
| `componentInstances/RNGestureHandlerRootViewComponentInstance.h`, `componentInstances/RNGestureHandlerRootViewComponentInstance.cpp` | The root view. It holds `isHandlingTouches`, which locks RN's own touch pipeline. | `setIsHandlingTouches` `cancelReactNativeTouches` |
| `componentInstances/RNGestureHandlerButtonComponentInstance.h`, `componentInstances/RNGestureHandlerButtonComponentInstance.cpp` | The RNGH button view and its `exclusive` prop. | `isExclusive` |
| `CMakeLists.txt` | Build. Detects react-native-svg and links `rnoh_svg` when it is new enough. | none |
| `generated/` | Codegen output for the turbo module and components. Do not hand-edit. | none |

`CMakeLists.txt` globs `*.cpp` recursively, so a new file under `native/` needs no build change. A react-native-svg newer than `RNSVG_SVG_ENABLE_THRESHOLD` turns on `RNGH_USE_COMMON_INTERFACE_FROM_RNSVG=1`.

## Where do I look for X

| Question | File |
|---|---|
| Which handler kinds exist, how a config reaches C++ | `ets/rnoh/RNGestureHandlerModule.ts` (`NATIVE_HANDLER_NAMES`) |
| Attach lifecycle, retries, recognizer creation, teardown | `NativeGestureManager.cpp` |
| Where a touch enters the library | `native/NativeGestureManagerTouch.cpp` (`handleSessionDown`) |
| Why a handler never sees a touch (overlap, hit test) | `native/NativeGestureManagerTouch.cpp` (`adoptCoveredHandlers`, `svgElementIsHit`) |
| hitSlop, response region, view frame | `native/NativeGestureManagerTouch.cpp` (`updateResponseRegion`, `refreshLayout`) |
| waitFor, simultaneousWith, blocks | `native/NativeGestureManagerOrchestration.cpp` (`anyWaitTarget`, `hasSucceededWaitTarget`, `isWaitingOnUndecidedTargets`, `isCancelledByActiveHandler`, `notifyWaitersAbout`) |
| Timers (multi-tap window, maxDuration, long press) | `native/NativeGestureManagerOrchestration.cpp` (`armTapWindow`, `armDurationTimer`, `armLongPressTimer`, `handlerStillPending`) |
| Activation gates (offsets, minDist, minVelocity) | `native/NativeGestureManagerOrchestration.cpp` (`evaluateMoveGates`, `panActivationGatesSatisfied`) |
| A gesture loses to a ScrollView, or wins | `native/NativeGestureManagerScroll.cpp` (`judgeNativeScroll`) |
| A ScrollView stays still while a gesture decides | `ScrollFramesHold.cpp`, `native/NativeGestureManagerScroll.cpp` (`setScrollFramesHeld`) |
| `onTouchesDown`, `onTouchesMove`, `onTouchesUp`, `onTouchesCancelled` payloads | `native/NativeGestureManagerTrackedTouches.cpp` |
| What JS receives, and on which channel | `native/NativeGestureManagerTrackedTouches.cpp` (`emitEvent`), `NativeGestureManager.cpp` (`statePayload`) |
| Per-gesture payload fields, recognizer params | `native/RecognizerSpec.cpp` |
| A config key's default | `native/GestureConfig.h` |
| RN's own touches are not cancelled | `componentInstances/RNGestureHandlerRootViewComponentInstance.cpp` (`setIsHandlingTouches`) |

## Touch sessions and the sentinels

A touch session is one press on one node, from the first finger down to the last finger up. The two sentinels in each node's group open and close it:

- The press sentinel, a long press that accepts after 1 ms (`SENTINEL_PRESS_DURATION_MS`), calls `handleSessionDown` on accept and `handleSessionUp` on end or cancel.
- The pan sentinel, a 1 vp pan (`SENTINEL_PAN_DISTANCE_VP`), calls `handleSessionMove` on its updates.

Nodes RNOH does not drive also register `NODE_TOUCH_EVENT`. It keeps `platformFingers`, drives the RNGH touch events and closes the session on the last platform up (`closeOnLastPlatformUp`). On ScrollView, RootView, ModalHostView and nodes with their own input handler, the pan sentinel feeds the tracker.

If the sentinels lose a race, `ensureSessionFromRecognizerEvent` opens the session from a recognizer event. On a normal close, `closeSession` settles any waiter still parked on a target whose node never opened a session.

A node's handlers stay in ATTACH order (`attachSeq`), as upstream delivers them, however their attach retries land.

Fingers that land and lift before any session opens are kept in `liftedBeforeSession` and replayed when one opens (`replayLiftedFingers`). A node up with no position (ArkUI's tenth-finger blind spot) lifts nothing.

## The state machine

States are RNGH's own (`RNGHState`). The transitions live in `native/NativeGestureManagerOrchestration.cpp`:

- `beginHandler` moves to BEGAN at touch-down.
- `requestActivation` asks for ACTIVE. It refuses under `manualActivation` unless the StateManager asks, and cancels when a wait target already succeeded or an active ancestor vetoes (`isCancelledByActiveHandler`).
- While a wait target is undecided, `requestActivation` parks the handler as `held`. A target above it whose node has not opened a session yet counts as undecided.
- `activateHandler` moves to ACTIVE, locks RN's touches unless `cancelsTouchesInView` is false, and cancels competitors. Tap, Fling and a handler lifted while held (`liftedWhileHeld`) end straight away.
- `endHandler` and `failHandler` reach a terminal state, mark the handler `suppressed` for the session, and wake its waiters (`notifyWaitersAbout`). A target's `endedAt` counts as "this touch" once its node session is already closed.
- Two unrelated taps on one node settle in attach order: `deferTapBehindEarlierSibling` makes the later one yield for 1 ms.

Relations are enforced at emission level, so the manager only gates what JS observes; the registry is manager-wide, so they work across subtrees. The StateManager's ACTIVE goes through the same gates.

## Gestures versus native scrolls

`guardScrollNode` puts the judge (a gesture interrupter) and `onParallelInnerGesture` on a ScrollView's node: ancestor scrolls at attach, descendant scrolls at attach and every touch-down, up to `DESCENDANT_SCROLL_SCAN_LIMIT` nodes. `onGestureInterrupt` finds the node through the API_2 user data (`interrupterUserData`) and hands built-in pans to `judgeNativeScroll`.

The judge picks one winner per touch, when the scroll's pan first tries to accept. It rejects the scroll when:

- a handler guarding it is already ACTIVE
- a pan met its activation gates before this sample (measured in window space, like the gates themselves, so a scaled or rotated view's local coordinates do not shrink the distance)
- the ScrollView's own NativeView already lost

Otherwise the scroll takes the touch and competitors still in BEGAN are cancelled. A cross-axis ask continues without a takeover.

RNGH's ScrollView wrapper and an explicit `Gesture.Native` on the same ScrollView are one scroll: the judge prefers the gated one, and `cancelCompetitorsOf` never cancels the twin.

## The scroll hold

`ScrollFramesHold` lets a ScrollView wait for its waitFor targets: while held, `onScrollFrameBegin` returns 0, so the scroll keeps the touch but does not move, and the nested-scroll mode is forced to SELF_ONLY until release.

`setScrollFramesHeld` holds the frames when the judge's NativeView waits on a handler in BEGAN, or when `failHandler` cancels a gated ScrollView mid-touch. Activation, the drop and the judge's next ask release them. Proxies stay installed in `m_scrollHolds` after release.

## Cross-tree adoption

`adoptCoveredHandlers` feeds a touch to covered Tap, LongPress, Pan and Manual handlers ArkUI skipped, when they relate to a handler that got it and the point is inside their frame. They read the other node's stream through their window origin, ignoring ancestor scroll offsets and transforms.

## Pointers and units

Handler state is in vp. ArkUI gesture events report px and are divided by the handler's `scale`; the node's own touch events are already in vp. Each output follows its own pointer rule:

- Emitted `x`/`y`, the hitSlop and "outside" checks, and the Tap and LongPress gates use the event's own point (`lastX`, `lastY`).
- Pan translation and gates use `panTrackWindowPoint`: the newest pointer at an arrival, then the one nearest the track (`PanNominate`), or the mean with `avgTouches`, in window space. Finger-count changes bank into `panOriginX`.
- Pan velocity uses the same tracked point, fed from both the sentinel and the recognizer stream into `velocityTracker`; a finger-count change shifts the tracker's history by the jump banked into `panOriginX`. The same value goes to `onUpdate`, the state payloads and the `minVelocity` gates. Fling still reads ArkUI's own velocity.
- The pinch focal point and rotation anchor use `windowMidpointOfFirstTwoPointers`.
- RNGH touch events track fingers in `trackedTouches`, matched by proximity; a lone finger is always id 0.

## SVG shapes

With react-native-svg support on, `tryAttach` hosts a shape's handler on the outermost `<Svg>` node (`svgRootTag`), measured by `refreshLayout`. At touch-down `svgElementIsHit` hit-tests the shapes by path and climbs to the nearest `responsible` ancestor; the handler joins only when that is its own view, otherwise `svgMissed` is set.

## Emitting to JS

`emitEvent` picks the channel from `actionType`:

- Reanimated worklets get both event kinds on the view's Fabric emitter.
- Animated gets only its updates (`onGestureHandlerEvent`) there.
- Everything else goes through `RCTDeviceEventEmitter`.

## The RecognizerSpec mechanism

`onAttach` sets each handler's `spec` from `findRecognizerSpec(handlerName)`; `type` stays on the record for the behavioural branches. The spec answers everything about a gesture kind that does not depend on manager state:

- `type()`, `hasRecognizer()`, `isContinuous()`
- `survivesScrollTakeover(config)`, `defaultShouldCancelWhenOutside()`
- `recognizerParams(config)`, which `onUpdate` compares to decide a recreate
- `createRecognizer(api, config, scale)`, `actionMask()`
- `fallbackPointerCount(config)`, `fillPayload(...)`, the kind's own payload fields

The eight specs are stateless `const` instances in `native/RecognizerSpec.cpp`.

## Adding a gesture

The base spec is continuous, has a recognizer, subscribes all four action callbacks, falls back to `numberOfPointers`, and reports `x`, `y`, `absoluteX`, `absoluteY`.

1. Subclass `RecognizerSpec` in `native/RecognizerSpec.cpp` and override what differs. A kind with a recognizer must override `createRecognizer`.
2. Add a `const` instance and a line in `findRecognizerSpec`.
3. Add the enumerator to `GestureType` in `native/NativeGestureHandler.h`.
4. Add the handler name to `NATIVE_HANDLER_NAMES` in `ets/rnoh/RNGestureHandlerModule.ts`.
5. Review the `GestureType::` branches that are not spec-driven: `handleTouchDown`, `handleTouchMove`, `handleTouchUp`, `evaluateMoveGates`, `deferTapBehindEarlierSibling`, `handleRecognizerAccept`, `handleRecognizerUpdate`, `activateHandler`, `refreshPanTracking`, `adoptCoveredHandlers`, `statePayload`, `judgeNativeScroll` and `cancelCompetitorsOf`.

## Shared helpers and static maps

Helpers shared across files are private static members declared in `NativeGestureManager.h`: `instanceCapiOf`, `componentOf` and `isUnderView` live in `NativeGestureManager.cpp`, `indexOfNearest` in `native/NativeGestureManagerTrackedTouches.cpp`. Helpers used by one file are `static` in that file.

The four static maps for the C callbacks (`s_managerByNode`, `s_managerByScrollNode`, `s_nodeCtxByRecognizer`, `s_ownedRecognizers`) are declared in `NativeGestureManager.h` and defined at the top of `NativeGestureManager.cpp`.

## Threading and re-entrancy

Everything runs on the MAIN thread (deferred tasks via `postDelayed`), so there are no locks and none should be added. `postDelayed` keeps its deadline on `steady_clock` and re-posts when RNOH's uv timer fires early: RNOH starts uv timers without `uv_update_time`, so a timer armed from an ArkUI callback after idle time is measured from a stale loop time and would fire at once (a tap's `maxDurationMs` failed it on touch-down). A Reanimated worklet runs inside `emitEvent` and can drop the handler; see the REENTRANCY note in `syncTrackedTouches`.

## Verifying a change

- Fast: a host syntax check with the OHOS clang (`-fsyntax-only` against the RNOH headers), about a second per file. It does not catch link errors.
- Real: the tester e2e suite on a device, `node ./scripts/e2e.js --target <serial>` from `tester/`. Compare failing case names against the baseline, not counts: a few cases flip between runs.
