#include "mainwindow.h"
#include <QGroupBox>
#include <QTimer>
#include <QSqlRecord>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    bool success = DatabaseManager::instance().init();
    if (success) {
        qDebug() << "数据库初始化成功！";
    } else {
        qDebug() << "数据库初始化失败！！！";
    }
    setupUi();
    startSyncThread();

    // UI 定时刷新时间
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateTime);
    timer->start(1000);
    updateTime();
}

MainWindow::~MainWindow() {
    workerThread->quit();
    workerThread->wait();
}

void MainWindow::setupUi() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    centralWidget->setStyleSheet("background-color: #2b2b2b; color: white;");

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 1. 顶部：班级信息 + 时间
    QHBoxLayout *topLayout = new QHBoxLayout();
    lblClassInfo = new QLabel("三年二班 (302教室)");
    lblClassInfo->setStyleSheet("font-size: 24px; font-weight: bold; color: #4CAF50;");

    lblTime = new QLabel();
    lblTime->setStyleSheet("font-size: 20px; color: #ddd;");

    topLayout->addWidget(lblClassInfo);
    topLayout->addStretch();
    topLayout->addWidget(lblTime);

    // 2. 中部：课程显示 (左侧) + 班级列表 (右侧 Model/View)
    QHBoxLayout *centerLayout = new QHBoxLayout();

    // 左侧：课程展示卡片
    QGroupBox *courseGroup = new QGroupBox("今日课程");
    courseGroup->setStyleSheet("QGroupBox { border: 1px solid gray; border-radius: 5px; margin-top: 10px; font-size: 14px; }"
                               "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; }");
    QVBoxLayout *courseLayout = new QVBoxLayout(courseGroup);

    lblCurrentCourse = new QLabel("正在加载...");
    lblCurrentCourse->setStyleSheet("font-size: 36px; color: #2196F3; font-weight: bold; qproperty-alignment: AlignCenter;");

    lblNextCourse = new QLabel("下节预告: --");
    lblNextCourse->setStyleSheet("font-size: 18px; color: #aaa; qproperty-alignment: AlignCenter;");

    courseLayout->addStretch();
    courseLayout->addWidget(lblCurrentCourse);
    courseLayout->addWidget(lblNextCourse);
    courseLayout->addStretch();

    // 右侧：班级选择列表 (Model/View)
    QGroupBox *listGroup = new QGroupBox("切换班级 (Model/View)");
    listGroup->setStyleSheet("QGroupBox { font-size: 14px; }");
    QVBoxLayout *listLayout = new QVBoxLayout(listGroup);

    classroomTable = new QTableView();
    model = new QSqlTableModel(this, DatabaseManager::instance().db());
    model->setTable("classrooms");
    model->select(); // 查询数据
    model->setHeaderData(1, Qt::Horizontal, "班级名称");
    model->setHeaderData(2, Qt::Horizontal, "年级");

    classroomTable->setModel(model);
    classroomTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    classroomTable->setColumnHidden(0, true); // 隐藏 ID 列
    classroomTable->horizontalHeader()->setStretchLastSection(true);
    classroomTable->setStyleSheet("QTableView { background-color: #333; gridline-color: #555; }"
                                  "QHeaderView::section { background-color: #444; }");

    listLayout->addWidget(classroomTable);

    // 连接选择变化
    connect(classroomTable->selectionModel(), &QItemSelectionModel::currentRowChanged,
            [this](const QModelIndex &current, const QModelIndex &){
                if(current.isValid()) {
                    // 1. 获取 Model 中的数据
                    // 假设数据库 ID 列是第 0 列 (虽然被隐藏了，但数据还在)
                    QString id = model->record(current.row()).value("id").toString();
                    QString name = model->record(current.row()).value("name").toString();

                    // 2. 更新界面标题
                    lblClassInfo->setText(name + QString(" (ID: %1)").arg(id));

                    // 3. === 核心：告诉后台线程下次同步这个 ID ===
                    worker->setTargetClassId(id);

                    // 4. (可选) 立即清空当前显示，给用户正在加载的反馈
                    lblCurrentCourse->setText("加载中...");

                    // 5. (可选) 也可以直接手动触发一次 sync，不用等5秒
                    QMetaObject::invokeMethod(worker, "doSync");
                }
            });

    centerLayout->addWidget(courseGroup, 2);
    centerLayout->addWidget(listGroup, 1);

    // 3. 底部：滚动通知
    marquee = new MarqueeLabel();
    marquee->setFixedHeight(40);
    marquee->setStyleSheet("background-color: #000; border-top: 2px solid #FF5722;");
    marquee->setText("欢迎使用智慧班牌系统，系统初始化完成...");

    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(centerLayout);
    mainLayout->addWidget(marquee);
}

void MainWindow::startSyncThread() {
    // 设置多线程
    workerThread = new QThread(this);
    worker = new SyncWorker();
    worker->moveToThread(workerThread);

    // 定时器触发同步（为了演示，这里用 Timer 触发 worker 的任务）
    QTimer *syncTimer = new QTimer(this);
    connect(syncTimer, &QTimer::timeout, worker, &SyncWorker::doSync);
    connect(workerThread, &QThread::started, syncTimer, [syncTimer](){ syncTimer->start(5000); }); // 5秒同步一次

    // 接收数据更新 UI
    connect(worker, &SyncWorker::dataSynced, this, &MainWindow::onDataSynced);

    workerThread->start();
}

void MainWindow::updateTime() {
    lblTime->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss dddd"));
}

void MainWindow::onDataSynced(QString cur, QString next, QString notice) {
    lblCurrentCourse->setText(cur);
    lblNextCourse->setText(next);
    // 只有通知改变时才重置
    static QString lastNotice = "";
    if(lastNotice != notice) {
        marquee->setText(notice);
        lastNotice = notice;
    }
}
