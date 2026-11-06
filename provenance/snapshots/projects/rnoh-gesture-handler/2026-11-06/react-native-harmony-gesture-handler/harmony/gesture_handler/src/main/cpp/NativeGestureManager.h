/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */
#pragma once
#include "RNOH/ArkTSMessageHandler.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"
#include "native/NativeGestureHandler.h"
#include <arkui/native_gesture.h>
#include <arkui/native_node.h>
#include <chrono>
#include <folly/dynamic.h>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace rnoh {

class ComponentInstance;
class NativeAnimatedTurboModule;
class RNInstanceCAPI;
class ScrollFramesHold;

/** ArkUI recognizers DETECT, this class ARBITRATES; see
 * docs/NATIVE-CODE-MAP.md. MAIN thread only, so nothing locks — the hazard is
 * re-entrancy (a worklet runs inside emitEvent), not concurrency. */
class NativeGestureManager : public ArkTSMessageHandler, public std::enable_shared_from_this<NativeGestureManager> {
public:
  void handleArkTSMessage(const Context &ctx) override;

  /** Not virtual in the base, but the shared_ptr deleter knows the type. */
  ~NativeGestureManager();

private:
  // --- message handlers ---
  void onAttach(const Context &ctx);
  void onUpdate(const Context &ctx);
  void onDrop(const Context &ctx);
  void onSetState(const Context &ctx);

  /** Holds the manager weakly; false when there is no live RN instance. */
  bool postDelayed(const NativeGestureHandler &handler, uint64_t delayMs,
                   std::function<void(NativeGestureManager &)> task);
  void postUntil(std::weak_ptr<TaskExecutor> executor, std::chrono::steady_clock::time_point deadline,
                 std::function<void(NativeGestureManager &)> task);

  void tryAttach(NativeGestureHandler &handler);
  void finishAttach(NativeGestureHandler &handler);
  /** The attach-time snapshot may predate layout (0x0). */
  void refreshLayout(NativeGestureHandler &handler);
  void detachAndDisposeRecognizer(NativeGestureHandler &handler);
  void removeHandlerFromNode(NativeGestureHandler &handler);
  void discardStaleNodeContext(NodeContext &nodeCtx, int newViewTag);
  void releaseScrollGuards(NativeGestureHandler &handler);
  /** False while the view is not yet inserted up to the surface root: RNOH
   * creates a component before it mounts it, and the attach can land between. */
  bool guardAncestorScrolls(NativeGestureHandler &handler);
  void guardAncestorScrollsWithRetry(int handlerTag, int retriesLeft);
  void guardDescendantScrolls(NativeGestureHandler &handler);
  void guardScrollNode(NativeGestureHandler &handler, ArkUI_NodeHandle node);

  // --- touch stream (driven by the sentinel recognizers) ---
  /** Its own node, or the one it was adopted onto. */
  [[nodiscard]] const NodeContext *sessionNodeOf(const NativeGestureHandler &handler) const;
  [[nodiscard]] std::vector<int> adoptedTagsFedBy(ArkUI_NodeHandle feedNode, bool onlyInSession) const;
  /** Tags, not pointers: a JS callback may drop a handler mid-loop. */
  [[nodiscard]] std::vector<int> sessionTagsFor(const NodeContext &nodeCtx) const;

