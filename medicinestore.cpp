#include "appsettings.h"
#include "medicinestore.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QStandardPaths>

namespace {

QString currentStorageDirectory()
{
    return AppSettings::dataDirectory();
}

QDate effectiveProcessingDate(const QDateTime &now)
{
    const QTime cutoff(23, 0);
    if (now.time() >= cutoff) {
        return now.date();
    }
    return now.date().addDays(-1);
}

QJsonObject medicineToJson(const Medicine &medicine)
{
    QJsonObject object;
    object["name"] = medicine.name;
    object["pillsPerBox"] = medicine.pillsPerBox;
    object["dailyPills"] = medicine.dailyPills;
    object["currentPills"] = medicine.currentPills;
    object["stockBoxes"] = medicine.stockBoxes;
    object["alertThreshold"] = medicine.alertThreshold;
    object["standby"] = medicine.standby;
    object["lowStockNotified"] = medicine.lowStockNotified;
    return object;
}

Medicine medicineFromJson(const QJsonObject &object)
{
    Medicine medicine;
    medicine.name = object["name"].toString();
    medicine.pillsPerBox = object["pillsPerBox"].toInt();
    medicine.dailyPills = object["dailyPills"].toInt();
    medicine.currentPills = object["currentPills"].toInt();
    medicine.stockBoxes = object["stockBoxes"].toInt();
    medicine.alertThreshold = object.contains("alertThreshold") ? object["alertThreshold"].toInt() : 10;
    medicine.standby = object["standby"].toBool();
    medicine.lowStockNotified = object["lowStockNotified"].toBool();
    return medicine;
}

} // namespace

MedicineStore::MedicineStore(QObject *parent)
    : QObject(parent)
{
    const QString baseDir = currentStorageDirectory();
    QDir().mkpath(baseDir);
    m_storagePath = baseDir + "/medicines.json";
}

bool MedicineStore::load()
{
    QFile file(m_storagePath);
    if (!file.exists()) {
        m_lastProcessedDate = QDate::currentDate().addDays(-1);
        return save();
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return false;
    }

    const QJsonObject root = document.object();
    m_lastProcessedDate = QDate::fromString(root["lastProcessedDate"].toString(), Qt::ISODate);
    if (!m_lastProcessedDate.isValid()) {
        m_lastProcessedDate = QDate::currentDate().addDays(-1);
    }

    m_medicines.clear();
    const QJsonArray medicines = root["medicines"].toArray();
    m_medicines.reserve(medicines.size());
    for (const QJsonValue &value : medicines) {
        if (value.isObject()) {
            m_medicines.append(medicineFromJson(value.toObject()));
        }
    }

    emit dataChanged();
    return true;
}

bool MedicineStore::save() const
{
    QFile file(m_storagePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QJsonArray medicines;
    for (const Medicine &medicine : m_medicines) {
        medicines.append(medicineToJson(medicine));
    }

    QJsonObject root;
    root["lastProcessedDate"] = m_lastProcessedDate.toString(Qt::ISODate);
    root["medicines"] = medicines;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

const QVector<Medicine> &MedicineStore::medicines() const
{
    return m_medicines;
}

QString MedicineStore::storagePath() const
{
    return m_storagePath;
}

void MedicineStore::setMedicines(const QVector<Medicine> &medicines)
{
    m_medicines = medicines;
    save();
    emit dataChanged();
}

void MedicineStore::upsertMedicine(int index, const Medicine &medicine)
{
    if (index >= 0 && index < m_medicines.size()) {
        m_medicines[index] = medicine;
    } else {
        m_medicines.append(medicine);
    }

    save();
    emit dataChanged();
}

void MedicineStore::removeMedicine(int index)
{
    if (index < 0 || index >= m_medicines.size()) {
        return;
    }

    m_medicines.removeAt(index);
    save();
    emit dataChanged();
}

void MedicineStore::setInventory(int index, int currentPills, int stockBoxes)
{
    if (index < 0 || index >= m_medicines.size()) {
        return;
    }

    Medicine &medicine = m_medicines[index];
    medicine.currentPills = qMax(0, currentPills);
    medicine.stockBoxes = qMax(0, stockBoxes);
    medicine.lowStockNotified = false;
    save();
    emit dataChanged();
}

QString MedicineStore::processPendingDailyUpdates(bool emitNotification)
{
    const QDate targetDate = effectiveProcessingDate(QDateTime::currentDateTime());
    if (!m_lastProcessedDate.isValid()) {
        m_lastProcessedDate = targetDate.addDays(-1);
    }

    if (m_lastProcessedDate >= targetDate) {
        return QString();
    }

    while (m_lastProcessedDate < targetDate) {
        processSingleDay();
        m_lastProcessedDate = m_lastProcessedDate.addDays(1);
    }

    save();
    emit dataChanged();

    const QString message = buildLowStockMessage();
    if (emitNotification && !message.isEmpty()) {
        emit lowStockAlert(message);
    }
    return message;
}

QString MedicineStore::lowStockSummary() const
{
    return buildLowStockMessage();
}

QString MedicineStore::buildLowStockMessage() const
{
    QStringList lines;
    for (const Medicine &medicine : m_medicines) {
        if (needsLowStockAlert(medicine)) {
            lines.append(tr("%1: %2 units left, stock depleted (threshold %3)")
                             .arg(medicine.name)
                             .arg(medicine.currentPills)
                             .arg(medicine.alertThreshold));
        }
    }
    return lines.join('\n');
}

void MedicineStore::processSingleDay()
{
    for (Medicine &medicine : m_medicines) {
        if (medicine.standby || medicine.dailyPills <= 0) {
            continue;
        }

        medicine.currentPills = qMax(0, medicine.currentPills - medicine.dailyPills);

        while (medicine.currentPills == 0 && medicine.stockBoxes > 0) {
            medicine.stockBoxes -= 1;
            medicine.currentPills = medicine.pillsPerBox;
            medicine.lowStockNotified = false;
            medicine.currentPills = qMax(0, medicine.currentPills - medicine.dailyPills);
        }

        if (!needsLowStockAlert(medicine)) {
            medicine.lowStockNotified = false;
        } else if (!medicine.lowStockNotified) {
            medicine.lowStockNotified = true;
        }
    }
}

bool MedicineStore::needsLowStockAlert(const Medicine &medicine) const
{
    return medicine.stockBoxes == 0
    && medicine.currentPills <= medicine.alertThreshold;
}