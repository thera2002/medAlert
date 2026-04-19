#include "notificationservice.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QVariantMap>

NotificationService::NotificationService(QObject *parent)
    : QObject(parent)
{
}

bool NotificationService::sendNotification(const QString &title, const QString &message, const QString &iconName)
{
    QDBusInterface notificationsInterface(
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.freedesktop.Notifications"),
        QDBusConnection::sessionBus());

    if (!notificationsInterface.isValid()) {
        return false;
    }

    QVariantMap hints;
    hints.insert(QStringLiteral("desktop-entry"), QStringLiteral("medAlert"));

    QDBusMessage reply = notificationsInterface.call(
        QStringLiteral("Notify"),
        QStringLiteral("medAlert"),
        static_cast<uint>(0),
        iconName,
        title,
        message,
        QStringList(),
        hints,
        15000);

    return reply.type() != QDBusMessage::ErrorMessage;
}

bool NotificationService::sendLowStockNotification(const QString &message)
{
    return sendNotification(QStringLiteral("Scorte medicine in esaurimento"),
                            message,
                            QStringLiteral("dialog-warning"));
}