  static void onSentinelPressEvent(ArkUI_GestureEvent *event, void *extraParams);
  static void onSentinelPanEvent(ArkUI_GestureEvent *event, void *extraParams);
  void handleSessionDown(NodeContext &nodeCtx, const ArkUI_UIInputEvent *raw);
  void handleSessionMove(NodeContext &nodeCtx, const ArkUI_UIInputEvent *raw);
  void handleSessionUp(NodeContext &nodeCtx, bool cancelled);
  /** The actual close; handleSessionUp decides when. */
  void closeSession(NodeContext &nodeCtx, bool cancelled);
  /** `fromNodeTouch`: the node's own touch event (vp), not a gesture's raw one. */
  void handleTouchDown(NodeContext &nodeCtx, NativeGestureHandler &handler, const ArkUI_UIInputEvent *event,
                       bool fromNodeTouch = false);
  /** A finger landing while another one's gesture is underway gets no
   * sentinel, so a button there opens its session from the node's feed. */
  void openButtonSessionFromNodeTouch(NodeContext &nodeCtx, const ArkUI_UIInputEvent *event);
  [[nodiscard]] bool hasSessionElsewhere(const NodeContext &nodeCtx) const;
#if RNGH_USE_COMMON_INTERFACE_FROM_RNSVG
  /** Whether a point (vp, local to the <Svg>) presses this shape handler's view. */
  bool svgElementIsHit(const NativeGestureHandler &handler, double x, double y);
#endif
  /** `feedTracker` only where the node's own touch event is unavailable;
   * `fromNodeTouch` as in handleTouchDown. */
  void handleTouchMove(NativeGestureHandler &handler, const ArkUI_UIInputEvent *event, bool feedTracker,
                       bool fromNodeTouch = false);
  /** Feeds the RNGH touch events. The sentinels' raw events cannot: a lifting
   * finger is zeroed one event early, a lift among still fingers is never
   * reported, and EMPTY events run long with several fingers down. */
  static void onNodeTouchEvent(ArkUI_NodeEvent *event);
  void handleNodeTouch(NodeContext &nodeCtx, const ArkUI_UIInputEvent *event);
  void replayLiftedFingers(NativeGestureHandler &handler, const std::vector<TrackedTouch> &lifted);
  void landPlatformFingers(NodeContext &nodeCtx);
  void landTrackedTouch(NativeGestureHandler &handler, const TrackedTouch &sample);
  /** Reported at the event's position, not the tracked one (upstream). */
  void liftTrackedTouchNear(NativeGestureHandler &handler, const TrackedTouch &sample);
  /** Banks the jump into panOrigin when the finger count changed. */
  void refreshPanTracking(NativeGestureHandler &handler, const ArkUI_UIInputEvent *rawInputEvent);
  /** Pressure gate: activates in [minForce, maxForce], reports force per move. */
  void evaluateForceTouch(NativeGestureHandler &handler, const ArkUI_UIInputEvent *event);
  void handleTouchUp(NativeGestureHandler &handler);
  /** `generation` scopes the poll to one press. */
  void resolvePendingFailure(int handlerTag, int generation);

  // --- cross-tree overlap adoption (see NativeGestureHandler::adopted) ---
  void adoptCoveredHandlers(NodeContext &nodeCtx, const ArkUI_UIInputEvent *raw);
  [[nodiscard]] bool areRelated(const NativeGestureHandler &a, const NativeGestureHandler &b) const;
  [[nodiscard]] bool isAncestorOrDescendantView(const NativeGestureHandler &handler, int otherViewTag) const;
  /** handler's view sits UNDER ancestorViewTag. */
  [[nodiscard]] bool isDescendantView(const NativeGestureHandler &handler, int ancestorViewTag) const;
  [[nodiscard]] std::optional<std::pair<double, double>> absoluteOriginOf(const NativeGestureHandler &handler) const;

