#include "mainwindow.h"

#include "notificationservice.h"

#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QLineEdit>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_notifications(new NotificationService(this))
    , m_table(new QTableWidget(this))
    , m_statusLabel(new QLabel(this))
    , m_dailyTimer(new QTimer(this))
{
    setWindowTitle(QStringLiteral("medAlert"));
    resize(900, 480);

    auto *centralWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(centralWidget);
    auto *buttonLayout = new QHBoxLayout();

    auto *addButton = new QPushButton(QStringLiteral("Aggiungi farmaco"), this);
    auto *editButton = new QPushButton(QStringLiteral("Modifica scheda"), this);
    auto *inventoryButton = new QPushButton(QStringLiteral("Aggiorna scorte"), this);
    auto *removeButton = new QPushButton(QStringLiteral("Rimuovi"), this);
    auto *runNowButton = new QPushButton(QStringLiteral("Esegui controllo ora"), this);

    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Farmaco"),
        QStringLiteral("Pastiglie/conf."),
        QStringLiteral("Assunzione/giorno"),
        QStringLiteral("Stato corrente"),
        QStringLiteral("Scorta confezioni")
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(editButton);
    buttonLayout->addWidget(inventoryButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(runNowButton);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(m_table);
    mainLayout->addWidget(m_statusLabel);
    setCentralWidget(centralWidget);

    connect(addButton, &QPushButton::clicked, this, &MainWindow::addMedicine);
    connect(editButton, &QPushButton::clicked, this, &MainWindow::editSelectedMedicine);
    connect(inventoryButton, &QPushButton::clicked, this, &MainWindow::updateInventory);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeSelectedMedicine);
    connect(runNowButton, &QPushButton::clicked, this, &MainWindow::runDailyUpdate);
    connect(&m_store, &MedicineStore::dataChanged, this, &MainWindow::refreshTable);
    connect(&m_store, &MedicineStore::lowStockAlert, this, &MainWindow::onLowStockAlert);
    connect(m_dailyTimer, &QTimer::timeout, this, &MainWindow::runDailyUpdate);

    if (!m_store.load()) {
        QMessageBox::warning(this,
                             QStringLiteral("medAlert"),
                             QStringLiteral("Impossibile caricare il file JSON di stato. Verrà creato al primo salvataggio."));
    }

    refreshTable();
    runDailyUpdate();
}

void MainWindow::refreshTable()
{
    const QVector<Medicine> medicines = m_store.medicines();
    m_table->setRowCount(medicines.size());

    for (int row = 0; row < medicines.size(); ++row) {
        const Medicine &medicine = medicines[row];
        m_table->setItem(row, 0, new QTableWidgetItem(medicine.name));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::number(medicine.pillsPerBox)));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::number(medicine.dailyPills)));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::number(medicine.currentPills)));
        m_table->setItem(row, 4, new QTableWidgetItem(QString::number(medicine.stockBoxes)));
    }

    m_statusLabel->setText(QStringLiteral("Archivio JSON: %1").arg(m_store.storagePath()));
    scheduleNextDailyUpdate();
}

void MainWindow::addMedicine()
{
    Medicine medicine;
    if (!editMedicineDialog(medicine, true)) {
        return;
    }
    m_store.upsertMedicine(-1, medicine);
}

void MainWindow::editSelectedMedicine()
{
    const int row = selectedRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("medAlert"), QStringLiteral("Seleziona un farmaco da modificare."));
        return;
    }

    Medicine medicine = m_store.medicines().at(row);
    if (!editMedicineDialog(medicine, true)) {
        return;
    }
    m_store.upsertMedicine(row, medicine);
}

void MainWindow::updateInventory()
{
    const int row = selectedRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("medAlert"), QStringLiteral("Seleziona un farmaco da aggiornare."));
        return;
    }

    Medicine medicine = m_store.medicines().at(row);
    if (!editMedicineDialog(medicine, false)) {
        return;
    }
    m_store.setInventory(row, medicine.currentPills, medicine.stockBoxes);
}

