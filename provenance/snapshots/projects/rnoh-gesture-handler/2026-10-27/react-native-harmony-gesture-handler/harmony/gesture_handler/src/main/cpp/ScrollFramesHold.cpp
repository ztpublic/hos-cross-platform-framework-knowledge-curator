/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */
#include "ScrollFramesHold.h"
#include "RNOH/arkui/NativeNodeApi.h"
#include <glog/logging.h>

namespace rnoh {

constexpr facebook::react::Float SCROLL_EDGE_EPSILON = 0.001;

std::unique_ptr<ScrollFramesHold> ScrollFramesHold::installOn(std::shared_ptr<ScrollViewComponentInstance> scrollView) {
  if (scrollView == nullptr) {
    return nullptr;
  }
  auto &node = scrollView->getLocalRootArkUINode();
  auto hold = std::unique_ptr<ScrollFramesHold>(new ScrollFramesHold(scrollView, node));
  node.setScrollNodeDelegate(hold.get());
  return hold;
}

ScrollFramesHold::ScrollFramesHold(std::shared_ptr<ScrollViewComponentInstance> scrollView, ScrollNode &node)
    : m_scrollView(std::move(scrollView)), m_node(node) {}

ScrollFramesHold::~ScrollFramesHold() {
  auto owner = m_scrollView.lock();
  if (owner == nullptr) {
    // the node is a member of the ScrollView, so it went with it
    return;
  }
  if (m_held) {
    m_held = false;
    this->restoreNestedScroll();
  }
  m_node.setScrollNodeDelegate(owner.get());
}

void ScrollFramesHold::setHeld(bool held) {
  if (m_held == held) {
    return;
  }
  m_held = held;
  if (held) {
    this->forceSelfOnly();
  } else {
    this->restoreNestedScroll();
  }
}

float ScrollFramesHold::onScrollFrameBegin(float offset, int32_t scrollState) {
  if (m_held) {
    // Consuming nothing keeps the touch with ArkUI's recognizer. The
    // ScrollView is deliberately not told about the frame: a held scroll
    // has no drag to start or stop.
    return 0;
  }
  auto owner = m_scrollView.lock();
  if (owner == nullptr) {
    return offset;
  }
  return static_cast<ScrollNodeDelegate *>(owner.get())->onScrollFrameBegin(offset, scrollState);
}

void ScrollFramesHold::onScroll() {
  auto owner = m_scrollView.lock();
  if (owner == nullptr) {
    return;
  }
  static_cast<ScrollNodeDelegate *>(owner.get())->onScroll();
}

void ScrollFramesHold::onScrollStart() {
  auto owner = m_scrollView.lock();
  if (owner == nullptr) {
    return;
  }
  static_cast<ScrollNodeDelegate *>(owner.get())->onScrollStart();
}

void ScrollFramesHold::onScrollStop() {
  auto owner = m_scrollView.lock();
  if (owner == nullptr) {
    return;
  }
  static_cast<ScrollNodeDelegate *>(owner.get())->onScrollStop();
  // a scroll resting at its end hands the edge back to its parent here; a
  // held one keeps it until the release, which restores this newer mode
  if (m_held) {
    this->forceSelfOnly();
  }
}

void ScrollFramesHold::onAppear() {
  auto owner = m_scrollView.lock();
  if (owner == nullptr) {
    return;
  }
  static_cast<ScrollNodeDelegate *>(owner.get())->onAppear();
}

bool ScrollFramesHold::readNestedScroll(ArkUI_ScrollNestedMode &forward, ArkUI_ScrollNestedMode &backward) const {
  auto *api = NativeNodeApi::getInstance();
  if (api == nullptr) {
    return false;
  }
  const auto *item = api->getAttribute(m_node.getArkUINodeHandle(), NODE_SCROLL_NESTED_SCROLL);
  if (item == nullptr || item->value == nullptr || item->size < 2) {
    // the hold still withholds frames, but stops keeping the delta
    LOG(WARNING) << "RNGH::hold — could not read the nested scroll mode (size=" << (item != nullptr ? item->size : -1)
                 << ")";
    return false;
  }
  forward = static_cast<ArkUI_ScrollNestedMode>(item->value[0].i32);
  backward = static_cast<ArkUI_ScrollNestedMode>(item->value[1].i32);
  return true;
}

void ScrollFramesHold::forceSelfOnly() {
  ArkUI_ScrollNestedMode forward = ARKUI_SCROLL_NESTED_MODE_SELF_FIRST;
  ArkUI_ScrollNestedMode backward = ARKUI_SCROLL_NESTED_MODE_SELF_FIRST;
  if (!this->readNestedScroll(forward, backward)) {
    return;
  }
  if (forward == ARKUI_SCROLL_NESTED_MODE_SELF_ONLY && backward == ARKUI_SCROLL_NESTED_MODE_SELF_ONLY) {
    return; // already keeps the delta to itself; nothing to give back
  }
  m_savedForward = forward;
  m_savedBackward = backward;
  m_forcedSelfOnly = true;
  m_node.setNestedScroll(ARKUI_SCROLL_NESTED_MODE_SELF_ONLY);
}

void ScrollFramesHold::restoreNestedScroll() {
  if (!m_forcedSelfOnly) {
    return;
  }
  m_forcedSelfOnly = false;
  if (!this->atEdge()) {
    // Away from an edge the ScrollView has narrowed the mode itself and
    // believes it owns it; it decides when the parent may scroll again.
    return;
  }
  ArkUI_ScrollNestedMode forward = ARKUI_SCROLL_NESTED_MODE_SELF_ONLY;
  ArkUI_ScrollNestedMode backward = ARKUI_SCROLL_NESTED_MODE_SELF_ONLY;
  if (this->readNestedScroll(forward, backward) &&
      (forward != ARKUI_SCROLL_NESTED_MODE_SELF_ONLY || backward != ARKUI_SCROLL_NESTED_MODE_SELF_ONLY)) {
    // a props change set its own mode since the hold began; newer than
    // the snapshot, so leave it alone
    return;
  }
  m_node.setNestedScroll(m_savedForward, m_savedBackward);
}

bool ScrollFramesHold::atEdge() const {
  auto owner = m_scrollView.lock();
  if (owner == nullptr) {
    return false;
  }
  const auto metrics = owner->getScrollViewMetrics();
  const auto settled = [](facebook::react::Float content, facebook::react::Float container,
                          facebook::react::Float offset) {
    const auto travel = content - container;
    return travel < SCROLL_EDGE_EPSILON || offset <= SCROLL_EDGE_EPSILON || travel - offset < SCROLL_EDGE_EPSILON;
  };
  return settled(metrics.contentSize.height, metrics.containerSize.height, metrics.contentOffset.y) &&
         settled(metrics.contentSize.width, metrics.containerSize.width, metrics.contentOffset.x);
}

} // namespace rnoh
