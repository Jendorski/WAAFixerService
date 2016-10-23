/*
 * Copyright (c) 2013-2015 BlackBerry Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "service.hpp"
#include <src/Logger/Logger.h>
#include <src/WorkerThread/WorkerThread.h>

#include <bb/Application>
#include <bb/platform/Notification>
#include <bb/platform/NotificationDefaultApplicationSettings>
#include <bb/system/InvokeManager>

#include <QFileInfo>

using namespace bb::platform;
using namespace bb::system;

Service::Service() :
        QObject(),
        m_fileSystemWatcher(new QFileSystemWatcher(this)),
        m_invokeManager(new InvokeManager(this)),
        m_notify(new Notification(this))
{
    // Add this one line of code to all your projects, it will save you a ton of problems
    // when dealing with foreign languages. No more ??? characters.
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

    QDir dir, dir2;
    QString waaMediaPath = QString(getenv("PERIMETER_HOME")) + "/shared/misc/android/WhatsApp/Media/";
    QStringList folderPartialNames = QStringList() << "Images" << "Audio" << "Documents" << "Video";

    foreach(QString partialName, folderPartialNames) {
        QString path = waaMediaPath + QString("WhatsApp %1/Sent").arg(partialName);
        dir2.setPath(path);
        if (!dir2.exists()) {
            Logger::logThis("Creating " + path + ": " + QString(dir.mkpath(path) ? "true" : "false"));
        }
        m_fileSystemWatcher->addPath(path);
    }

    QString path = waaMediaPath + QString("WhatsApp Audio");
    m_fileSystemWatcher->addPath(path);

    connect(m_fileSystemWatcher, SIGNAL(directoryChanged(const QString&)), this, SLOT(onDirectoryChanged(const QString&)));

    m_invokeManager->connect(m_invokeManager, SIGNAL(invoked(const bb::system::InvokeRequest&)),
            this, SLOT(handleInvoke(const bb::system::InvokeRequest&)));

    NotificationDefaultApplicationSettings settings;
    settings.setPreview(NotificationPriorityPolicy::Allow);
    settings.apply();
}

void Service::handleInvoke(const bb::system::InvokeRequest & request)
{
    QString action = request.action().split(".").last();
    Logger::logThis("Service::handleInvoke(): " + action);
    if (action == "SHUTDOWN") {
        bb::Application::instance()->quit();
    }
}

void Service::onDirectoryChanged(const QString& path) {
    Logger::logThis("Service::onDirectoryChanged(): " + path);
    if (!path.contains("/Sent") && !path.contains("/WhatsApp Audio")) { return; }

    QDir dir(path);
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files, QDir::Time);
    if (fileInfoList.isEmpty()) { return; }

    QFileInfo fileInfo = fileInfoList.first();
    QString absoluteFilePath = fileInfo.absoluteFilePath();
    if (absoluteFilePath.contains("_WAA_COPY")) { return; }
    if (absoluteFilePath.contains("qt_temp")) { return; }

    if (!m_listOfFilesCatched.contains(absoluteFilePath)) {
        Logger::logThis("Rename this file: " + absoluteFilePath);
        m_listOfFilesCatched.append(absoluteFilePath);
        while (m_listOfFilesCatched.size() > 20) { m_listOfFilesCatched.removeFirst(); }

//        bool isVideo = absoluteFilePath.contains("/WhatsApp Video/Sent");
//        WorkerThread *workerThread = new WorkerThread(absoluteFilePath, isVideo, this);
        WorkerThread *workerThread = new WorkerThread(absoluteFilePath, false, this);
        workerThread->start();
    }
    else {
        Logger::logThis("File already renamed in the past: " + absoluteFilePath);
        return;
    }
}
