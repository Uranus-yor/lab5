#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    setbuf(stdout, nullptr);
    setbuf(stderr, nullptr);
    QApplication a(argc, argv);

    // 设置高DPI缩放
    a.setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    MainWindow w;
    w.resize(1024, 600);
    w.setWindowTitle("智慧校园班牌系统 v1.0");
    w.show();

    return a.exec();
}
