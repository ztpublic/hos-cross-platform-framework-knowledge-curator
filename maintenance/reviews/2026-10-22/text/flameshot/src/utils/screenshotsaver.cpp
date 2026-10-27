// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "screenshotsaver.h"
#include "core/flameshot.h"
#include "core/flameshotdaemon.h"
#include "utils/abstractlogger.h"
#include "utils/confighandler.h"
#include "utils/desktopinfo.h"
#include "utils/filenamehandler.h"
#include "utils/globalvalues.h"

#include <QByteArray>
#include <QDebug>
#include <QImageWriter>
#include <QPixmap>
#include <QProcess>
#include <QThread>
#include <QTemporaryFile>

#if USE_WAYLAND_CLIPBOARD
#include <KSystemClipboard>
#endif

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMimeData>
#include <QPointer>
#include <QStandardPaths>
#include <QTimer>
#include <qimagewriter.h>
#include <qmimedatabase.h>
#if defined(Q_OS_MACOS)
#include "widgets/capture/capturewidget.h"
#endif

bool saveToFilesystem(const QPixmap& capture,
                      const QString& path,
                      const QString& messagePrefix)
{
    QString completePath = FileNameHandler().properScreenshotPath(
      path, ConfigHandler().saveAsFileExtension());
    QFile file{ completePath };
    bool okay = false;

    if (file.open(QIODevice::WriteOnly)) {
        QString saveExtension;
        saveExtension = QFileInfo(completePath).suffix().toLower();
        if (saveExtension == "jpg" || saveExtension == "jpeg") {
            okay = capture.save(&file, nullptr, ConfigHandler().jpegQuality());
        } else {
            okay = capture.save(&file);
        }

        QString saveMessage = messagePrefix;
        QString notificationPath = completePath;
        if (!saveMessage.isEmpty()) {
            saveMessage += " ";
        }

        if (okay) {
            saveMessage += QObject::tr("Capture saved as ") + completePath;
            AbstractLogger::info().attachNotificationPath(notificationPath)
              << saveMessage;
        } else {
            saveMessage +=
              QObject::tr("Error trying to save as ") + completePath;
            if (file.error() != QFile::NoError) {
                saveMessage += ": " + file.errorString();
            }
            notificationPath = "";
            AbstractLogger::error().attachNotificationPath(notificationPath)
              << saveMessage;
        }
    }
    return okay;
}

QString ShowSaveFileDialog(const QString& title, const QString& directory)
{
#ifdef Q_OS_OHOS
    // On OHOS the QFileDialog maps to the native DocumentViewPicker. The
    // picker must attach to a stable foreground ability, so make sure we
    // have a visible parent: prefer the active window, fall back to any
    // visible top-level widget (e.g. the launcher, after the capture
    // window hid itself).
    QWidget* parent = nullptr;
    if (QWidget* active = QApplication::activeWindow()) {
        parent = active;
    } else {
        const auto topLevels = QApplication::topLevelWidgets();
        for (QWidget* w : topLevels) {
            if (w->isVisible()) {
                parent = w;
                break;
            }
        }
    }
    // Pass only the directory and pre-select the file name. The Qt OHOS
    // file dialog helper derives the picker's newFileNames option from the
    // selected file; passing the full path as the "directory" left
    // newFileNames empty and the picker fell back to odd defaults
    // ("files_3.ppm").
    QFileInfo pathInfo(directory);
    QFileDialog dialog(parent, title, pathInfo.dir().absolutePath());
    if (!pathInfo.fileName().isEmpty()) {
        dialog.selectFile(pathInfo.fileName());
    }
#else
    QFileDialog dialog(nullptr, title, directory);
#endif
    dialog.setAcceptMode(QFileDialog::AcceptSave);

    // Build string list of supported image formats
    QStringList mimeTypeList;
    for (const auto& mimeType : QImageWriter::supportedMimeTypes()) {
        // image/heif has several aliases and they cause glitch in save dialog
        // It is necessary to keep the image/heif (otherwise HEIF plug-in from
        // kimageformats will not work) but the aliases could be filtered out.
        if (mimeType != "image/heic" && mimeType != "image/heic-sequence" &&
            mimeType != "image/heif-sequence") {
            mimeTypeList.append(mimeType);
        }
    }

    QString suffix = ConfigHandler().saveAsFileExtension();
    if (suffix.isEmpty()) {
        suffix = "png";
    }
    QString defaultMimeType =
      QMimeDatabase().mimeTypeForFile("image." + suffix).name();

#ifdef Q_OS_OHOS
    // The native DocumentViewPicker applies the FIRST suffix choice by
    // default and its extension OVERRIDES the pre-filled file name's
    // extension. QImageWriter::supportedMimeTypes() lists image/ppm first on
    // this build, so screenshots were saved as ".ppm" - an extension no
    // installed app can open (the system "open with" app list came up
    // empty). Move the configured format's mime type to the front so the
    // picker defaults to it instead.
    if (mimeTypeList.removeAll(defaultMimeType) > 0) {
        mimeTypeList.prepend(defaultMimeType);
    } else if (mimeTypeList.removeAll(QStringLiteral("image/png")) > 0) {
        mimeTypeList.prepend(QStringLiteral("image/png"));
    }
#endif
    dialog.setMimeTypeFilters(mimeTypeList);

    dialog.selectMimeTypeFilter(defaultMimeType);
    dialog.setDefaultSuffix(suffix);
    if (dialog.exec() == QDialog::Accepted) {
        return dialog.selectedFiles().constFirst();
    } else {
        return {};
    }
}

