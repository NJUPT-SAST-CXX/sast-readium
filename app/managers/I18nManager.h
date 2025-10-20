#pragma once

#include <QObject>
#include <QString>
#include <memory>

// Forward declaration
class I18nManagerImpl;

/**
 * Internationalization Manager
 * Manages application translations and language switching
 * Note: Protected destructor is intentional for singleton pattern with QObject
 * base
 */
// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
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

protected:
    // Protected destructor for singleton pattern - prevents deletion through
    // base class pointer while allowing the static instance to be destroyed.
    // Must override QObject's virtual destructor.
    ~I18nManager() override;

private:
    explicit I18nManager(QObject* parent = nullptr);

    std::unique_ptr<I18nManagerImpl> m_pImpl;
};
