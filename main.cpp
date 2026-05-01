#include "apptranslator.h"
#include "appsettings.h"
#include "mainwindow.h"

#include "medicinestore.h"
#include "notificationservice.h"

#include <QCoreApplication>
#include <QApplication>
#include <QtGlobal>

namespace {

QtMessageHandler defaultMessageHandler = nullptr;

void medAlertMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (context.category != nullptr
        && QString::fromUtf8(context.category) == QStringLiteral("qt.qpa.wayland.textinput")
        && message.contains(QStringLiteral("QWaylandTextInputv3::zwp_text_input_v3_leave"))) {
        return;
    }

    if (defaultMessageHandler != nullptr) {
        defaultMessageHandler(type, context, message);
    }
}

} // namespace

int runHeadlessCheck(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("medAlert"));
    QCoreApplication::setOrganizationName(QStringLiteral("medAlert"));
    applyApplicationLanguage(app, AppSettings::loadPreferredLanguage());

    MedicineStore store;
    if (!store.load()) {
        qCritical("%s",
                  qPrintable(QCoreApplication::translate("main", "Unable to load medicines JSON state.")));
        return 1;
    }

    store.processPendingDailyUpdates(false);

    const QString message = store.lowStockSummary();
    if (message.isEmpty()) {
        return 0;
    }

    NotificationService notifications;
    if (!notifications.sendLowStockNotification(message)) {
        qCritical("%s",
                  qPrintable(QCoreApplication::translate("main", "Unable to deliver low-stock notification.")));
        return 2;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    defaultMessageHandler = qInstallMessageHandler(medAlertMessageHandler);

    for (int index = 1; index < argc; ++index) {
        if (QString::fromLocal8Bit(argv[index]) == QStringLiteral("--check")) {
            return runHeadlessCheck(argc, argv);
        }
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("medAlert"));
    QApplication::setOrganizationName(QStringLiteral("medAlert"));
    applyApplicationLanguage(app, AppSettings::loadPreferredLanguage());

    MainWindow window;
    window.show();

    return app.exec();
}