void saveToClipboardMime(const QPixmap& capture, const QString& imageType)
{
    QByteArray array;
    QBuffer buffer{ &array };
    QImageWriter imageWriter{ &buffer, imageType.toUpper().toUtf8() };
    if (imageType == "jpeg") {
        imageWriter.setQuality(ConfigHandler().jpegQuality());
    }
    imageWriter.write(capture.toImage());

    QPixmap formattedPixmap;
    bool isLoaded =
      formattedPixmap.loadFromData(reinterpret_cast<uchar*>(array.data()),
                                   array.size(),
                                   imageType.toUpper().toUtf8());
    if (isLoaded) {

        auto* mimeData = new QMimeData();

#if defined(Q_OS_MACOS)
        // setImageData provides the image in a native pasteboard format
        // (public.tiff) that macOS can always access. Also include the
        // original format bytes for apps that prefer PNG/JPEG.
        mimeData->setImageData(formattedPixmap.toImage());
        mimeData->setData("image/" + imageType, array);
        QApplication::clipboard()->setMimeData(mimeData);
#elif defined(USE_WAYLAND_CLIPBOARD)
        if (QGuiApplication::platformName() == "wayland") {
            mimeData->setImageData(formattedPixmap.toImage());
            mimeData->setData(QStringLiteral("x-kde-force-image-copy"),
                              QByteArray());
            KSystemClipboard::instance()->setMimeData(mimeData,
                                                      QClipboard::Clipboard);
        } else {
            mimeData->setData("image/" + imageType, array);
            QApplication::clipboard()->setMimeData(mimeData);
        }
#else
        mimeData->setData("image/" + imageType, array);
        QApplication::clipboard()->setMimeData(mimeData);
#endif

    } else {
        AbstractLogger::error()
          << QObject::tr("Error while saving to clipboard");
    }
}

// If data is saved to the clipboard before the notification is sent via
// dbus, the application freezes.
void saveToClipboard(const QPixmap& capture)
{
    // If we are able to properly save the file, save the file and copy to
    // clipboard.
    if ((ConfigHandler().saveAfterCopy()) &&
        (!ConfigHandler().savePath().isEmpty())) {
        saveToFilesystem(capture,
                         ConfigHandler().savePath(),
                         QObject::tr("Capture saved to clipboard."));
    } else {
        AbstractLogger() << QObject::tr("Capture saved to clipboard.");
    }
#if defined(Q_OS_MACOS)
    // On macOS, setPixmap uses lazy clipboard which fails when the
    // process exits ("Cannot keep promise"). Always use serialized bytes.
    saveToClipboardMime(capture,
                        ConfigHandler().useJpgForClipboard() ? "jpeg" : "png");
#else
    if (ConfigHandler().useJpgForClipboard()) {
        saveToClipboardMime(capture, "jpeg");
    } else {
#if defined(Q_OS_UNIX)
        if (DesktopInfo().waylandDetected()) {
            saveToClipboardMime(capture, "png");
        } else {
            QApplication::clipboard()->setPixmap(capture);
        }
#else
        QApplication::clipboard()->setPixmap(capture);
#endif
    }
#endif
}

