#pragma once

#include <QObject>
#include <QString>
#include <memory>

// Forward declaration
class I18nManagerImpl;

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
    Language currentLanguage() const;
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

    std::unique_ptr<I18nManagerImpl> pImpl;
};
