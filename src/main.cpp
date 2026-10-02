// LocalPDF Application Main Entry Point
// Author: Antigravity Assistant

#include <QApplication>
#include <QIcon>
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/app.ico"));

    MainWindow window;
    window.setWindowTitle("LocalPDF");
    window.resize(1280, 800);
    window.show();

    return app.exec();
}
