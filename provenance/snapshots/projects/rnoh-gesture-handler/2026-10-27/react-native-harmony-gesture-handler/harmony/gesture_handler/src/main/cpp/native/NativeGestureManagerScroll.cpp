/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */
/** Gesture versus native scroll: the guards, the judge's verdict, and the
 * frame hold that stands in for the "wait" ArkUI has no word for. */
#include "NativeGestureManager.h"
#include "RNOHCorePackage/ComponentInstances/ScrollViewComponentInstance.h"
#include "ScrollFramesHold.h"
#include "native/GestureConfig.h"
#include "native/PointerSampling.h"
#include "native/RecognizerSpec.h"
#include <algorithm>
#include <arkui/native_node.h>
#include <cmath>
#include <glog/logging.h>
#include <span>
#include <vector>

namespace rnoh {

/** Whether the handler guards this scroll node (ancestor, own node, or descendant). */
[[nodiscard]] static bool guardsScroll(const NativeGestureHandler &handler, ArkUI_NodeHandle node) {
  const auto &guarded = handler.guardedScrollNodes;
  return std::find(guarded.begin(), guarded.end(), node) != guarded.end();
}

/** The parallel-inner pairing is not optional: PARALLEL priority parallelizes
 * only against CHILDREN, so without it the instantly-accepting press sentinel
 * on a descendant wins the exclusive race and kills scrolling. */
void NativeGestureManager::guardScrollNode(NativeGestureHandler &handler, ArkUI_NodeHandle node) {
  if (node == nullptr || guardsScroll(handler, node)) {
    return;
  }
  auto *api = getGestureAPI();
  if (api == nullptr) {
    return;
  }
  // every call: a freed scroll node's handle can come back under a new ScrollView
  auto *api2 = getGestureAPI2();
  if (api2 != nullptr && api2->setGestureInterrupterToNode != nullptr) {
    api2->setGestureInterrupterToNode(node, node, NativeGestureManager::onGestureInterrupt);
  } else {
    api->setGestureInterrupterToNode(node, NativeGestureManager::onGestureInterrupt);
  }
  api->setInnerGestureParallelTo(node, nullptr, NativeGestureManager::onParallelInnerGesture);
  s_managerByScrollNode[node] = this;
  auto it = m_scrollGuards.find(node);
  if (it == m_scrollGuards.end()) {
    m_scrollGuards.emplace(node, ScrollGuard{1});
  } else {
    it->second.refCount++;
  }
  handler.guardedScrollNodes.push_back(node);
}

void NativeGestureManager::guardAncestorScrolls(NativeGestureHandler &handler) {
  auto componentInstance = componentOf(handler);
  if (componentInstance == nullptr) {
    return;
  }
  if (handler.isScrollContainer) {
    this->guardScrollNode(handler, handler.node);
  }
  auto ancestor = componentInstance->getParent().lock();
  while (ancestor != nullptr) {
    if (ancestor->getComponentName() == "ScrollView") {
      this->guardScrollNode(handler, ancestor->getLocalRootArkUINode().getArkUINodeHandle());
    }
    ancestor = ancestor->getParent().lock();
  }
}

/** A ScrollView inside the view competes just like an ancestor one, and has no
 * handler of its own to guard its node. Re-scanned at every touch-down, since
 * the subtree can change; bounded, so a huge one still costs a fixed amount. */
void NativeGestureManager::guardDescendantScrolls(NativeGestureHandler &handler) {
  auto componentInstance = componentOf(handler);
  if (componentInstance == nullptr) {
    return;
  }
  std::vector<ComponentInstance::Shared> pending = componentInstance->getChildren();
  int visited = 0;
  while (!pending.empty() && visited++ < DESCENDANT_SCROLL_SCAN_LIMIT) {
    auto current = std::move(pending.back());
    pending.pop_back();
    if (current == nullptr) {
      continue;
    }
    if (current->getComponentName() == "ScrollView") {
      this->guardScrollNode(handler, current->getLocalRootArkUINode().getArkUINodeHandle());
    }
    const auto &children = current->getChildren();
    pending.insert(pending.end(), children.begin(), children.end());
  }
}

void NativeGestureManager::setScrollFramesHeld(NativeGestureHandler &nativeView, bool held) {
  auto scrollView = std::dynamic_pointer_cast<ScrollViewComponentInstance>(componentOf(nativeView));
  if (scrollView == nullptr) {
    return;
  }
  // a new ScrollView on an old one's address makes its proxy stale
  std::erase_if(m_scrollHolds, [](const auto &entry) { return !entry.second->ownerAlive(); });
  const auto node = scrollView->getLocalRootArkUINode().getArkUINodeHandle();
  auto it = m_scrollHolds.find(node);
  if (it == m_scrollHolds.end()) {
    if (!held) {
      return; // nothing proxied, so nothing is held
    }
    auto hold = ScrollFramesHold::installOn(scrollView);
    if (hold == nullptr) {
      return;
    }
    it = m_scrollHolds.emplace(node, std::move(hold)).first;
  }
  it->second->setHeld(held);
}

// --- Interrupter (judge) ---

/** Defensive by necessity: GetSystemRecognizerType answers -1 and the gesture
 * event's GetNode null (API 19 and 24), so the direction mask is what tells a
 * pan apart, and the guarded node rides along as API_2 user data. */
ArkUI_GestureInterruptResult NativeGestureManager::onGestureInterrupt(ArkUI_GestureInterruptInfo *info) {
  auto *recognizer = OH_ArkUI_GestureInterruptInfo_GetRecognizer(info);
  if (recognizer == nullptr || s_ownedRecognizers.count(recognizer) > 0) {
    return GESTURE_INTERRUPT_RESULT_CONTINUE;
  }
  const bool builtIn = OH_ArkUI_GestureInterruptInfo_GetSystemFlag(info) || OH_ArkUI_IsBuiltInGesture(recognizer);
  if (!builtIn) {
    return GESTURE_INTERRUPT_RESULT_CONTINUE;
  }
  auto *api = getGestureAPI();
  ArkUI_GestureDirectionMask mask = 0;
  const bool maskKnown = OH_ArkUI_GetPanGestureDirectionMask(recognizer, &mask) == 0;
  const int systemType = OH_ArkUI_GestureInterruptInfo_GetSystemRecognizerType(info);
  const bool isPan =
    maskKnown || (api != nullptr && api->getGestureType != nullptr && api->getGestureType(recognizer) == PAN_GESTURE) ||
    systemType == PAN_GESTURE || systemType == SWIPE_GESTURE;
  if (!isPan) {
    return GESTURE_INTERRUPT_RESULT_CONTINUE;
  }
  auto *gestureEvent = OH_ArkUI_GestureInterruptInfo_GetGestureEvent(info);
  auto node = static_cast<ArkUI_NodeHandle>(interrupterUserData(info));
  if (node == nullptr && gestureEvent != nullptr) {
    node = OH_ArkUI_GestureEvent_GetNode(gestureEvent);
  }
  auto managerIt = node != nullptr ? s_managerByScrollNode.find(node) : s_managerByScrollNode.end();
  if (managerIt == s_managerByScrollNode.end()) {
    return GESTURE_INTERRUPT_RESULT_CONTINUE;
  }
  ScrollAsk ask{};
  ask.mask = mask;
  ask.event = gestureEvent;
  if (gestureEvent != nullptr) {
    ask.offsetXpx = OH_ArkUI_PanGesture_GetOffsetX(gestureEvent);
    ask.offsetYpx = OH_ArkUI_PanGesture_GetOffsetY(gestureEvent);
    ask.hasOffsets = ask.offsetXpx != 0 || ask.offsetYpx != 0;
    const auto *raw = OH_ArkUI_GestureEvent_GetRawInputEvent(gestureEvent);
    if (raw != nullptr) {
      ask.windowXpx = OH_ArkUI_PointerEvent_GetWindowX(raw);
      ask.windowYpx = OH_ArkUI_PointerEvent_GetWindowY(raw);
      ask.hasWindowPoint = true;
    }
  }
  return managerIt->second->judgeNativeScroll(node, ask);
}

ArkUI_GestureRecognizer *NativeGestureManager::onParallelInnerGesture(ArkUI_ParallelInnerGestureEvent *event) {
  if (event == nullptr) {
    return nullptr;
  }
  ArkUI_GestureRecognizerHandleArray conflicts = nullptr;
  int32_t size = 0;
  if (OH_ArkUI_ParallelInnerGestureEvent_GetConflictRecognizers(event, &conflicts, &size) != 0 ||
      conflicts == nullptr || size <= 0) {
    return nullptr;
  }
  const std::span conflictList(conflicts, static_cast<size_t>(size));
  // Pairing would let a same-axis descendant scrollable scroll along with this
  // one, so skip it - unless RNGH orchestrates the scroll (its relations need
  // the sentinels).
  bool orchestrated = false;
  for (auto *conflict : conflictList) {
    auto ctxIt = s_nodeCtxByRecognizer.find(conflict);
    if (ctxIt == s_nodeCtxByRecognizer.end() || ctxIt->second->manager == nullptr) {
      continue;
    }
    auto *manager = ctxIt->second->manager;
    // ...or the scroll waits on these handlers, whose gates run on this node
    orchestrated = std::any_of(ctxIt->second->handlers.begin(), ctxIt->second->handlers.end(),
                               [manager](const NativeGestureHandler *h) {
                                 return h != nullptr && (manager->hasWaitRelations(*h) || manager->isWaitTarget(*h));
                               });
    if (orchestrated) {
      break;
    }
  }

  if (!orchestrated) {
    auto *current = OH_ArkUI_ParallelInnerGestureEvent_GetCurrentRecognizer(event);
    ArkUI_GestureDirectionMask currentMask = GESTURE_DIRECTION_ALL;
    if (current == nullptr || OH_ArkUI_GetPanGestureDirectionMask(current, &currentMask) != 0) {
      currentMask = GESTURE_DIRECTION_ALL;
    }
    for (auto *conflict : conflictList) {
      if (s_ownedRecognizers.count(conflict) > 0 || !OH_ArkUI_IsBuiltInGesture(conflict)) {
        continue;
      }
      ArkUI_GestureDirectionMask conflictMask = GESTURE_DIRECTION_ALL;
      if (OH_ArkUI_GetPanGestureDirectionMask(conflict, &conflictMask) != 0) {
        continue; // not a pan — no scrollable behind it
      }
      if ((conflictMask & currentMask) != 0) {
        return nullptr;
      }
    }
  }
  for (auto *conflict : conflictList) {
    if (s_ownedRecognizers.count(conflict) > 0) {
      return conflict;
    }
  }
  return nullptr;
}

/** ONE winner per touch, decided the first time the scroll's pan tries to
 * accept; a tie goes to the scroll. CONTINUE is as final as REJECT — ArkUI has
 * no hold, so every competitor still in BEGAN is cancelled here and now. */
ArkUI_GestureInterruptResult NativeGestureManager::judgeNativeScroll(ArkUI_NodeHandle node, const ScrollAsk &ask) {
  const bool horizontal = (ask.mask & GESTURE_DIRECTION_HORIZONTAL) != 0;
  const bool vertical = (ask.mask & GESTURE_DIRECTION_VERTICAL) != 0;
  const bool xDominant = std::abs(ask.offsetXpx) >= std::abs(ask.offsetYpx);
  const bool horizontalOnly = horizontal && !vertical;
  const bool verticalOnly = vertical && !horizontal;
  const bool alongOwnAxis = !ask.hasOffsets || ((!horizontalOnly || xDominant) && (!verticalOnly || !xDominant));

  NativeGestureHandler *nativeView = nullptr;
  // already CANCELLED this touch, so it stays still for the rest of it
  NativeGestureHandler *lostNativeView = nullptr;
  // not in session yet: a touch landing on a flinging list reaches its pan
  // before this node's sentinels open one
  NativeGestureHandler *pendingNativeView = nullptr;
  // only the next touch-down resets a finished session, so a suppressed
  // handler is "this touch" only while the node's touch is live
  auto nodeIt = m_nodes.find(node);
  const bool touchLive = nodeIt != m_nodes.end() && nodeIt->second->touchActive;
  // ScrollView wrapper + Gesture.Native on one node: prefer the gated one
  auto preferGated = [this](NativeGestureHandler *&slot, NativeGestureHandler &h) {
    if (slot == nullptr || (!this->hasWaitRelations(*slot) && this->hasWaitRelations(h))) {
      slot = &h;
    }
  };
  for (auto &[tag, handlerPtr] : m_handlersByTag) {
    auto &h = *handlerPtr;
    if (h.type != GestureType::NATIVE_VIEW || h.node != node) {
      continue;
    }
    if (h.inSession && !h.suppressed) {
      preferGated(nativeView, h);
      continue;
    }
    if (touchLive) {
      if (h.inSession && h.state == CANCELLED && this->hasWaitRelations(h)) {
        lostNativeView = &h;
      }
      continue;
    }
    preferGated(pendingNativeView, h);
  }
  if (nativeView == nullptr && lostNativeView == nullptr && pendingNativeView != nullptr && ask.event != nullptr) {
    pendingNativeView->inSession = false; // leftovers of the previous touch
    this->ensureSessionFromRecognizerEvent(*pendingNativeView, ask.event);
    if (pendingNativeView->inSession && !pendingNativeView->suppressed) {
      nativeView = pendingNativeView;
    }
  }

  const NativeGestureHandler *owner = nullptr;
  const char *reason = "";
  bool holdForWaiter = false;
  std::vector<int> competitorTags;
  for (auto &[tag, handlerPtr] : m_handlersByTag) {
    auto &h = *handlerPtr;
    if (&h == nativeView || !h.inSession || h.suppressed || h.type == GestureType::NATIVE_VIEW ||
        !guardsScroll(h, node)) {
      continue;
    }
    if (nativeView != nullptr && this->areSimultaneous(h, *nativeView)) {
      continue;
    }
    if (h.state == ACTIVE) {
      owner = &h;
      reason = "already ACTIVE";
      break;
    }
    if (h.state != BEGAN) {
      continue;
    }
    if (nativeView != nullptr && this->waitsOnTag(*nativeView, h.handlerTag)) {
      // requireExternalGestureToFail: defer, do not decide — the scroll
      // may take the touch with its frames held until the target settles
      holdForWaiter = true;
      continue;
    }
    if (h.type == GestureType::PAN && this->panWouldHaveActivatedBefore(h, ask)) {
      owner = &h;
      reason = "pan past its activation gates before this sample";
      break;
    }
    competitorTags.push_back(h.handlerTag);
  }

  if (owner != nullptr) {
    LOG(INFO) << "RNGH::judge — REJECT system scroll pan: handlerTag=" << owner->handlerTag
              << " (type=" << static_cast<int>(owner->type) << ", state=" << owner->state << ") " << reason;
    return GESTURE_INTERRUPT_RESULT_REJECT;
  }
  if (nativeView == nullptr && lostNativeView != nullptr) {
    // its frames have been held since the cancel
    LOG(INFO) << "RNGH::judge — REJECT system scroll pan: its NativeView handlerTag=" << lostNativeView->handlerTag
              << " already lost the touch";
    return GESTURE_INTERRUPT_RESULT_REJECT;
  }
  if (holdForWaiter && nativeView != nullptr && nativeView->state == BEGAN) {
    this->setScrollFramesHeld(*nativeView, true);
    this->requestActivation(*nativeView); // held until the targets settle
    LOG(INFO) << "RNGH::judge — CONTINUE, frames held until waitFor targets settle (node=" << node << ")";
    return GESTURE_INTERRUPT_RESULT_CONTINUE;
  }
  if (nativeView != nullptr) {
    // RNOH never lifts a hold itself, so one left by the previous touch
    // (kept through its fling) ends here
    this->setScrollFramesHeld(*nativeView, false);
  }
  if (!alongOwnAxis) {
    LOG(INFO) << "RNGH::judge — CONTINUE (cross-axis ask, no takeover)";
    return GESTURE_INTERRUPT_RESULT_CONTINUE;
  }
  // activating cancels competitors, so it belongs to the takeover
  if (nativeView != nullptr && nativeView->state == BEGAN) {
    this->requestActivation(*nativeView);
  }
  LOG(INFO) << "RNGH::judge — CONTINUE, the scroll takes the touch (node=" << node << ")";
  // with no Gesture.Native there is no prop to honour; the takeover must
  // still clear the field
  const bool interruptibleScroll = nativeView != nullptr && !cfgBool(nativeView->config, "disallowInterruption", false);
  for (int tag : competitorTags) {
    auto *h = this->findHandler(tag);
    if (h == nullptr || !h->inSession || h->suppressed || h->state != BEGAN || h->type == GestureType::MANUAL) {
      continue;
    }
    if (interruptibleScroll && !h->held && h->spec->survivesScrollTakeover(h->config)) {
      continue;
    }
    this->failHandler(*h, /* cancelled */ true);
  }
  return GESTURE_INTERRUPT_RESULT_CONTINUE;
}

/** Our sentinels sit shallower, so they normally lag one sample behind; when
 * they did consume the current one (same window point), the one before it is
 * used. Either way a gate first met on the scroll's sample is its win.
 * Asked in window space, the same the gates are evaluated in: the node's local
 * point is un-transformed by ArkUI, so on a view scaled up by a pinch the local
 * distance is a fraction of the finger's and the scroll would win every drag. */
bool NativeGestureManager::panWouldHaveActivatedBefore(const NativeGestureHandler &handler,
                                                       const ScrollAsk &ask) const {
  double dx = panTranslationX(handler);
  double dy = panTranslationY(handler);
  if (ask.hasWindowPoint && std::abs(handler.lastAbsX * handler.scale - ask.windowXpx) < SAME_SAMPLE_TOLERANCE_PX &&
      std::abs(handler.lastAbsY * handler.scale - ask.windowYpx) < SAME_SAMPLE_TOLERANCE_PX) {
    dx = handler.panTranslationBeforeX;
    dy = handler.panTranslationBeforeY;
  }
  return this->panActivationGatesSatisfied(handler, dx, dy);
}

} // namespace rnoh
