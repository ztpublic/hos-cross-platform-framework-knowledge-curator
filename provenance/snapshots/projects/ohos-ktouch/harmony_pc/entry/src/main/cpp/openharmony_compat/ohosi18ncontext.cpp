#include "ohosi18ncontext.h"

OhosI18nContext::OhosI18nContext(QObject *parent)
    : QObject(parent)
{
}

QString OhosI18nContext::i18n(const QString &text) const
{
    return text;
}

QString OhosI18nContext::i18n(const QString &text, const QVariant &arg1) const
{
    return format(text, arg1);
}

QString OhosI18nContext::i18n(const QString &text, const QVariant &arg1, const QVariant &arg2) const
{
    return format(text, arg1, arg2);
}

QString OhosI18nContext::i18n(const QString &text, const QVariant &arg1, const QVariant &arg2, const QVariant &arg3) const
{
    return format(text, arg1, arg2, arg3);
}

QString OhosI18nContext::i18nc(const QString &context, const QString &text) const
{
    Q_UNUSED(context)
    return text;
}

QString OhosI18nContext::i18nc(const QString &context, const QString &text, const QVariant &arg1) const
{
    Q_UNUSED(context)
    return format(text, arg1);
}

QString OhosI18nContext::i18nc(const QString &context, const QString &text, const QVariant &arg1, const QVariant &arg2) const
{
    Q_UNUSED(context)
    return format(text, arg1, arg2);
}

QString OhosI18nContext::format(QString text, const QVariant &arg1, const QVariant &arg2, const QVariant &arg3)
{
    if (arg1.isValid()) {
        text = text.arg(arg1.toString());
    }
    if (arg2.isValid()) {
        text = text.arg(arg2.toString());
    }
    if (arg3.isValid()) {
        text = text.arg(arg3.toString());
    }
    return text;
}