class ClipboardWatcherMimeData : public QMimeData
{
public:
    ClipboardWatcherMimeData(const QImage& img, QWidget* owner)
      : m_image(img)
      , m_owner(owner)
    {}

protected:
    QStringList formats() const override
    {
        return { QStringLiteral("image/png"),
                 QStringLiteral("application/x-qt-image") };
    }

    QVariant retrieveData(const QString& mimeType,
                          QMetaType type) const override
    {
        if (mimeType == QLatin1String("application/x-qt-image")) {
            notifyOwner();
            return QVariant::fromValue(m_image);
        }
        if (mimeType == QLatin1String("image/png")) {
            QByteArray ba;
            QBuffer buffer(&ba);
            buffer.open(QIODevice::WriteOnly);
            m_image.save(&buffer, "PNG");
            notifyOwner();
            return ba;
        }
        auto result = QMimeData::retrieveData(mimeType, type);
        if (result.isValid())
            notifyOwner();
        return result;
    }

private:
    void notifyOwner() const
    {
        if (m_notified || m_owner.isNull())
            return;
        m_notified = true;
        AbstractLogger::info() << QObject::tr("Capture saved to clipboard.");
    }

    QImage m_image;
    mutable bool m_notified{ false };
    QPointer<QWidget> m_owner;
};

bool saveToClipboardGnomeWorkaround(const QPixmap& pixmap, QWidget* keepAlive)
{
    auto* mimeData = new ClipboardWatcherMimeData(pixmap.toImage(), keepAlive);
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setMimeData(mimeData);

    keepAlive->hide();

    // Close once something else takes clipboard ownership, rather than
    // after the first read: some paste consumers probe the clipboard
    // (e.g. checking available types) before issuing the real fetch, and
    // closing on that first read would cut them off before it arrives.
    // dataChanged also fires for our own setMimeData() call above (the
    // Wayland clipboard claim is confirmed asynchronously by the
    // compositor), so we only close once the clipboard's current data is
    // no longer ours, rather than closing on the first emission.
    QObject::connect(clipboard,
                     &QClipboard::dataChanged,
                     keepAlive,
                     [clipboard, mimeData, keepAlive]() {
                         if (clipboard->mimeData() == mimeData)
                             return;
                         if (keepAlive)
                             keepAlive->close();
                     });

    // Safety net: force close if nothing ever fetches the data. GNOME's
    // mutter grabs clipboard offers eagerly on copy, so 500ms was enough
    // there, but compositors that fetch lazily (e.g. COSMIC) only request
    // the data once the user actually pastes, which can take much longer.
    QTimer::singleShot(30000, keepAlive, [keepAlive]() {
        qWarning() << "Clipboard workaround timed out, compositor did not "
                      "request clipboard data within 30s. Force closing.";
        if (keepAlive)
            keepAlive->close();
    });

    return true;
}

