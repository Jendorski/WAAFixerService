/*
 * Settings.cpp
 *
 *  Created on: 2014-09-06
 *      Author: Roger
 */

/*
 *
 * Don't forget to add this to your PRO file :
 * LIBS += -lbbdata
 *
 *
 * This is a custom object that can be used as a basic alternative to QSettings.
 * It stores everything in a JSON file. This can be helful for performance as
 * JSON is faster than QSettings in most case, and even in Ekke's tests, JSON was
 * the best choice between JSON, XML and SQL.
 *
 * To add this object to your project, follow these steps :
 * 1- Copy both Settings.cpp and Settings.h to your src folder
 * 2- In your applicationui.hpp, add those lines :
 *      #include "Settings.h"
 *
 *      private:
 *          Settings *settings;
 *
 * 3- In your application.cpp, add those lines :
 *      (in constructor)
 *      settings = new Settings(this);
 *          ...
 *      QmlDocument *qml = QmlDocument::create("asset:///main.qml").parent(this);
 *      qml->setContextProperty("_settings", settings);
 *
 *
 */

#include <src/Settings/Settings.h>
#include <QDebug>
#include <QFile>

Settings::Settings(QObject *_parent) :
    QObject(_parent),
    isInUiThread(false)
{
    // Load the settings file on startup.
    sync();

    // If the settings file doesn't exists, create it
    QFile jsonFile(SETTINGS_FILE);
    if (!jsonFile.exists())
        save();

    jsonFile.deleteLater();

    m_server = new QUdpSocket(this);
    m_socket = new QUdpSocket(this);

    this->listenOnPort(isInUiThread ? SETTINGS_UI_LISTENING_PORT : SETTINGS_HEADLESS_LISTENING_PORT);

    // Watcher for changes in the settings file.
//    watcher = new QFileSystemWatcher(this);

    // Listen for any change to the settings file, change to this file can occur if
    // another instance is modifying the file (ie: headless or UI).
//    watcher->addPath(SETTINGS_FILE);
//    connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(settingsChanged(QString)));
}

Settings::~Settings()
{
    // Save the settings file on exit.
    save();
}

QStringList Settings::allKeys()
{
    // Returns a list containing all the keys in the map in ascending order.
    return settings.keys();
}

void Settings::clear()
{
    // Removes all items from the settings file.
    settings.clear();
    save();
}

bool Settings::contains(const QString &key)
{
    // Returns true if the settings file contains an item with key key;
    // otherwise returns false.
    return settings.contains(key);
}

QString Settings::fileName()
{
    return SETTINGS_FILE;
}

void Settings::listenOnPort(int _port)
{
    qDebug() << "Settings::listenOnPort()" << _port;
    m_server->bind(QHostAddress::Any, _port);
    bool ok = connect(m_server, SIGNAL(readyRead()), this, SLOT(onReadyRead()));

    ok==true ? qDebug() << "binding ok" : qDebug() << "binding failed";
}

void Settings::onReadyRead()
{
  qDebug() << "Settings::onReadyRead()";
    while (m_server->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(m_server->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        m_server->readDatagram(datagram.data(), datagram.size(),&sender, &senderPort);
        QString data = QString(datagram);

//        qDebug() << data;
//        emit receivedData(data);
        if (data == SETTINGS_FILE_UPDATED) {
            qDebug() << (isInUiThread ? "UI" : "HL") << "-- Settings changed...";
            sync();
        }
    }
}

int Settings::remove(const QString &key)
{
    // Removes all the items that have the key key from the settings file.
    // Returns the number of items removed which is usually 1 but will be 0
    // if the key isn't in the settings file.

    int numberRemoved = settings.remove(key);
    save();
    return numberRemoved;
}

void Settings::save()
{
    QMutexLocker locker(&mutex);

    jda.save(QVariant(settings), SETTINGS_FILE);

    locker.unlock();

    emit allSettingsChanged(settings);
    this->sendMessage(SETTINGS_FILE_UPDATED);
}

void Settings::sendMessage(QString _data)
{
//  qDebug() << "Settings::sendMessage()" << _data;
    m_socket->writeDatagram(_data.toStdString().c_str(),QHostAddress("127.0.0.1"), isInUiThread ? SETTINGS_HEADLESS_LISTENING_PORT : SETTINGS_UI_LISTENING_PORT);
}

/*
void Settings::settingsChanged(QString path) {
    qDebug() << "UI -- Settings::settingsChanged()";
    Q_UNUSED(path);

    sync();
}
*/

void Settings::setValue(const QString &key, const QVariant &value)
{
    // Inserts a new item with the key key and a value of value.
    // If there is already an item with the key key, that item's value is
    // replaced with value.

    qDebug() << "UI - Settings::setValue ->" << key << ":" << value;

    settings.remove(key);
    settings.insert(key, value);
    save();
}

void Settings::sync()
{
    QMutexLocker locker(&mutex);

    // Reloads any settings that have been changed by another application.
    // This function is called automatically by the event loop at regular intervals,
    // so you normally don't need to call it yourself.
    settings = jda.load(SETTINGS_FILE).toMap();

    locker.unlock();

    emit allSettingsChanged(settings);
}

QVariant Settings::value(const QString &key, const QVariant &defaultValue)
{
    // Returns the value associated with the key key.
    // If the settings file contains no item with key key,
    // the function returns defaultValue.

    // Return on empty key.
    if (key.trimmed().isEmpty())
        return QVariant();

    if (settings.contains(key)) {
//        qDebug() << "UI - Settings::value ->" << key << ":" << settings.value(key);
        return settings.value(key, defaultValue);
    }
    else
        return defaultValue;
}
