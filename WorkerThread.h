/*
 * WorkerThread.h
 *
 *  Created on: 2016-07-08
 *      Author: Roger
 */

#ifndef WORKERTHREAD_H_
#define WORKERTHREAD_H_

#include <src/FullscreenPid/FullscreenPid.h>

#include <QObject>
#include <QThread>

class WorkerThread : public QThread
{
    Q_OBJECT

    enum NotificationType {
        NOTIFICATION_CLICK_RETRY,
        NOTIFICATION_RESTART_WHATSAPP
    };


public:
    WorkerThread(const QString& filePath, const bool& isVideo = false, QObject* parent = NULL);

    void run(); // Only function that will run in separate thread

private slots:
    void onFullscreenPidChanged(int&);
    void onRenameDone();

private:
    void showNotification(NotificationType type);

    FullscreenPid* m_fullscreenPid;

    QString m_filePath;
    int m_maxAttempts;

signals:
    void renameDone();
};

#endif /* WORKERTHREAD_H_ */
