/*
 * WorkerThread.cpp
 *
 *  Created on: 2016-07-08
 *      Author: Roger
 */

#include "WorkerThread.h"

#include <bb/platform/Notification>
#include <QFileInfo>
#include <QDebug>
#include <QTimer>

using namespace bb::platform;

WorkerThread::WorkerThread(const QString& filePath, const bool& isVideo, QObject* parent) :
    QThread(parent),
    m_fullscreenPid(NULL),
    m_filePath(filePath),
    m_maxAttempts(isVideo ? 1 : 1200)
{
    connect(this, SIGNAL(renameDone()), this, SLOT(onRenameDone()));
    if (isVideo) {
        m_fullscreenPid = new FullscreenPid(this);
        connect(m_fullscreenPid, SIGNAL(fullscreenPidChanged(int&)), this, SLOT(onFullscreenPidChanged(int&)));
    }

    QTimer::singleShot(10 * 60000, this, SLOT(deleteLater())); // Lives a maximum of 10 minutes
}

void WorkerThread::onFullscreenPidChanged(int& pid) {
    Q_UNUSED(pid);
    this->start();
}

void WorkerThread::onRenameDone() {
    if (m_fullscreenPid != NULL) {
        disconnect(m_fullscreenPid, SIGNAL(fullscreenPidChanged(int&)), this, SLOT(onFullscreenPidChanged(int&)));
        delete m_fullscreenPid;
        m_fullscreenPid = NULL;

//        this->showNotification(NOTIFICATION_RESTART_WHATSAPP);
    }
    else {
//        this->showNotification(NOTIFICATION_CLICK_RETRY);
    }
}

void WorkerThread::run() {
    if (m_filePath.contains(".opus")) { usleep(2000 * 1000); } // 2 secs

    int attempts = -1;
    QString extension = m_filePath.mid(m_filePath.lastIndexOf("."));
    QString newFile = m_filePath;
    newFile.remove(extension);
    QString toTrash = newFile + "_WAA_TRASH" + extension;
    newFile += "_WAA_COPY" + extension;
    qDebug() << "newFile:" << newFile;
    bool worked = QFile::exists(newFile);
    while (!worked && (++attempts < m_maxAttempts)) {
        worked = QFile::copy(m_filePath, newFile);
        qDebug() << "attempts:" << attempts;
        if (!worked) { usleep(500 * 1000); } // 500ms
    }
    if (!worked) { return; }
    qDebug() << "Copied...";

    worked = QFile::remove(m_filePath);
    if (!worked) { return; }
    qDebug() << "Removed...";

    /*
    worked = QFile::rename(m_filePath, toTrash);
    if (!worked) { return; }
    qDebug() << "Renamed to trash...";
    */

    worked = QFile::rename(newFile, m_filePath);
    if (!worked) { return; }
    qDebug() << "Replaced...";

//    worked = QFile::remove(toTrash);
//    if (!worked) { return; }
//    qDebug() << "Removed...";

    emit renameDone();
}

void WorkerThread::showNotification(NotificationType type) {
    Notification* notification = new Notification();
    QString iconPath;

    switch (type) {
        case NOTIFICATION_CLICK_RETRY: {
            iconPath = QString::fromUtf8("file://%1/app/native/assets/Images/retry.png").arg(QDir::currentPath());
            notification->setTitle(tr("Click Retry in WhatsApp"));
            notification->setBody(tr("In WhatsApp for Android, click on the Retry icon (↥)."));
            QTimer::singleShot(3000, notification, SLOT(deleteFromInbox()));
            break;
        }
        case NOTIFICATION_RESTART_WHATSAPP: {
            iconPath = QString::fromUtf8("file://%1/app/native/assets/Images/restart.png").arg(QDir::currentPath());
            notification->setTitle(tr("Restart WhatsApp and click on Retry"));
            notification->setBody(tr("Close WhatsApp, restart it and click on the Retry icon (↥)."));
            QTimer::singleShot(60000, notification, SLOT(deleteFromInbox()));
            break;
        }
        default: break;
    }

    notification->setIconUrl(iconPath);
    notification->notify();

    this->deleteLater();
}
