#include "appsettings.h"
#include "mainwindow.h"

#include "apptranslator.h"
#include "notificationservice.h"

#include <QComboBox>
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
#include <QSignalBlocker>
#include <QCheckBox>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QTime>
#include <QTimeEdit>
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
        "Description=%1\n"
        "After=graphical-session.target\n"
        "PartOf=graphical-session.target\n"
        "\n"
        "[Service]\n"
        "Type=oneshot\n"
        "ExecStart=%2 --check\n")
        .arg(MainWindow::tr("medAlert daily medication check"))
        .arg(quotedSystemdExec(QCoreApplication::applicationFilePath()));
}

    QString timerFileContent(const QTime &updateTime)
{
    return QStringLiteral(
        "[Unit]\n"
        "Description=%1\n"
        "\n"
        "[Timer]\n"
        "OnCalendar=*-*-* %2:00\n"
        "Persistent=true\n"
        "Unit=medalert.service\n"
        "\n"
        "[Install]\n"
        "WantedBy=timers.target\n")
        .arg(MainWindow::tr("Run medAlert daily at %1").arg(updateTime.toString(QStringLiteral("HH:mm"))))
        .arg(updateTime.toString(QStringLiteral("HH:mm")));
}

bool runUserSystemctl(const QStringList &arguments, QString *errorMessage)
{
    QProcess process;
    process.start(QStringLiteral("systemctl"), QStringList{QStringLiteral("--user")} + arguments);
    if (!process.waitForFinished()) {
        if (errorMessage != nullptr) {
            *errorMessage = MainWindow::tr("The systemctl --user command did not complete.");
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
    , m_addButton(new QPushButton(this))
    , m_moveUpButton(new QPushButton(QStringLiteral(""), this))
    , m_moveDownButton(new QPushButton(QStringLiteral(""), this))
    , m_editButton(new QPushButton(this))
    , m_removeButton(new QPushButton(this))
    , m_selectedNamesButton(new QPushButton(this))
    , m_settingsButton(new QPushButton(this))
    , m_installTimerButton(new QPushButton(this))
    , m_runNowButton(new QPushButton(this))
    , m_updateTime(new QTime(AppSettings::loadUpdateTime()))
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

    m_moveUpButton->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    m_moveDownButton->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));

    m_table->setColumnCount(7);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_table->setColumnWidth(0, 280);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_moveUpButton);
    buttonLayout->addWidget(m_moveDownButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_removeButton);
    buttonLayout->addWidget(m_selectedNamesButton);
    buttonLayout->addWidget(m_settingsButton);
    buttonLayout->addWidget(m_installTimerButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_runNowButton);

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

    retranslateUi();

    connect(m_addButton, &QPushButton::clicked, this, &MainWindow::addMedicine);
    connect(m_moveUpButton, &QPushButton::clicked, this, &MainWindow::moveSelectedMedicineUp);
    connect(m_moveDownButton, &QPushButton::clicked, this, &MainWindow::moveSelectedMedicineDown);
    connect(m_editButton, &QPushButton::clicked, this, &MainWindow::editSelectedMedicine);
    connect(m_removeButton, &QPushButton::clicked, this, &MainWindow::removeSelectedMedicine);
    connect(m_selectedNamesButton, &QPushButton::clicked, this, &MainWindow::showSelectedMedicines);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::showSettingsDialog);
    connect(m_installTimerButton, &QPushButton::clicked, this, &MainWindow::installUserTimer);
    connect(m_runNowButton, &QPushButton::clicked, this, &MainWindow::triggerManualCheck);
    connect(&m_store, &MedicineStore::dataChanged, this, &MainWindow::refreshTable);
    connect(&m_store, &MedicineStore::lowStockAlert, this, &MainWindow::onLowStockAlert);
    connect(m_dailyTimer, &QTimer::timeout, this, &MainWindow::runDailyUpdate);

    if (!m_store.load()) {
        QMessageBox::warning(this,
                             QStringLiteral("medAlert"),
                             tr("Unable to load the JSON state file. It will be created on first save."));
    }

    refreshTable();
    runDailyUpdate();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }

    QMainWindow::changeEvent(event);
}

void MainWindow::retranslateUi()
{
    m_addButton->setText(tr("Add medication"));
    m_editButton->setText(tr("Edit entry"));
    m_removeButton->setText(tr("Remove"));
    m_selectedNamesButton->setText(tr("Show selected"));
    m_settingsButton->setText(tr("Settings"));
    m_installTimerButton->setText(tr("Enable daily timer"));
    m_runNowButton->setText(tr("Run check now"));
    m_moveUpButton->setToolTip(tr("Move up"));
    m_moveDownButton->setToolTip(tr("Move down"));
    m_table->setHorizontalHeaderLabels({
        tr("Medication"),
        tr("Units/box"),
        tr("Units/day"),
        tr("Current units"),
        tr("Stock boxes"),
        tr("Alert threshold"),
        tr("Standby")
    });
    refreshTable();
}