void MainWindow::removeSelectedMedicine()
{
    const int row = selectedRow();
    if (row < 0) {
        return;
    }

    const auto answer = QMessageBox::question(this,
                                              QStringLiteral("medAlert"),
                                              QStringLiteral("Rimuovere il farmaco selezionato?"));
    if (answer == QMessageBox::Yes) {
        m_store.removeMedicine(row);
    }
}

void MainWindow::scheduleNextDailyUpdate()
{
    const QDateTime now = QDateTime::currentDateTime();
    QDateTime nextRun(QDate::currentDate(), QTime(23, 0));
    if (now >= nextRun) {
        nextRun = nextRun.addDays(1);
    }

    const qint64 milliseconds = now.msecsTo(nextRun);
    m_dailyTimer->start(static_cast<int>(qMax<qint64>(1000, milliseconds)));
}

void MainWindow::runDailyUpdate()
{
    m_store.processPendingDailyUpdates();
    scheduleNextDailyUpdate();
}

void MainWindow::onLowStockAlert(const QString &message)
{
    const bool notified = m_notifications->sendLowStockNotification(message);
    if (!notified) {
        QMessageBox::warning(this,
                             QStringLiteral("Scorte medicine in esaurimento"),
                             message);
    }
}

int MainWindow::selectedRow() const
{
    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return -1;
    }
    return rows.first().row();
}

bool MainWindow::editMedicineDialog(Medicine &medicine, bool editInventory)
{
    QDialog dialog(this);
    dialog.setWindowTitle(editInventory ? QStringLiteral("Scheda farmaco") : QStringLiteral("Aggiorna scorte"));

    auto *layout = new QVBoxLayout(&dialog);
    auto *formLayout = new QFormLayout();
    auto *nameEdit = new QLineEdit(medicine.name, &dialog);
    auto *pillsPerBoxSpin = new QSpinBox(&dialog);
    auto *dailyPillsSpin = new QSpinBox(&dialog);
    auto *currentPillsSpin = new QSpinBox(&dialog);
    auto *stockBoxesSpin = new QSpinBox(&dialog);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);

    pillsPerBoxSpin->setRange(1, 10000);
    dailyPillsSpin->setRange(0, 1000);
    currentPillsSpin->setRange(0, 10000);
    stockBoxesSpin->setRange(0, 10000);

    pillsPerBoxSpin->setValue(qMax(1, medicine.pillsPerBox));
    dailyPillsSpin->setValue(qMax(0, medicine.dailyPills));
    currentPillsSpin->setValue(qMax(0, medicine.currentPills));
    stockBoxesSpin->setValue(qMax(0, medicine.stockBoxes));

    formLayout->addRow(QStringLiteral("Nome farmaco"), nameEdit);
    if (editInventory) {
        formLayout->addRow(QStringLiteral("Pastiglie per confezione"), pillsPerBoxSpin);
        formLayout->addRow(QStringLiteral("Pastiglie assunte al giorno"), dailyPillsSpin);
    }
    formLayout->addRow(QStringLiteral("Stato corrente (pastiglie)"), currentPillsSpin);
    formLayout->addRow(QStringLiteral("Magazzino (confezioni)"), stockBoxesSpin);

    layout->addLayout(formLayout);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
        if (nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dialog, QStringLiteral("medAlert"), QStringLiteral("Il nome del farmaco è obbligatorio."));
            return;
        }
        dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    medicine.name = nameEdit->text().trimmed();
    if (editInventory) {
        medicine.pillsPerBox = pillsPerBoxSpin->value();
        medicine.dailyPills = dailyPillsSpin->value();
    }
    medicine.currentPills = currentPillsSpin->value();
    medicine.stockBoxes = stockBoxesSpin->value();
    medicine.lowStockNotified = false;
    return true;
}