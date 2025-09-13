#pragma once

#include <QObject>
#include <QTranslator>
#include <QString>
#include <QLocale>
#include <memory>
#include <vector>

class I18nManager : public QObject {
    Q_OBJECT

public:
    enum Language {
        English,
        Chinese,
        System  // Use system locale
    };

    static I18nManager& instance();

    // Initialize the i18n system
    bool initialize();

    // Load translation for a specific language
    bool loadLanguage(Language lang);
    bool loadLanguage(const QString& languageCode);

    // Get available languages
    QStringList availableLanguages() const;
    
    // Get current language
    Language currentLanguage() const { return m_currentLanguage; }
    QString currentLanguageCode() const;
    QString currentLanguageName() const;

    // Language utilities
    static QString languageToCode(Language lang);
    static Language codeToLanguage(const QString& code);
    static QString languageToName(Language lang);

signals:
    void languageChanged(Language newLanguage);
    void languageChanged(const QString& languageCode);

private:
    I18nManager(QObject* parent = nullptr);
    ~I18nManager();
    I18nManager(const I18nManager&) = delete;
    I18nManager& operator=(const I18nManager&) = delete;

    bool loadTranslation(const QString& languageCode);
    void removeTranslators();
    QString getSystemLanguageCode() const;

private:
    Language m_currentLanguage;
    std::vector<std::unique_ptr<QTranslator>> m_translators;
    QString m_translationPath;
    bool m_initialized;
};
