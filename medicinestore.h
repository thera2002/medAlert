#pragma once

#include <QDate>
#include <QObject>
#include <QString>
#include <QVector>

struct Medicine {
    QString name;
    int pillsPerBox = 0;
    int dailyPills = 0;
    int currentPills = 0;
    int stockBoxes = 0;
    bool lowStockNotified = false;
};

class MedicineStore : public QObject {
    Q_OBJECT

public:
    explicit MedicineStore(QObject *parent = nullptr);

    bool load();
    bool save() const;

    const QVector<Medicine> &medicines() const;
    QString storagePath() const;

    void setMedicines(const QVector<Medicine> &medicines);
    void upsertMedicine(int index, const Medicine &medicine);
    void removeMedicine(int index);
    void setInventory(int index, int currentPills, int stockBoxes);

    QString processPendingDailyUpdates();

signals:
    void dataChanged();
    void lowStockAlert(const QString &message);

private:
    QString buildLowStockMessage() const;
    void processSingleDay();
    bool needsLowStockAlert(const Medicine &medicine) const;

    QVector<Medicine> m_medicines;
    QDate m_lastProcessedDate;
    QString m_storagePath;
};