#include <QApplication>
#include <QStringList>
#include <cstdio>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow window;

    /* Headless check: print the tree the window built from the inventory. */
    if (app.arguments().contains("--dump-tree")) {
        const QStringList rows = window.treeDump();

        for (const QString &row : rows)
            std::printf("%s\n", qPrintable(row));
        return 0;
    }

    window.show();
    return app.exec();
}
