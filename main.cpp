#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("medAlert"));
    QApplication::setOrganizationName(QStringLiteral("medAlert"));

    MainWindow window;
    window.show();

    return app.exec();
}