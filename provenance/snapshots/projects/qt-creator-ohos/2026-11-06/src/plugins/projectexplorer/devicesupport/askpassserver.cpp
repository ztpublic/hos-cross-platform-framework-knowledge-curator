// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "askpassserver.h"

#include <coreplugin/icore.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QPointer>

namespace ProjectExplorer {

// ============================================================================
// AskpassServerWorker — runs on a dedicated QThread
// ============================================================================

AskpassServerWorker::AskpassServerWorker(QObject *parent)
    : QObject(parent)
{
}

AskpassServerWorker::~AskpassServerWorker()
{
    stop();
}

bool AskpassServerWorker::start()
{
    if (m_server)
        return true;

    m_server = new QLocalServer(this);

    const Utils::FilePath sockFilePath = Core::ICore::userResourcePath("qtc-askpass.sock");
    const Utils::FilePath parentDir = sockFilePath.parentDir();
    if (!parentDir.isEmpty())
        QDir().mkpath(parentDir.path());
    m_socketPath = sockFilePath.path();

    QFile::remove(m_socketPath);

    if (!m_server->listen(m_socketPath)) {
        qWarning() << "AskpassServer: listen failed";
        delete m_server;
        m_server = nullptr;
        m_socketPath.clear();
        return false;
    }

    connect(m_server, &QLocalServer::newConnection,
            this, &AskpassServerWorker::onNewConnection);

    qInfo() << "AskpassServer: listening";
    return true;
}

void AskpassServerWorker::stop()
{
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
    if (!m_socketPath.isEmpty())
        QFile::remove(m_socketPath);
    m_socketPath.clear();
    m_buffers.clear();
}

void AskpassServerWorker::onNewConnection()
{
    while (QLocalSocket *client = m_server->nextPendingConnection()) {
        qDebug() << "AskpassServer: new connection";
        connect(client, &QLocalSocket::readyRead,
                this, &AskpassServerWorker::onReadyRead);
        connect(client, &QLocalSocket::disconnected,
                this, &AskpassServerWorker::onClientDisconnected);
        m_buffers.insert(client, QByteArray());
    }
}

void AskpassServerWorker::onReadyRead()
{
    QLocalSocket *client = qobject_cast<QLocalSocket *>(sender());
    if (!client) {
        qWarning() << "AskpassServer: onReadyRead - sender is not a QLocalSocket";
        return;
    }

    m_buffers[client] += client->readAll();

    while (true) {
        const int idx = m_buffers[client].indexOf('\n');
        if (idx < 0)
            break;

        const QString prompt = QString::fromUtf8(m_buffers[client].left(idx));
        m_buffers[client] = m_buffers[client].mid(idx + 1);

        emit passwordRequested(client, prompt);
    }
}

void AskpassServerWorker::onClientDisconnected()
{
    QLocalSocket *client = qobject_cast<QLocalSocket *>(sender());
    if (!client) {
        qWarning() << "AskpassServer: onClientDisconnected - sender is not a QLocalSocket";
        return;
    }
    qDebug() << "AskpassServer: client disconnected";
    m_buffers.remove(client);
    client->deleteLater();
}

void AskpassServerWorker::sendPassword(QLocalSocket *client, const QString &password)
{
    if (!client) {
        qWarning() << "AskpassServer: sendPassword - null client";
        return;
    }
    if (!m_buffers.contains(client)) {
        qWarning() << "AskpassServer: sendPassword - client not in buffers (disconnected?)";
        return;
    }
    const QByteArray data = password.toUtf8() + '\n';
    client->write(data);
    client->flush();
    client->disconnectFromServer();
    qDebug() << "AskpassServer: password sent";
}

void AskpassServerWorker::sendCancel(QLocalSocket *client)
{
    if (!client) {
        qWarning() << "AskpassServer: sendCancel - null client";
        return;
    }
    if (!m_buffers.contains(client)) {
        qWarning() << "AskpassServer: sendCancel - client not in buffers (disconnected?)";
        return;
    }
    client->write("\n");
    client->flush();
    client->disconnectFromServer();
    qDebug() << "AskpassServer: cancel sent";
}

// ============================================================================
// AskpassServer — facade on the main thread, delegates to worker
// ============================================================================

AskpassServer &AskpassServer::instance()
{
    static AskpassServer s;
    return s;
}

AskpassServer::AskpassServer()
{
    m_thread.setObjectName("AskpassServer");
}

AskpassServer::~AskpassServer()
{
    stop();
}

bool AskpassServer::start()
{
    if (m_worker)
        return true;

    m_worker = new AskpassServerWorker;
    m_worker->moveToThread(&m_thread);

    // Forward worker signal to facade (auto = queued, since cross-thread)
    connect(m_worker, &AskpassServerWorker::passwordRequested,
            this, &AskpassServer::passwordRequested);

    m_thread.start();

    // Call worker->start() on the worker thread (blocking to get result).
    // This is called once at startup; the brief block is acceptable.
    bool result = false;
    QMetaObject::invokeMethod(m_worker, [this, &result] {
        result = m_worker->start();
    }, Qt::BlockingQueuedConnection);

    m_socketPath = m_worker->socketPath();
    m_running = result;
    return result;
}

void AskpassServer::stop()
{
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, [this] {
            m_worker->stop();
        }, Qt::BlockingQueuedConnection);
        m_thread.quit();
        m_thread.wait();
        delete m_worker;
        m_worker = nullptr;
    }
    m_socketPath.clear();
    m_running = false;
}

void AskpassServer::sendPassword(QLocalSocket *client, const QString &password)
{
    if (!m_worker) {
        qWarning() << "AskpassServer: sendPassword - worker is null";
        return;
    }
    // Use QPointer to avoid use-after-free if client disconnects before the
    // queued call executes on the worker thread.
    QPointer<QLocalSocket> ptr(client);
    QMetaObject::invokeMethod(m_worker, [this, ptr, password] {
        if (ptr) {
            m_worker->sendPassword(ptr, password);
        } else {
            qWarning() << "AskpassServer: sendPassword - client disconnected before queued call";
        }
    }, Qt::QueuedConnection);
}

void AskpassServer::sendCancel(QLocalSocket *client)
{
    if (!m_worker) {
        qWarning() << "AskpassServer: sendCancel - worker is null";
        return;
    }
    QPointer<QLocalSocket> ptr(client);
    QMetaObject::invokeMethod(m_worker, [this, ptr] {
        if (ptr) {
            m_worker->sendCancel(ptr);
        } else {
            qWarning() << "AskpassServer: sendCancel - client disconnected before queued call";
        }
    }, Qt::QueuedConnection);
}

} // namespace ProjectExplorer