bool saveToFilesystemGUI(const QPixmap& capture)
{
    bool okay = false;
    ConfigHandler config;
    QString defaultSavePath = ConfigHandler().savePath();
#ifdef Q_OS_OHOS
    // Diagnostic (visible in hilog): what does the configured path and each
    // QStandardPaths candidate resolve to on this device, and are they
    // usable? The configured path is often a FileKit URI-internal path
    // (e.g. "/storage/Users/currentUser/Images") which is NOT a usable
    // filesystem path for the app - it exists only in the file://docs URI
    // space, so every exists()/isWritable() check fails on it.
    qWarning("[OHOS-SAVE-DIAG] configured savePath='%s' exists=%d writable=%d",
             qPrintable(defaultSavePath),
             QDir(defaultSavePath).exists(),
             QFileInfo(defaultSavePath).isWritable());
    {
        const QString desktop =
          QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        const QString documents =
          QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        const QString download =
          QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        const QString appData =
          QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        qWarning("[OHOS-SAVE-DIAG] Desktop='%s' exists=%d writable=%d",
                 qPrintable(desktop),
                 QDir(desktop).exists(),
                 QFileInfo(desktop).isWritable());
        qWarning("[OHOS-SAVE-DIAG] Documents='%s' exists=%d writable=%d",
                 qPrintable(documents),
                 QDir(documents).exists(),
                 QFileInfo(documents).isWritable());
        qWarning("[OHOS-SAVE-DIAG] Download='%s' exists=%d writable=%d",
                 qPrintable(download),
                 QDir(download).exists(),
                 QFileInfo(download).isWritable());
        qWarning("[OHOS-SAVE-DIAG] AppData='%s' exists=%d writable=%d",
                 qPrintable(appData),
                 QDir(appData).exists(),
                 QFileInfo(appData).isWritable());
    }
#endif
#if defined(Q_OS_OHOS)
    // Writability of the DEFAULT dir only matters when the file is written
    // directly (savePathFixed, no dialog). When the save picker is shown it
    // grants the app write access to the saved file, so a read-only public
    // dir is still a valid picker start location (verified on device: the
    // per-app FUSE mounts show Desktop/Documents/Download as read-only
    // without a picker grant, yet saving through the picker works).
    const bool directWrite = config.savePathFixed();
    if (defaultSavePath.isEmpty() || !QDir(defaultSavePath).exists() ||
        (directWrite && !QFileInfo(defaultSavePath).isWritable())) {
        // QStandardPaths::PicturesLocation is NOT SUPPORTED by the Qt OHOS
        // port and returns "" (qstandardpaths_ohos.cpp). The OHOS
        // environment API only exposes Desktop/Documents/Download. Prefer a
        // PUBLIC user dir so that screenshots are visible in the file
        // manager and openable by other apps (the system preview resolves
        // ".png" through the extension-derived UTD type - verified on
        // device: ImplicitQueryInfos finds HiPreview as the default
        // application). Saving into the app sandbox made the files
        // invisible to the file manager and unopenable from outside.
        const QStringList candidates = {
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
        };
        for (const QString& candidate : candidates) {
            if (candidate.isEmpty() || !QDir(candidate).exists()) {
                continue;
            }
            if (directWrite && !QFileInfo(candidate).isWritable()) {
                continue;
            }
            defaultSavePath = candidate;
            break;
        }
        if (defaultSavePath.isEmpty() || !QDir(defaultSavePath).exists()) {
            // Last resort: the app sandbox files dir
            defaultSavePath = QStandardPaths::writableLocation(
              QStandardPaths::AppDataLocation);
            QDir saveDir(defaultSavePath);
            if (!saveDir.exists()) {
                saveDir.mkpath(".");
            }
        }
    }
#else
    if (defaultSavePath.isEmpty() || !QDir(defaultSavePath).exists() ||
        !QFileInfo(defaultSavePath).isWritable()) {
        defaultSavePath =
          QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    }
#endif
    QString savePath = FileNameHandler().properScreenshotPath(
      defaultSavePath, ConfigHandler().saveAsFileExtension());
#if defined(Q_OS_MACOS)
    for (QWidget* widget : qApp->topLevelWidgets()) {
        QString className(widget->metaObject()->className());
        if (0 ==
            className.compare(CaptureWidget::staticMetaObject.className())) {
            widget->showNormal();
            widget->hide();
            break;
        }
    }
#endif
    if (!config.savePathFixed()) {
        savePath = ShowSaveFileDialog(QObject::tr("Save screenshot"), savePath);
    }
    if (savePath == "") {
        return okay;
    }

    QFile file{ savePath };
    if (file.open(QIODevice::WriteOnly)) {
        QString saveExtension;
        saveExtension = QFileInfo(savePath).suffix().toLower();
        if (saveExtension == "jpg" || saveExtension == "jpeg") {
            okay = capture.save(&file, nullptr, ConfigHandler().jpegQuality());
        } else {
            okay = capture.save(&file);
        }

        if (okay) {
            // Don't use QDir::separator() here, as Qt internally always uses
            // '/'
            QString pathNoFile = savePath.left(savePath.lastIndexOf('/'));

            ConfigHandler().setSavePath(pathNoFile);

            QString msg = QObject::tr("Capture saved as ") + savePath;
            AbstractLogger().attachNotificationPath(savePath) << msg;

            if (config.copyPathAfterSave()) {
#ifdef Q_OS_WIN
                savePath.replace('/', '\\');
#endif
                FlameshotDaemon::copyToClipboard(
                  savePath,
                  QObject::tr("Path copied to clipboard as ") + savePath);
            }

        } else {
            QString msg = QObject::tr("Error trying to save as ") + savePath;

            if (file.error() != QFile::NoError) {
                msg += ": " + file.errorString();
            }

            QMessageBox saveErrBox(
              QMessageBox::Warning, QObject::tr("Save Error"), msg);
            saveErrBox.setWindowIcon(QIcon(GlobalValues::iconPath()));
            saveErrBox.exec();
        }
    }

    return okay;
}
