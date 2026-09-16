#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    // Dynamically inherits styles, colors, and layout engines directly from the active DE
    QApplication app(argc, argv);
    
    MainWindow window;
    window.show();
    return app.exec();
}
