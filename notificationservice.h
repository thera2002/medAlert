#pragma once

#include <QObject>

class NotificationService : public QObject {
    Q_OBJECT

public:
    explicit NotificationService(QObject *parent = nullptr);
    bool sendLowStockNotification(const QString &message);
};