QString MainWindow::languageDisplayName(const QString &languageCode) const
{
    if (languageCode == QStringLiteral("it")) {
        return tr("Italian");
    }
    if (languageCode == QStringLiteral("fr")) {
        return tr("French");
    }
    if (languageCode == QStringLiteral("de")) {
        return tr("German");
    }
    return tr("English");
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
        auto *standbyItem = new QTableWidgetItem(medicine.standby ? tr("Yes") : tr("No"));

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

    m_statusLabel->setText(tr("JSON file: %1").arg(m_store.storagePath()));
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
                                 tr("Select a medication to move."));
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
        QMessageBox::information(this, QStringLiteral("medAlert"), tr("Select a medication to edit."));
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
    confirmationBox.setWindowTitle(tr("Confirm removal"));
    confirmationBox.setText(tr("Do you really want to remove this medication?"));
    confirmationBox.setInformativeText(
        tr("The medication \"%1\" will be removed from the archive.").arg(medicineName));
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
                                 tr("Selected medications"),
                                 tr("Select one or more medications from the table."));
        return;
    }

    QStringList names;
    names.reserve(rows.size());
    for (const QModelIndex &row : rows) {
        names << m_store.medicines().at(row.row()).name;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Selected medications"));
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
                             tr("Timer activation"),
                             errorMessage);
        return;
    }

    QMessageBox::information(this,
                             tr("Timer activation"),
                             tr("The medalert.timer user timer has been installed and enabled for %1.")
                                 .arg(m_updateTime->toString(QStringLiteral("HH:mm"))));
}

void MainWindow::showSettingsDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Application settings"));
    dialog.setMinimumWidth(360);

    auto *layout = new QVBoxLayout(&dialog);
    auto *formLayout = new QFormLayout();
    auto *languageCombo = new QComboBox(&dialog);
    auto *timeEdit = new QTimeEdit(*m_updateTime, &dialog);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    auto *infoLabel = new QLabel(tr("The application must be restarted to update every visible label after a language change."), &dialog);

    languageCombo->addItem(tr("English"), QStringLiteral("en"));
    languageCombo->addItem(tr("Italian"), QStringLiteral("it"));
    languageCombo->addItem(tr("French"), QStringLiteral("fr"));
    languageCombo->addItem(tr("German"), QStringLiteral("de"));
    languageCombo->setCurrentIndex(languageCombo->findData(currentAppLanguageCode()));

    timeEdit->setDisplayFormat(QStringLiteral("HH:mm"));
    timeEdit->setTime(*m_updateTime);
    infoLabel->setWordWrap(true);

    formLayout->addRow(tr("Language"), languageCombo);
    formLayout->addRow(tr("Update time"), timeEdit);
    layout->addLayout(formLayout);
    layout->addWidget(infoLabel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString languageCode = languageCombo->currentData().toString();
    const QTime selectedTime = timeEdit->time();
    if (!AppSettings::savePreferences(languageCode, selectedTime)) {
        QMessageBox::warning(this,
                             QStringLiteral("medAlert"),
                             tr("Unable to save application settings."));
        return;
    }

    *m_updateTime = selectedTime;
    applyApplicationLanguage(*qApp, languageCode);
    scheduleNextDailyUpdate();
}

void MainWindow::triggerManualCheck()
{
    m_store.processPendingDailyUpdates(false);

    const QString message = m_store.lowStockSummary();
    if (message.isEmpty()) {
        showNotification(tr("Check complete"),
                         tr("Nothing needs to be ordered."),
                         false);
    } else {
        showNotification(tr("Low medication stock"),
                         message,
                         true);
    }

    scheduleNextDailyUpdate();
}

void MainWindow::scheduleNextDailyUpdate()
{
    const QDateTime now = QDateTime::currentDateTime();
    QDateTime nextRun(QDate::currentDate(), *m_updateTime);
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
    showNotification(tr("Low medication stock"), message, true);
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
            *errorMessage = tr("Unable to determine the user configuration directory.");
        }
        return false;
    }

    const QString systemdDir = configRoot + QStringLiteral("/systemd/user");
    if (!QDir().mkpath(systemdDir)) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("Unable to create directory %1.").arg(systemdDir);
        }
        return false;
    }

    QFile serviceFile(systemdDir + QStringLiteral("/medalert.service"));
    if (!serviceFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("Unable to write the user service file.");
        }
        return false;
    }
    QTextStream(&serviceFile) << serviceFileContent();
    serviceFile.close();

    QFile timerFile(systemdDir + QStringLiteral("/medalert.timer"));
    if (!timerFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("Unable to write the user timer file.");
        }
        return false;
    }
    QTextStream(&timerFile) << timerFileContent(*m_updateTime);
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
    dialog.setWindowTitle(tr("Medication entry"));
    dialog.setMinimumWidth(440);

    auto *layout = new QVBoxLayout(&dialog);
    auto *formLayout = new QFormLayout();
    auto *nameEdit = new QLineEdit(medicine.name, &dialog);
    auto *pillsPerBoxSpin = new QSpinBox(&dialog);
    auto *dailyPillsSpin = new QSpinBox(&dialog);
    auto *currentPillsSpin = new QSpinBox(&dialog);
    auto *stockBoxesSpin = new QSpinBox(&dialog);
    auto *alertThresholdSpin = new QSpinBox(&dialog);
    auto *standbyCheck = new QCheckBox(tr("Medication on standby"), &dialog);
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

    formLayout->addRow(tr("Medication name"), nameEdit);
    formLayout->addRow(tr("Units per box"), pillsPerBoxSpin);
    formLayout->addRow(tr("Units taken per day"), dailyPillsSpin);
    formLayout->addRow(tr("Current state (units)"), currentPillsSpin);
    formLayout->addRow(tr("Stock (boxes)"), stockBoxesSpin);
    formLayout->addRow(tr("Notification threshold (units)"), alertThresholdSpin);
    formLayout->addRow(QString(), standbyCheck);

    layout->addLayout(formLayout);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
        if (nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dialog, QStringLiteral("medAlert"), tr("Medication name is required."));
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