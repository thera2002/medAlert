#pragma once

#include <QObject>

class NotificationService : public QObject {
    Q_OBJECT

public:
    explicit NotificationService(QObject *parent = nullptr);
    bool sendNotification(const QString &title, const QString &message, const QString &iconName);
    bool sendLowStockNotification(const QString &message);
};