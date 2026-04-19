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

bool NotificationService::sendLowStockNotification(const QString &message)
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
        QStringLiteral("dialog-warning"),
        QStringLiteral("Scorte medicine in esaurimento"),
        message,
        QStringList(),
        hints,
        15000);

    return reply.type() != QDBusMessage::ErrorMessage;
}