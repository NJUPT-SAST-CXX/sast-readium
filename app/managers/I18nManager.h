#pragma once

#include <QObject>
#include <QString>
#include <memory>

// Forward declaration
class I18nManagerImpl;

class I18nManager : public QObject {
    Q_OBJECT

public:
    enum class Language : std::uint8_t {
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
    [[nodiscard]] QStringList availableLanguages() const;

    // Get current language
    [[nodiscard]] Language currentLanguage() const;
    [[nodiscard]] QString currentLanguageCode() const;
    [[nodiscard]] QString currentLanguageName() const;

    // Language utilities
    static QString languageToCode(Language lang);
    static Language codeToLanguage(const QString& code);
    static QString languageToName(Language lang);

signals:
    void languageChanged(Language newLanguage);
    void languageChanged(const QString& languageCode);

public:
    // Deleted copy/move operations (public for better error messages)
    I18nManager(const I18nManager&) = delete;
    I18nManager& operator=(const I18nManager&) = delete;
    I18nManager(I18nManager&&) = delete;
    I18nManager& operator=(I18nManager&&) = delete;

private:
    explicit I18nManager(QObject* parent = nullptr);
    ~I18nManager() override;

    std::unique_ptr<I18nManagerImpl> pImpl;
};
