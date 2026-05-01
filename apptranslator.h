#pragma once

#include <QCoreApplication>
#include <QLocale>
#include <QStringList>
#include <QTranslator>

class AppTranslator : public QTranslator {
    Q_OBJECT

public:
    explicit AppTranslator(const QString &languageCode, QObject *parent = nullptr);

    bool isEmpty() const override;
    QString translate(const char *context,
                      const char *sourceText,
                      const char *disambiguation = nullptr,
                      int n = -1) const override;

    static QString normalizedLanguageCode(const QString &languageCode);
    static QString languageFromLocale(const QLocale &locale);
    static QStringList supportedLanguageCodes();

private:
    QString m_languageCode;
};

QString effectiveLanguageCode(const QString &savedLanguageCode,
                              const QLocale &fallbackLocale = QLocale::system());
QString currentAppLanguageCode();
void applyApplicationLanguage(QCoreApplication &app, const QString &languageCode);