#pragma once

#include <QTime>
#include <QString>

namespace AppSettings {

QString dataDirectory();
QString settingsFilePath();
QString loadPreferredLanguage();
QTime loadUpdateTime();
bool savePreferences(const QString &languageCode, const QTime &updateTime);

} // namespace AppSettings