  // --- orchestration core ---
  void beginHandler(NativeGestureHandler &handler);
  /** The activation gates (upstream tryActivate); the StateManager passes
   * them too, only the manualActivation refusal does not apply to it. */
  void requestActivation(NativeGestureHandler &handler, bool fromStateManager = false);
  void activateHandler(NativeGestureHandler &handler);
  void failHandler(NativeGestureHandler &handler, bool cancelled);
  void endHandler(NativeGestureHandler &handler);
  void cancelCompetitorsOf(NativeGestureHandler &winner);
  void notifyWaitersAbout(NativeGestureHandler &settled);
  /** waitFor, plus every handler that declared it blocks us. */
  [[nodiscard]] bool anyWaitTarget(const NativeGestureHandler &handler,
                                   const std::function<bool(const NativeGestureHandler &)> &predicate) const;
  [[nodiscard]] bool isWaitingOnUndecidedTargets(const NativeGestureHandler &handler) const;
  [[nodiscard]] bool hasSucceededWaitTarget(const NativeGestureHandler &handler) const;
  [[nodiscard]] bool waitsOnTag(const NativeGestureHandler &handler, int tag) const;
  [[nodiscard]] bool areSimultaneous(const NativeGestureHandler &a, const NativeGestureHandler &b) const;
  /** Upstream's hit test gives the touch to the deepest native view. */
  [[nodiscard]] bool hasDeeperNativeViewInSession(const NativeGestureHandler &handler) const;
  /** Upstream android ButtonViewGroup.canBegin: the `exclusive` prop against
   * the buttons other fingers already press. */
  [[nodiscard]] bool buttonCanBegin(const NativeGestureHandler &button) const;
  /** A button and a handler in a disjoint subtree, neither adopted: one
   * finger hit-tests one chain of views, so they hold different fingers. */
  [[nodiscard]] bool isButtonOnOtherFinger(const NativeGestureHandler &a, const NativeGestureHandler &b) const;
  ArkUI_GestureInterruptResult judgeNativeScroll(ArkUI_NodeHandle node, const ScrollAsk &ask);
  [[nodiscard]] bool panWouldHaveActivatedBefore(const NativeGestureHandler &handler, const ScrollAsk &ask) const;
  /** Null when the timer is stale. */
  [[nodiscard]] NativeGestureHandler *handlerStillPending(int handlerTag, int generation, bool requireTouchDown);
  [[nodiscard]] bool deferTapBehindEarlierSibling(NativeGestureHandler &handler);
  /** FAILED when maxDelayMs elapses between taps. */
  void armTapWindow(NativeGestureHandler &handler);
  /** FAILED when maxDurationMs elapses mid-press. */
  void armDurationTimer(NativeGestureHandler &handler);
  void armPanLongPressTimer(NativeGestureHandler &handler);
  void armLongPressTimer(NativeGestureHandler &handler);
  void setRootViewHandlingTouches(NativeGestureHandler &handler, bool handling);

  // --- recognizer events ---
  static void onNativeGestureCallback(ArkUI_GestureEvent *event, void *extraParams);
  void ensureSessionFromRecognizerEvent(NativeGestureHandler &handler, ArkUI_GestureEvent *event);
  void handleRecognizerAccept(NativeGestureHandler &handler, ArkUI_GestureEvent *event);
  void handleRecognizerUpdate(NativeGestureHandler &handler, ArkUI_GestureEvent *event);
  void handleRecognizerFinish(NativeGestureHandler &handler, bool cancelled);
  /** Fling rides on a pan recognizer — see FlingSpec::createRecognizer. */
  void handleFlingPanEvent(NativeGestureHandler &handler, ArkUI_GestureEvent *event);

  // --- gates (thresholds the recognizers cannot express) ---
  [[nodiscard]] bool passesHitSlop(const NativeGestureHandler &handler, double x, double y) const;
  [[nodiscard]] bool cancelledByLeavingView(const NativeGestureHandler &handler) const;
  void evaluateMoveGates(NativeGestureHandler &handler);
  [[nodiscard]] bool panActivationGatesSatisfied(const NativeGestureHandler &handler, double dx, double dy) const;

  /** Positive hitSlop must catch touches OUTSIDE the view, which per-node
   * observation never sees. The NDK region sizes are percentages, so this
   * needs a measured frame — hence the delayed retry. */
  void updateResponseRegion(NodeContext &nodeCtx);
  void applyHitSlopRegionWithRetry(int handlerTag, int retriesLeft);

