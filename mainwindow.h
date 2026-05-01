#pragma once

#include "medicinestore.h"

#include <QMainWindow>

class QEvent;
class QLabel;
class QTime;
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
    void showSettingsDialog();
    void installUserTimer();
    void triggerManualCheck();
    void scheduleNextDailyUpdate();
    void runDailyUpdate();
    void onLowStockAlert(const QString &message);

protected:
    void changeEvent(QEvent *event) override;

private:
    int selectedRow() const;
    bool editMedicineDialog(Medicine &medicine);
    bool installSystemdUserTimer(QString *errorMessage = nullptr) const;
    void moveSelectedMedicine(int offset);
    void retranslateUi();
    void showNotification(const QString &title, const QString &message, bool warning);
    QString languageDisplayName(const QString &languageCode) const;

    MedicineStore m_store;
    NotificationService *m_notifications = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_moveUpButton = nullptr;
    QPushButton *m_moveDownButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_removeButton = nullptr;
    QPushButton *m_selectedNamesButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QPushButton *m_installTimerButton = nullptr;
    QPushButton *m_runNowButton = nullptr;
    QTime *m_updateTime = nullptr;
    QTimer *m_dailyTimer = nullptr;
};