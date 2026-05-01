#pragma once

#include "medicinestore.h"

#include <QMainWindow>

class QLabel;
class QString;
class QPushButton;
class QTableWidget;
class QTimer;

class NotificationService;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void refreshTable();
    void addMedicine();
    void moveSelectedMedicineUp();
    void moveSelectedMedicineDown();
    void editSelectedMedicine();
    void removeSelectedMedicine();
    void showSelectedMedicines();
    void installUserTimer();
    void triggerManualCheck();
    void scheduleNextDailyUpdate();
    void runDailyUpdate();
    void onLowStockAlert(const QString &message);

private:
    int selectedRow() const;
    bool editMedicineDialog(Medicine &medicine);
    bool installSystemdUserTimer(QString *errorMessage = nullptr) const;
    void moveSelectedMedicine(int offset);
    void showNotification(const QString &title, const QString &message, bool warning);

    MedicineStore m_store;
    NotificationService *m_notifications = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_statusLabel = nullptr;
    QTimer *m_dailyTimer = nullptr;
};