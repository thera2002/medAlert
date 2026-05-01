#include "mainwindow.h"

#include "notificationservice.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QColor>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <QLineEdit>

namespace {

QString quotedSystemdExec(const QString &path)
{
    QString escaped = path;
    escaped.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    escaped.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    return QStringLiteral("\"") + escaped + QStringLiteral("\"");
}

QString serviceFileContent()
{
    return QStringLiteral(
        "[Unit]\n"
        "Description=medAlert daily medicine check\n"
        "After=graphical-session.target\n"
        "PartOf=graphical-session.target\n"
        "\n"
        "[Service]\n"
        "Type=oneshot\n"
        "ExecStart=%1 --check\n").arg(quotedSystemdExec(QCoreApplication::applicationFilePath()));
}

QString timerFileContent()
{
    return QStringLiteral(
        "[Unit]\n"
        "Description=Run medAlert daily at 23:00\n"
        "\n"
        "[Timer]\n"
        "OnCalendar=*-*-* 23:00:00\n"
        "Persistent=true\n"
        "Unit=medalert.service\n"
        "\n"
        "[Install]\n"
        "WantedBy=timers.target\n");
}

bool runUserSystemctl(const QStringList &arguments, QString *errorMessage)
{
    QProcess process;
    process.start(QStringLiteral("systemctl"), QStringList{QStringLiteral("--user")} + arguments);
    if (!process.waitForFinished()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("systemctl --user non ha completato l'operazione.");
        }
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (errorMessage != nullptr) {
            const QString stderrText = QString::fromUtf8(process.readAllStandardError()).trimmed();
            const QString stdoutText = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
            *errorMessage = !stderrText.isEmpty() ? stderrText : stdoutText;
        }
        return false;
    }

    return true;
}

} // namespace

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
    centralWidget->setObjectName(QStringLiteral("mainPanel"));
    auto *mainLayout = new QVBoxLayout(centralWidget);
    auto *buttonLayout = new QHBoxLayout();

    mainLayout->setContentsMargins(14, 14, 14, 14);
    mainLayout->setSpacing(10);

    auto *addButton = new QPushButton(QStringLiteral("Aggiungi farmaco"), this);
    auto *moveUpButton = new QPushButton(QStringLiteral(""), this);
    auto *moveDownButton = new QPushButton(QStringLiteral(""), this);
    auto *editButton = new QPushButton(QStringLiteral("Modifica scheda"), this);
    auto *removeButton = new QPushButton(QStringLiteral("Rimuovi"), this);
    auto *selectedNamesButton = new QPushButton(QStringLiteral("Mostra selezionati"), this);
    auto *installTimerButton = new QPushButton(QStringLiteral("Attiva timer 23:00"), this);
    auto *runNowButton = new QPushButton(QStringLiteral("Esegui controllo ora"), this);

    moveUpButton->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    moveDownButton->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));

    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Farmaco"),
        QStringLiteral("Unità/conf."),
        QStringLiteral("Unità/die"),
        QStringLiteral("Stato corr."),
        QStringLiteral("Scorta conf."),
        QStringLiteral("Soglia notif."),
        QStringLiteral("Stand-by")
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_table->setColumnWidth(0, 280);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(moveUpButton);
    buttonLayout->addWidget(moveDownButton);
    buttonLayout->addWidget(editButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addWidget(selectedNamesButton);
    buttonLayout->addWidget(installTimerButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(runNowButton);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(m_table);
    mainLayout->addWidget(m_statusLabel);
    setCentralWidget(centralWidget);

    setStyleSheet(QStringLiteral(
        "QWidget#mainPanel {"
        " border: 3px solid #2f7d32;"
        " border-radius: 10px;"
        " background-color: palette(window);"
        "}"
    ));

    connect(addButton, &QPushButton::clicked, this, &MainWindow::addMedicine);
    connect(moveUpButton, &QPushButton::clicked, this, &MainWindow::moveSelectedMedicineUp);
    connect(moveDownButton, &QPushButton::clicked, this, &MainWindow::moveSelectedMedicineDown);
    connect(editButton, &QPushButton::clicked, this, &MainWindow::editSelectedMedicine);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeSelectedMedicine);
    connect(selectedNamesButton, &QPushButton::clicked, this, &MainWindow::showSelectedMedicines);
    connect(installTimerButton, &QPushButton::clicked, this, &MainWindow::installUserTimer);
    connect(runNowButton, &QPushButton::clicked, this, &MainWindow::triggerManualCheck);
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
    const QColor lowStockRowColor(QStringLiteral("#ffd9d9"));

    for (int row = 0; row < medicines.size(); ++row) {
        const Medicine &medicine = medicines[row];
        const bool isBelowThreshold = medicine.stockBoxes == 0
            && medicine.currentPills <= medicine.alertThreshold;

        auto *nameItem = new QTableWidgetItem(medicine.name);
        auto *pillsPerBoxItem = new QTableWidgetItem(QString::number(medicine.pillsPerBox));
        auto *dailyPillsItem = new QTableWidgetItem(QString::number(medicine.dailyPills));
        auto *currentPillsItem = new QTableWidgetItem(QString::number(medicine.currentPills));
        auto *stockBoxesItem = new QTableWidgetItem(QString::number(medicine.stockBoxes));
        auto *alertThresholdItem = new QTableWidgetItem(QString::number(medicine.alertThreshold));
        auto *standbyItem = new QTableWidgetItem(medicine.standby ? QStringLiteral("Sì") : QStringLiteral("No"));

        if (isBelowThreshold) {
            nameItem->setBackground(lowStockRowColor);
            pillsPerBoxItem->setBackground(lowStockRowColor);
            dailyPillsItem->setBackground(lowStockRowColor);
            currentPillsItem->setBackground(lowStockRowColor);
            stockBoxesItem->setBackground(lowStockRowColor);
            alertThresholdItem->setBackground(lowStockRowColor);
            standbyItem->setBackground(lowStockRowColor);
        }

        m_table->setItem(row, 0, nameItem);
        m_table->setItem(row, 1, pillsPerBoxItem);
        m_table->setItem(row, 2, dailyPillsItem);
        m_table->setItem(row, 3, currentPillsItem);
        m_table->setItem(row, 4, stockBoxesItem);
        m_table->setItem(row, 5, alertThresholdItem);
        m_table->setItem(row, 6, standbyItem);
    }

    m_statusLabel->setText(QStringLiteral("Archivio JSON: %1").arg(m_store.storagePath()));
    scheduleNextDailyUpdate();
}

