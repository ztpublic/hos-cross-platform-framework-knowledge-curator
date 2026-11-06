#include "KeyboardInsetsViewComponentInstance.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "RNOHCorePackage/ComponentInstances/ScrollViewComponentInstance.h"
#include "react/renderer/components/_rnoh_react_native_keyboard_insets/Props.h"

namespace rnoh {
namespace {

std::optional<float> findFocusedBottom(
    ComponentInstance::Shared const& parent,
    float parentTop) {
  auto scrollView =
      std::dynamic_pointer_cast<ScrollViewComponentInstance>(parent);
  const float scrollOffsetY =
      scrollView == nullptr ? 0.0f : scrollView->getCurrentOffset().y;

  for (auto const& child : parent->getChildren()) {
    if (child->getComponentName() == "KeyboardInsetsView") {
      continue;
    }
    const auto frame = child->getLayoutMetrics().frame;
    const float childTop = parentTop + frame.origin.y - scrollOffsetY;
    bool focused = false;
    try {
      focused = child->getLocalRootArkUINode().isFocused();
    } catch (ComponentInstance::NoArkUINodeError const&) {
    }
    if (focused) {
      return childTop + frame.size.height;
    }
    if (auto focusedBottom = findFocusedBottom(child, childTop)) {
      return focusedBottom;
    }
  }
  return std::nullopt;
}

} // namespace

KeyboardInsetsViewComponentInstance::KeyboardInsetsViewComponentInstance(
    Context context)
    : BaseKeyboardInsetsViewComponentInstance(std::move(context)),
      ArkTSMessageHub::Observer(m_deps->arkTSMessageHub),
      m_customNode(m_arkUINodeCtx) {}

void KeyboardInsetsViewComponentInstance::onPropsChanged(
    SharedConcreteProps const& concreteProps) {
  BaseKeyboardInsetsViewComponentInstance::onPropsChanged(concreteProps);
  if (!m_hasKeyboardState) {
    return;
  }
  auto props = std::static_pointer_cast<
      const facebook::react::KeyboardInsetsViewProps>(concreteProps);
  updateAutoTranslation(*props, getLayoutMetrics().frame.size.height);
}

void KeyboardInsetsViewComponentInstance::onLayoutChanged(
    facebook::react::LayoutMetrics const& layoutMetrics) {
  BaseKeyboardInsetsViewComponentInstance::onLayoutChanged(layoutMetrics);
  if (!m_hasKeyboardState) {
    return;
  }
  auto props = std::dynamic_pointer_cast<
      const facebook::react::KeyboardInsetsViewProps>(getProps());
  if (props != nullptr) {
    updateAutoTranslation(*props, layoutMetrics.frame.size.height);
  }
}

void KeyboardInsetsViewComponentInstance::updateAutoTranslation(
    facebook::react::KeyboardInsetsViewProps const& props,
    float containerHeight) {
  if (props.mode != facebook::react::KeyboardInsetsViewMode::Auto ||
      m_systemAvoidsKeyboard || m_lastPosition <= 0) {
    m_customNode.setTranslate(0, 0);
    return;
  }

  const auto focusedBottom = findFocusedBottom(shared_from_this(), 0);
  if (!focusedBottom.has_value()) {
    m_customNode.setTranslate(0, 0);
    return;
  }

  const float keyboardTop = containerHeight - m_lastPosition;
  const float overlap = *focusedBottom + props.extraHeight - keyboardTop;
  m_customNode.setTranslate(0, -std::max(overlap, 0.0f));
}

void KeyboardInsetsViewComponentInstance::onMessageReceived(
    ArkTSMessage const& message) {
  if (message.name != "KeyboardInsets::KeyboardChanged" ||
      !message.payload.isObject() ||
      !message.payload.count("position") ||
      !message.payload.count("height") ||
      !message.payload.count("shown") ||
      !message.payload.count("transitioning") ||
      !message.payload.count("systemAvoidsKeyboard") ||
      !message.payload["position"].isNumber() ||
      !message.payload["height"].isNumber() ||
      !message.payload["shown"].isBool() ||
      !message.payload["transitioning"].isBool() ||
      !message.payload["systemAvoidsKeyboard"].isBool()) {
    return;
  }

  auto props = std::dynamic_pointer_cast<const facebook::react::KeyboardInsetsViewProps>(
      getProps());
  if (props == nullptr) {
    return;
  }

  const float position = static_cast<float>(message.payload["position"].asDouble());
  const float height = static_cast<float>(message.payload["height"].asDouble());
  const bool shown = message.payload["shown"].asBool();
  const bool transitioning = message.payload["transitioning"].asBool();
  const bool systemAvoidsKeyboard =
      message.payload["systemAvoidsKeyboard"].asBool();
  const bool positionChanged = !m_hasKeyboardState || position != m_lastPosition;
  const bool statusChanged = !m_hasKeyboardState || height != m_lastHeight ||
      shown != m_lastShown || transitioning != m_lastTransitioning;

  m_lastPosition = position;
  m_lastHeight = height;
  m_lastShown = shown;
  m_lastTransitioning = transitioning;
  m_systemAvoidsKeyboard = systemAvoidsKeyboard;
  m_hasKeyboardState = true;

  if (props->mode == facebook::react::KeyboardInsetsViewMode::Auto) {
    updateAutoTranslation(*props, getLayoutMetrics().frame.size.height);
    return;
  }

  const bool hasFocusedDescendant =
      findFocusedBottom(shared_from_this(), 0).has_value();
  const bool shouldEmitManualEvents =
      hasFocusedDescendant || m_manualKeyboardRelevant;
  if (hasFocusedDescendant && shown) {
    m_manualKeyboardRelevant = true;
  }
  if (!shouldEmitManualEvents) {
    return;
  }
  if (!shown && !transitioning) {
    m_manualKeyboardRelevant = false;
  }

  auto eventEmitter = getEventEmitter();
  if (eventEmitter == nullptr) {
    return;
  }
  if (statusChanged) {
    eventEmitter->onStatusChanged({
        .height = height,
        .shown = shown,
        .transitioning = transitioning,
    });
  }
  if (positionChanged) {
    eventEmitter->onPositionChanged({.position = position});
  }
}

void KeyboardInsetsViewComponentInstance::onChildInserted(
    ComponentInstance::Shared const& childComponentInstance,
    std::size_t index) {
  CppComponentInstance::onChildInserted(childComponentInstance, index);
  m_customNode.insertChild(
      childComponentInstance->getLocalRootArkUINode(), index);
}

void KeyboardInsetsViewComponentInstance::onChildRemoved(
    ComponentInstance::Shared const& childComponentInstance) {
  CppComponentInstance::onChildRemoved(childComponentInstance);
  m_customNode.removeChild(childComponentInstance->getLocalRootArkUINode());
}

CustomNode& KeyboardInsetsViewComponentInstance::getLocalRootArkUINode() {
  return m_customNode;
}

} // namespace rnoh
