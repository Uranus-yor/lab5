#pragma once
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QCoreApplication>
#include <QThread> // 引入线程头文件
#include <QUuid>   // 用于生成唯一连接名

class DatabaseManager {
public:
    static DatabaseManager& instance() {
        static DatabaseManager _instance;
        return _instance;
    }

    bool init() {
        // 主线程初始化默认连接 (给 Model/View 使用)
        m_dbPath = QCoreApplication::applicationDirPath() + "/client_cache_v2.db";

        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE"); // 默认连接
        db.setDatabaseName(m_dbPath);

        qDebug() << "客户端数据库路径：" << m_dbPath;

        if (!db.open()) {
            qDebug() << "打开数据库失败:" << db.lastError();
            return false;
        }
        createTables();
        return true;
    }

    void createTables() {
        QSqlQuery query; // 使用默认连接
        query.exec("CREATE TABLE IF NOT EXISTS classrooms (id INTEGER PRIMARY KEY, name TEXT, grade TEXT)");
        query.exec("INSERT OR IGNORE INTO classrooms (id, name, grade) VALUES (1, '三年二班', '三年级')");
        query.exec("INSERT OR IGNORE INTO classrooms (id, name, grade) VALUES (2, '一年一班', '一年级')");
        query.exec("INSERT OR IGNORE INTO classrooms (id, name, grade) VALUES (3, '五年三班', '五年级')");

        query.exec("CREATE TABLE IF NOT EXISTS course_cache ("
                   "class_id TEXT PRIMARY KEY, "
                   "subject TEXT, teacher TEXT, next TEXT, notice TEXT)");
    }

    // === 获取当前线程可用的数据库连接 ===
    // 这是解决多线程问题的核心函数
    QSqlDatabase getDatabase() {
        QString connectionName;
        // 如果是主线程，使用默认连接
        if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
            return QSqlDatabase::database();
        }

        // 如果是子线程，创建一个唯一的连接名
        // 使用内存地址作为唯一标识，防止冲突
        connectionName = QString("WorkerConn_%1").arg((quint64)QThread::currentThreadId());

        if (!QSqlDatabase::contains(connectionName)) {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
            db.setDatabaseName(m_dbPath);
            if (!db.open()) {
                qDebug() << "子线程数据库打开失败:" << db.lastError();
            }
        }
        return QSqlDatabase::database(connectionName);
    }

    // === 写入缓存 (线程安全版) ===
    void updateCache(const QString& classId, const QString& subject, const QString& teacher, const QString& next, const QString& notice) {
        // 1. 获取属于当前线程的数据库连接
        QSqlDatabase db = getDatabase();

        // 2. 将连接传递给 QSqlQuery 构造函数
        QSqlQuery query(db);

        query.prepare("INSERT OR REPLACE INTO course_cache (class_id, subject, teacher, next, notice) "
                      "VALUES (:id, :sub, :tea, :nxt, :not)");
        query.bindValue(":id", classId);
        query.bindValue(":sub", subject);
        query.bindValue(":tea", teacher);
        query.bindValue(":nxt", next);
        query.bindValue(":not", notice);

        if(!query.exec()) {
            qDebug() << "❌ 写入缓存失败! ClassID:" << classId << " 错误:" << query.lastError().text();
        } else {
            qDebug() << "✅ 写入缓存成功! ClassID:" << classId;
        }
    }

    // === 读取缓存 (线程安全版) ===
    bool readCache(const QString& classId, QString &subject, QString &teacher, QString &next, QString &notice) {
        // 1. 获取属于当前线程的数据库连接
        QSqlDatabase db = getDatabase();

        // 2. 将连接传递给 QSqlQuery 构造函数
        QSqlQuery query(db);

        query.prepare("SELECT * FROM course_cache WHERE class_id = :id");
        query.bindValue(":id", classId);

        if (query.exec()) {
            if (query.next()) {
                subject = query.value("subject").toString();
                teacher = query.value("teacher").toString();
                next = query.value("next").toString();
                notice = query.value("notice").toString();
                qDebug() << "✅ 读取缓存成功! ClassID:" << classId;
                return true;
            } else {
                qDebug() << "⚠️ 缓存中找不到 ClassID:" << classId;
                return false;
            }
        } else {
            qDebug() << "❌ 查询缓存语句错误:" << query.lastError().text();
            return false;
        }
    }

    // 给 Model 使用的接口，返回默认连接即可
    QSqlDatabase db() const { return QSqlDatabase::database(); }

private:
    DatabaseManager() {}
    QString m_dbPath;
};