void MainWindow::addMedicine()
{
    Medicine medicine;
    if (!editMedicineDialog(medicine)) {
        return;
    }
    m_store.upsertMedicine(-1, medicine);
}

void MainWindow::moveSelectedMedicineUp()
{
    moveSelectedMedicine(-1);
}

void MainWindow::moveSelectedMedicineDown()
{
    moveSelectedMedicine(1);
}

void MainWindow::moveSelectedMedicine(int offset)
{
    const int row = selectedRow();
    if (row < 0) {
        QMessageBox::information(this,
                                 QStringLiteral("medAlert"),
                                 QStringLiteral("Seleziona un farmaco da spostare."));
        return;
    }

    const QVector<Medicine> currentMedicines = m_store.medicines();
    const int targetRow = row + offset;
    if (targetRow < 0 || targetRow >= currentMedicines.size()) {
        return;
    }

    QVector<Medicine> reorderedMedicines = currentMedicines;
    qSwap(reorderedMedicines[row], reorderedMedicines[targetRow]);
    m_store.setMedicines(reorderedMedicines);
    m_table->selectRow(targetRow);
}

void MainWindow::editSelectedMedicine()
{
    const int row = selectedRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("medAlert"), QStringLiteral("Seleziona un farmaco da modificare."));
        return;
    }

    Medicine medicine = m_store.medicines().at(row);
    if (!editMedicineDialog(medicine)) {
        return;
    }
    m_store.upsertMedicine(row, medicine);
}

void MainWindow::removeSelectedMedicine()
{
    const int row = selectedRow();
    if (row < 0) {
        return;
    }

    const QString medicineName = m_store.medicines().at(row).name;
    QMessageBox confirmationBox(this);
    confirmationBox.setIcon(QMessageBox::Warning);
    confirmationBox.setWindowTitle(QStringLiteral("Conferma rimozione"));
    confirmationBox.setText(QStringLiteral("Vuoi davvero rimuovere questo farmaco?"));
    confirmationBox.setInformativeText(
        QStringLiteral("Il farmaco \"%1\" verrà eliminato dall'archivio.").arg(medicineName));
    confirmationBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmationBox.setDefaultButton(QMessageBox::No);

    if (confirmationBox.exec() == QMessageBox::Yes) {
        m_store.removeMedicine(row);
    }
}

void MainWindow::showSelectedMedicines()
{
    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        QMessageBox::information(this,
                                 QStringLiteral("Farmaci selezionati"),
                                 QStringLiteral("Seleziona uno o più farmaci dalla tabella."));
        return;
    }

    QStringList names;
    names.reserve(rows.size());
    for (const QModelIndex &row : rows) {
        names << m_store.medicines().at(row.row()).name;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Farmaci selezionati"));
    dialog.setModal(true);
    dialog.resize(480, 360);

    auto *layout = new QVBoxLayout(&dialog);
    auto *textView = new QPlainTextEdit(&dialog);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);

    textView->setReadOnly(true);
    textView->setPlainText(names.join(QStringLiteral("\n")));

    layout->addWidget(textView);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);

    dialog.exec();
}

void MainWindow::installUserTimer()
{
    QString errorMessage;
    if (!installSystemdUserTimer(&errorMessage)) {
        QMessageBox::warning(this,
                             QStringLiteral("Attivazione timer"),
                             errorMessage);
        return;
    }

    QMessageBox::information(this,
                             QStringLiteral("Attivazione timer"),
                             QStringLiteral("Il timer utente medalert.timer è stato installato e abilitato alle 23:00."));
}

