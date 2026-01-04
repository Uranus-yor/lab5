#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QThread>
#include "DatabaseManager.h" // 确保包含这个头文件
#include <QMutex>

class SyncWorker : public QObject {
    Q_OBJECT
public:
    SyncWorker() {
        m_currentClassId = "1";
    }

    void setTargetClassId(const QString &id) {
        QMutexLocker locker(&m_mutex);
        m_currentClassId = id;
    }

    void doSync() {
        QString targetId;
        {
            QMutexLocker locker(&m_mutex);
            targetId = m_currentClassId;
        }

        QTcpSocket socket;
        socket.connectToHost("127.0.0.1", 12345);

        QString subject = "无数据", teacher = "--", next = "--", notice = "正在连接...";
        bool networkSuccess = false;

        // 1. 尝试网络连接
        if (socket.waitForConnected(2000)) {
            QJsonObject req;
            req["class_id"] = targetId;
            socket.write(QJsonDocument(req).toJson());

            if (socket.waitForReadyRead(3000)) {
                QByteArray data = socket.readAll();
                QJsonDocument doc = QJsonDocument::fromJson(data);

                if (!doc.isNull() && doc.isObject()) {
                    QJsonObject obj = doc.object();

                    // 只要拿到合法的 JSON，就算网络交互成功
                    networkSuccess = true;

                    if (obj["found"].toBool()) {
                        subject = obj["subject"].toString();
                        teacher = obj["teacher"].toString();
                        next = obj["next"].toString();
                        notice = obj["notice"].toString();

                        // =======================================================
                        // 🔴 核心检查点：你之前是不是漏了下面这行？或者是括号没对齐？
                        // =======================================================
                        qDebug() << ">>> 准备调用写入缓存函数，ID:" << targetId;
                        DatabaseManager::instance().updateCache(targetId, subject, teacher, next, notice);

                    } else {
                        subject = "暂无课程";
                        notice = "服务器端未录入该班级数据";
                    }
                }
            }
            socket.disconnectFromHost();
        }

        // 2. 如果网络失败，才读取缓存
        if (!networkSuccess) {
            // 读取时也要传入 targetId
            if (DatabaseManager::instance().readCache(targetId, subject, teacher, next, notice)) {
                notice = "(离线模式) " + notice;
                qDebug() << "连接失败，已加载本地缓存";
            } else {
                subject = "无数据";
                notice = "无法连接服务器且无本地缓存";
            }
        }

        emit dataSynced(
            QString("科目：%1  教师：%2").arg(subject, teacher),
            QString("下节预告：%1").arg(next),
            notice
            );
    }

signals:
    void dataSynced(QString current, QString next, QString notice);

private:
    QString m_currentClassId;
    QMutex m_mutex;
};
