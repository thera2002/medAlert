#pragma once

#include "medicinestore.h"

#include <QMainWindow>

class QLabel;
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
    void editSelectedMedicine();
    void updateInventory();
    void removeSelectedMedicine();
    void showSelectedMedicines();
    void scheduleNextDailyUpdate();
    void runDailyUpdate();
    void onLowStockAlert(const QString &message);

private:
    int selectedRow() const;
    bool editMedicineDialog(Medicine &medicine, bool editInventory);

    MedicineStore m_store;
    NotificationService *m_notifications = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_statusLabel = nullptr;
    QTimer *m_dailyTimer = nullptr;
};