void MainWindow::triggerManualCheck()
{
    m_store.processPendingDailyUpdates(false);

    const QString message = m_store.lowStockSummary();
    if (message.isEmpty()) {
        showNotification(QStringLiteral("Controllo completato"),
                         QStringLiteral("Non c'è nulla da ordinare."),
                         false);
    } else {
        showNotification(QStringLiteral("Scorte medicine in esaurimento"),
                         message,
                         true);
    }

    scheduleNextDailyUpdate();
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
    showNotification(QStringLiteral("Scorte medicine in esaurimento"), message, true);
}

void MainWindow::showNotification(const QString &title, const QString &message, bool warning)
{
    const bool notified = m_notifications->sendNotification(
        title,
        message,
        warning ? QStringLiteral("dialog-warning") : QStringLiteral("dialog-information"));

    if (!notified) {
        if (warning) {
            QMessageBox::warning(this, title, message);
        } else {
            QMessageBox::information(this, title, message);
        }
    }
}

bool MainWindow::installSystemdUserTimer(QString *errorMessage) const
{
    const QString configRoot = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    if (configRoot.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Impossibile determinare la directory di configurazione utente.");
        }
        return false;
    }

    const QString systemdDir = configRoot + QStringLiteral("/systemd/user");
    if (!QDir().mkpath(systemdDir)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Impossibile creare la directory %1.").arg(systemdDir);
        }
        return false;
    }

    QFile serviceFile(systemdDir + QStringLiteral("/medalert.service"));
    if (!serviceFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Impossibile scrivere il file service utente.");
        }
        return false;
    }
    QTextStream(&serviceFile) << serviceFileContent();
    serviceFile.close();

    QFile timerFile(systemdDir + QStringLiteral("/medalert.timer"));
    if (!timerFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Impossibile scrivere il file timer utente.");
        }
        return false;
    }
    QTextStream(&timerFile) << timerFileContent();
    timerFile.close();

    if (!runUserSystemctl({QStringLiteral("daemon-reload")}, errorMessage)) {
        return false;
    }
    if (!runUserSystemctl({QStringLiteral("enable"), QStringLiteral("--now"), QStringLiteral("medalert.timer")}, errorMessage)) {
        return false;
    }

    return true;
}

int MainWindow::selectedRow() const
{
    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return -1;
    }
    return rows.first().row();
}

bool MainWindow::editMedicineDialog(Medicine &medicine)
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Scheda farmaco"));
    dialog.setMinimumWidth(440);

    auto *layout = new QVBoxLayout(&dialog);
    auto *formLayout = new QFormLayout();
    auto *nameEdit = new QLineEdit(medicine.name, &dialog);
    auto *pillsPerBoxSpin = new QSpinBox(&dialog);
    auto *dailyPillsSpin = new QSpinBox(&dialog);
    auto *currentPillsSpin = new QSpinBox(&dialog);
    auto *stockBoxesSpin = new QSpinBox(&dialog);
    auto *alertThresholdSpin = new QSpinBox(&dialog);
    auto *standbyCheck = new QCheckBox(QStringLiteral("Farmaco in stand-by"), &dialog);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);

    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setFormAlignment(Qt::AlignTop);
    formLayout->setContentsMargins(0, 10, 0, 0);

    pillsPerBoxSpin->setRange(1, 10000);
    dailyPillsSpin->setRange(0, 1000);
    currentPillsSpin->setRange(0, 10000);
    stockBoxesSpin->setRange(0, 10000);
    alertThresholdSpin->setRange(0, 10000);

    pillsPerBoxSpin->setValue(qMax(1, medicine.pillsPerBox));
    dailyPillsSpin->setValue(qMax(0, medicine.dailyPills));
    currentPillsSpin->setValue(qMax(0, medicine.currentPills));
    stockBoxesSpin->setValue(qMax(0, medicine.stockBoxes));
    alertThresholdSpin->setValue(qMax(0, medicine.alertThreshold));
    standbyCheck->setChecked(medicine.standby);

    formLayout->addRow(QStringLiteral("Nome farmaco"), nameEdit);
    formLayout->addRow(QStringLiteral("Unità per confezione"), pillsPerBoxSpin);
    formLayout->addRow(QStringLiteral("Unità assunte al giorno"), dailyPillsSpin);
    formLayout->addRow(QStringLiteral("Stato corrente (unità)"), currentPillsSpin);
    formLayout->addRow(QStringLiteral("Magazzino (confezioni)"), stockBoxesSpin);
    formLayout->addRow(QStringLiteral("Soglia notifica (unità)"), alertThresholdSpin);
    formLayout->addRow(QString(), standbyCheck);

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
    medicine.pillsPerBox = pillsPerBoxSpin->value();
    medicine.dailyPills = dailyPillsSpin->value();
    medicine.currentPills = currentPillsSpin->value();
    medicine.stockBoxes = stockBoxesSpin->value();
    medicine.alertThreshold = alertThresholdSpin->value();
    medicine.standby = standbyCheck->isChecked();
    medicine.lowStockNotified = false;
    return true;
}