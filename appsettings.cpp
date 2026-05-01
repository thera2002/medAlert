#include "appsettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace AppSettings {

namespace {

QJsonObject loadSettingsObject()
{
    QFile file(settingsFilePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return QJsonObject();
    }

    return document.object();
}

QString ensureDataDirectory()
{
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + "/"
        + QCoreApplication::applicationName();
    QDir().mkpath(directory);
    return directory;
}

} // namespace

QString dataDirectory()
{
    return ensureDataDirectory();
}

QString settingsFilePath()
{
    return dataDirectory() + "/settings.json";
}

QString loadPreferredLanguage()
{
    return loadSettingsObject().value(QStringLiteral("preferredLanguage")).toString();
}

QTime loadUpdateTime()
{
    const QString value = loadSettingsObject().value(QStringLiteral("updateTime")).toString();
    const QTime time = QTime::fromString(value, QStringLiteral("HH:mm"));
    return time.isValid() ? time : QTime(23, 0);
}

bool savePreferences(const QString &languageCode, const QTime &updateTime)
{
    QJsonObject root = loadSettingsObject();
    root.insert(QStringLiteral("preferredLanguage"), languageCode);
    root.insert(QStringLiteral("updateTime"), updateTime.toString(QStringLiteral("HH:mm")));

    QFile file(settingsFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

} // namespace AppSettings