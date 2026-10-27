/*
 * Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file.
 */
#pragma once
#include "RNOH/arkui/ScrollNode.h"
#include "RNOHCorePackage/ComponentInstances/ScrollViewComponentInstance.h"
#include <arkui/native_node.h>
#include <memory>

namespace rnoh {

/** While held, the scroll keeps the touch but its content does not move: the
 * only way to postpone a verdict ArkUI asks for once per touch. */
class ScrollFramesHold : public ScrollNodeDelegate {
public:
  /** Null when there is nothing to proxy. */
  [[nodiscard]] static std::unique_ptr<ScrollFramesHold>
  installOn(std::shared_ptr<ScrollViewComponentInstance> scrollView);

  ~ScrollFramesHold() override;

  ScrollFramesHold(const ScrollFramesHold &) = delete;
  ScrollFramesHold &operator=(const ScrollFramesHold &) = delete;

  void setHeld(bool held);

  /** False once the proxied ScrollView has been unmounted. */
  [[nodiscard]] bool ownerAlive() const noexcept { return !m_scrollView.expired(); }

  // ScrollNodeDelegate
  void onScroll() override;
  void onScrollStart() override;
  void onScrollStop() override;
  float onScrollFrameBegin(float offset, int32_t scrollState) override;
  void onAppear() override;

private:
  ScrollFramesHold(std::shared_ptr<ScrollViewComponentInstance> scrollView, ScrollNode &node);

  /** A withheld frame reads as unconsumed, so ArkUI hands the whole delta to
   * the parent and the page travels at twice the finger. */
  void forceSelfOnly();
  void restoreNestedScroll();
  [[nodiscard]] bool atEdge() const;
  [[nodiscard]] bool readNestedScroll(ArkUI_ScrollNestedMode &forward, ArkUI_ScrollNestedMode &backward) const;

  std::weak_ptr<ScrollViewComponentInstance> m_scrollView;
  ScrollNode &m_node;
  bool m_held = false;
  bool m_forcedSelfOnly = false;
  ArkUI_ScrollNestedMode m_savedForward = ARKUI_SCROLL_NESTED_MODE_SELF_FIRST;
  ArkUI_ScrollNestedMode m_savedBackward = ARKUI_SCROLL_NESTED_MODE_SELF_FIRST;
};

} // namespace rnoh
