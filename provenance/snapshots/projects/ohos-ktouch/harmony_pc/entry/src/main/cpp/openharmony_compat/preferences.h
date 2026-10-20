#pragma once

#include <QColor>
#include <QSettings>
#include <array>

class Preferences
{
public:
    static Preferences *self()
    {
        static Preferences instance;
        return &instance;
    }

    static QString keyboardLayoutName()
    {
        return settings().value(QStringLiteral("General/KeyboardLayoutName"), QStringLiteral("us")).toString();
    }

    static bool enforceTypingErrorCorrection()
    {
        return settings().value(QStringLiteral("Training/EnforceTypingErrorCorrection"), true).toBool();
    }

    static bool showKeyboard()
    {
        return settings().value(QStringLiteral("Training/ShowKeyboard"), true).toBool();
    }

    static void setShowKeyboard(bool value)
    {
        setValue(QStringLiteral("Training/ShowKeyboard"), value);
    }

    static bool showStatistics()
    {
        return settings().value(QStringLiteral("Training/ShowStatistics"), true).toBool();
    }

    static void setShowStatistics(bool value)
    {
        setValue(QStringLiteral("Training/ShowStatistics"), value);
    }

    static bool nextLineWithReturn()
    {
        return settings().value(QStringLiteral("Training/NextLineWithReturn"), true).toBool();
    }

    static void setNextLineWithReturn(bool value)
    {
        setValue(QStringLiteral("Training/NextLineWithReturn"), value);
    }

    static bool nextLineWithSpace()
    {
        return settings().value(QStringLiteral("Training/NextLineWithSpace"), false).toBool();
    }

    static void setNextLineWithSpace(bool value)
    {
        setValue(QStringLiteral("Training/NextLineWithSpace"), value);
    }

    static int requiredStrokesPerMinute()
    {
        return settings().value(QStringLiteral("Training/RequiredStrokesPerMinute"), 180).toInt();
    }

    static void setRequiredStrokesPerMinute(int value)
    {
        setValue(QStringLiteral("Training/RequiredStrokesPerMinute"), value);
    }

    static double requiredAccuracy()
    {
        return settings().value(QStringLiteral("Training/RequiredAccuracy"), 98.0).toDouble();
    }

    static void setRequiredAccuracy(double value)
    {
        setValue(QStringLiteral("Training/RequiredAccuracy"), value);
    }

    static int lastUsedProfileId()
    {
        return settings().value(QStringLiteral("Session/LastUsedProfileId"), -1).toInt();
    }

    static void setLastUsedProfileId(int value)
    {
        setValue(QStringLiteral("Session/LastUsedProfileId"), value);
    }

    static QColor fingerColor(int index)
    {
        static const std::array<const char *, 8> defaults = {
            "#ff0000", "#0000ff", "#00ff00", "#ffff00",
            "#ff00ff", "#00ff00", "#0000ff", "#ff0000"
        };
        if (index < 0 || index >= static_cast<int>(defaults.size())) {
            index = 0;
        }
        const QString key = QStringLiteral("Colors/FingerColor%1").arg(index);
        return QColor(settings().value(key, QString::fromLatin1(defaults.at(index))).toString());
    }

    static double fingerOpacity()
    {
        return settings().value(QStringLiteral("Colors/FingerOpacity"), 12.5).toDouble();
    }

    static void setFingerOpacity(double value)
    {
        setValue(QStringLiteral("Colors/FingerOpacity"), value);
    }

    void save()
    {
        settings().sync();
    }

private:
    Preferences() = default;

    static QSettings &settings()
    {
        static QSettings settings(QStringLiteral("KDE"), QStringLiteral("KTouch"));
        return settings;
    }

    static void setValue(const QString &key, const QVariant &value)
    {
        settings().setValue(key, value);
    }
};
