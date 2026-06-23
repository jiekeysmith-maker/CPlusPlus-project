#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("JiekeySmith"));
    QCoreApplication::setApplicationName(QStringLiteral("CPlusPlusProject"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));

    MainWindow w;
    w.show();
    return a.exec();
}
