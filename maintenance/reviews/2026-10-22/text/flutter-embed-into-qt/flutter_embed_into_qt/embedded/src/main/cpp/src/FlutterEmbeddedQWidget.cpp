/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FlutterEmbeddedQWidget.h"
#include <QtCore/qcoreevent.h>
#include "QJsonTool.h"
#include <QWindow>
#include "Embeddedcomponent.h"
#include "EwNode.h"

int FlutterEmbeddedQWidget::g_count = 0;
const QString TAG = "FlutterEmbeddedQWidget";
FlutterEmbeddedEventHandler::FlutterEmbeddedEventHandler(const QString &name,
    std::weak_ptr<FlutterEmbeddedQWidget> flutterPagePtr)
    : EB::IMessageHandler(name), flutterPageWeakPtr(flutterPagePtr) {}

void FlutterEmbeddedEventHandler::onMessage(const QString &fun, const QString &data)
{
    if (fun == "HasDestroy") {
        qDebug() << "FlutterEmbeddedQWidget Process：window destroy";
    } else if (fun == "FlutterSendQT") {
        if (messageCB) {
            messageCB(data);
        }
    }
}

void FlutterEmbeddedEventHandler::setCallBack(std::function<void(const QString&)> cb)
{
    messageCB = cb;
}

FlutterEmbeddedQWidget::FlutterEmbeddedQWidget(QString route):flutterRouterPage(route)
{
    g_count +=1;
    id = "FlutterQWidget" + QString::number(g_count);
}

FlutterEmbeddedQWidget::~FlutterEmbeddedQWidget()
{
    if (wrapperWidget != nullptr) {
        qDebug()<<"FlutterEmbeddedQWidget ~FlutterEmbeddedQWidget entry";
    }
}

int FlutterEmbeddedQWidget::sendMessageToEmbedded(QString message)
{
    QJsonObject dataObj;
    dataObj.insert("handleName", id);
    dataObj.insert("message", message);
    eventHandler->invokeMethod(id, "QTSendFlutter", QJsonToQString(dataObj));
    return 0;
}

std::shared_ptr<FlutterEmbeddedEventHandler> FlutterEmbeddedQWidget::GetEventHandler(void)
{
    return eventHandler;
}

void FlutterEmbeddedQWidget::RegisterHandler(std::shared_ptr<FlutterEmbeddedEventHandler> handler)
{
    eventHandler = handler;
}

class EventFilter : public QObject {
    Q_OBJECT

public:
    explicit EventFilter(FlutterEmbeddedQWidget *widget, QObject *parent = nullptr)
        : QObject(parent), mWidget(widget) {}

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (mWidget->GetInnerWidget() == watched) {
            mWidget->dispatchEvent(event);
        }
        return QObject::eventFilter(watched, event);
    }
private:
    FlutterEmbeddedQWidget* mWidget;
};

void FlutterEmbeddedQWidget::dispatchEvent(const QEvent* event)
{
    QJsonObject dataObj;
    dataObj.insert("handleName", id);
    dataObj.insert("event", QString::number(static_cast<int>(event->type())));
    eventHandler->invokeMethod(id, "QTDispatchEvent", QJsonToQString(dataObj));
}

void FlutterEmbeddedQWidget::init(QWidget* parent)
{
    Node *nodeptr = new Node();
    ArkUI_NodeHandle nativehandle = EmbeddedComponent::CreateComponent(id, flutterRouterPage);
    if (nativehandle == nullptr) {
        qCritical() << "nativehandle is nullptr";
    }
    nodeptr->content = nativehandle;
    QWindow *foreignWindow = QWindow::fromWinId(reinterpret_cast<WId>(nodeptr));
    if (foreignWindow == nullptr) {
        qCritical() << "foreignWindow is nullptr";
        return;
    }
    auto widget = QWidget::createWindowContainer(foreignWindow, parent);
    if (widget == nullptr) {
        qCritical() << "wrapperWidget is nullptr";
    }
    wrapperWidget.reset(widget);
    qDebug() << "FlutterEmbeddedQWidget Process：mount node";
    EventFilter *eventFilter = new EventFilter(this);
    wrapperWidget->installEventFilter(eventFilter);
}

std::shared_ptr<FlutterEmbeddedQWidget> FlutterEmbeddedQWidget::Create(QWidget* parent, QString route)
{
    std::shared_ptr<FlutterEmbeddedQWidget> result(new FlutterEmbeddedQWidget(route));
    if (result == nullptr) {
        return nullptr;
    }
    std::shared_ptr<FlutterEmbeddedEventHandler> handler =
        std::make_shared<FlutterEmbeddedEventHandler>(result->getIdStr(), result);
    if (handler == nullptr) {
        return nullptr;
    }
    result->RegisterHandler(handler);
    result->init(parent);
    return result;
}

QString FlutterEmbeddedQWidget::getIdStr() { return id; }

void FlutterEmbeddedQWidget::Destroy()
{
    if (eventHandler != nullptr) {
        qDebug()<<"FlutterEmbeddedQWidget ~Destory NotifyWidgetDestory";
        eventHandler->NotifyWidgetDestroy(id);
    }
}

QWidget* FlutterEmbeddedQWidget::GetInnerWidget()
{
    return wrapperWidget.get();
}

void FlutterEmbeddedQWidget::Destroy(std::shared_ptr<FlutterEmbeddedQWidget>  & widget)
{
    if (widget !=nullptr) {
        widget->Destroy();
    }
    
    if (widget != nullptr) {
        widget.reset();
    }
}

#include "FlutterEmbeddedQWidget.moc"