  // --- emission ---
  void emitStateChange(NativeGestureHandler &handler, int state, int oldState);
  void emitUpdate(NativeGestureHandler &handler);
  void syncTrackedTouches(NativeGestureHandler &handler, const ArkUI_UIInputEvent *event, double unitsPerVp);
  void liftTrackedTouches(NativeGestureHandler &handler);
  /** One touches-cancelled naming every tracked finger, owed on the way to
   * any terminal state. */
  void cancelTrackedTouches(NativeGestureHandler &handler);
  /** `all` is the tracker, plus the lifted finger on a touches-up. */
  void emitTouchPayload(NativeGestureHandler &handler, int eventType, const std::vector<TrackedTouch> &changed,
                        const std::vector<TrackedTouch> &all, int numberOfTouches);
  void emitEvent(NativeGestureHandler &handler, const char *eventName, folly::dynamic payload);
  /** Animated.event(useNativeDriver: true) - see emitEvent. */
  void feedNativeAnimated(const NativeGestureHandler &handler, const char *eventName, const folly::dynamic &payload);
  [[nodiscard]] folly::dynamic statePayload(const NativeGestureHandler &handler, int state, int oldState) const;

  // --- interrupter (judge) ---
  static ArkUI_GestureInterruptResult onGestureInterrupt(ArkUI_GestureInterruptInfo *info);
  static ArkUI_GestureRecognizer *onParallelInnerGesture(ArkUI_ParallelInnerGestureEvent *event);

  /** The "wait" verdict ArkUI's judge lacks. */
  void setScrollFramesHeld(NativeGestureHandler &nativeView, bool held);
  /** upstream shouldBeCancelledByActiveHandler: an activation (or a waiter
   * released by its target's failure) is cancelled when a non-simultaneous
   * ANCESTOR is active - or parked while still under the finger. */
  [[nodiscard]] bool isCancelledByActiveHandler(const NativeGestureHandler &handler) const;
  [[nodiscard]] bool hasWaitRelations(const NativeGestureHandler &handler) const;
  [[nodiscard]] bool isWaitTarget(const NativeGestureHandler &handler) const;

  [[nodiscard]] NativeGestureHandler *findHandler(int handlerTag);

  // --- shared helpers (used from several translation units) ---
  [[nodiscard]] static std::shared_ptr<RNInstanceCAPI> instanceCapiOf(const NativeGestureHandler &handler);
  [[nodiscard]] static std::shared_ptr<ComponentInstance> componentOf(const NativeGestureHandler &handler);
  /** Walks up from `viewTag` looking for `ancestorViewTag`. */
  [[nodiscard]] static bool isUnderView(RNInstanceCAPI &instanceCAPI, int viewTag, int ancestorViewTag);
  /** The list must not be empty. */
  [[nodiscard]] static size_t indexOfNearest(const std::vector<TrackedTouch> &touches, const TrackedTouch &sample);

  std::unordered_map<int, std::unique_ptr<NativeGestureHandler>> m_handlersByTag;
  uint64_t m_attachSeq = 0;
  std::unordered_map<ArkUI_NodeHandle, std::unique_ptr<NodeContext>> m_nodes;
  std::unordered_map<ArkUI_NodeHandle, ScrollGuard> m_scrollGuards;
  /** Stays installed (forwarding) once released: the node's delegate pointer
   * is not guaranteed to outlive the proxy. */
  std::unordered_map<ArkUI_NodeHandle, std::shared_ptr<ScrollFramesHold>> m_scrollHolds;
  /** Lazily resolved; one manager serves one RN instance. */
  std::weak_ptr<NativeAnimatedTurboModule> m_nativeAnimatedTurboModule;

  /** Lookups for the C callbacks that carry no user data (API_1). */
  static std::unordered_map<ArkUI_NodeHandle, NativeGestureManager *> s_managerByNode;
  static std::unordered_map<ArkUI_GestureRecognizer *, NodeContext *> s_nodeCtxByRecognizer;
  static std::unordered_map<ArkUI_NodeHandle, NativeGestureManager *> s_managerByScrollNode;
  static std::unordered_set<ArkUI_GestureRecognizer *> s_ownedRecognizers;
};

} // namespace rnoh
