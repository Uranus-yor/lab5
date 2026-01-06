#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QTableView>
#include <QSqlTableModel>
#include <QSqlRecord>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include "DatabaseManager.h"
#include "SyncWorker.h"
#include "MarqueeLabel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateTime();
    void onDataSynced(QString cur, QString next, QString notice);

private:
    void setupUi();
    void startSyncThread();

    // UI Controls
    QLabel *lblTime;
    QLabel *lblClassInfo;
    QLabel *lblCurrentCourse;
    QLabel *lblNextCourse;
    MarqueeLabel *marquee;
    QTableView *classroomTable;
    QSqlTableModel *model; // Model/View

    // Threading
    QThread *workerThread;
    SyncWorker *worker;
};
