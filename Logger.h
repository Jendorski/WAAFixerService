/*
 * Logger.h
 *
 *  Created on: 2015-06-08
 *      Author: Roger
 */

#ifndef LOGGER_H_
#define LOGGER_H_

/*
 * #include <src/Logger/Logger.h>
 *
 * Call it using :
 * Logger::logThis("My message to log");
 */

#include <src/Logger/HeapUsage/HeapUsage.h>

#include <bb/Application>
#include <bb/data/JsonDataAccess>
#include <QDebug>
#include <QStringList>
#include <QTime>
#include <QMutex>
#include <QMutexLocker>

#define LOG_FILE "data/log.txt"
#define MAXIMUM_LOG_SIZE 100

class Logger
{
public:
    static void logThis(const QString& message) {
        qDebug() << HeapUsage::measureMem() << message;

        QMutex mutex;
        QMutexLocker locker(&mutex);

        bb::data::JsonDataAccess jda;
        QStringList log = jda.load(LOG_FILE).toMap()["log"].toStringList();
        log.prepend(QTime::currentTime().toString("hh:mm:ss") + " " + QString::number(HeapUsage::measureMem()) + " " + message);

        while (log.size() > MAXIMUM_LOG_SIZE) {
            log.removeLast();
        }

        QVariantMap logMap;
        logMap.insert("applicationVersion", bb::Application::instance()->applicationVersion());
        logMap.insert("Time", QDateTime::currentDateTime());
        logMap.insert("log", log);
        jda.save(logMap, LOG_FILE);
    }
};

#endif /* LOGGER_H_ */
