#include "I18nManager.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include "../logging/LoggingMacros.h"

I18nManager::I18nManager(QObject* parent)
    : QObject(parent)
    , m_currentLanguage(System)
    , m_initialized(false)
{
    // Set translation path relative to application directory
    m_translationPath = QApplication::applicationDirPath();
}

I18nManager::~I18nManager()
{
    removeTranslators();
}

I18nManager& I18nManager::instance()
{
    static I18nManager instance;
    return instance;
}

bool I18nManager::initialize()
{
    if (m_initialized) {
        LOG_WARNING("I18nManager: Already initialized");
        return true;
    }

    LOG_INFO("I18nManager: Initializing translation system");

    // Load system language by default
    QString systemLang = getSystemLanguageCode();
    LOG_INFO("I18nManager: System language detected: {}", systemLang.toStdString());

    // Try to load system language, fall back to English if failed
    if (!loadLanguage(systemLang)) {
        LOG_WARNING("I18nManager: Failed to load system language, falling back to English");
        if (!loadLanguage("en")) {
            LOG_ERROR("I18nManager: Failed to load English translation");
            return false;
        }
    }

    m_initialized = true;
    LOG_INFO("I18nManager: Initialization completed successfully");
    return true;
}

bool I18nManager::loadLanguage(Language lang)
{
    return loadLanguage(languageToCode(lang));
}

bool I18nManager::loadLanguage(const QString& languageCode)
{
    LOG_INFO("I18nManager: Loading language: {}", languageCode.toStdString());

    // Remove existing translators
    removeTranslators();

    // Handle system language
    QString actualCode = languageCode;
    if (languageCode == "system") {
        actualCode = getSystemLanguageCode();
    }

    // Load translation
    if (!loadTranslation(actualCode)) {
        LOG_ERROR("I18nManager: Failed to load translation for: {}", actualCode.toStdString());
        return false;
    }

    // Update current language
    m_currentLanguage = codeToLanguage(actualCode);

    // Emit signal to notify UI components
    emit languageChanged(m_currentLanguage);
    emit languageChanged(actualCode);

    LOG_INFO("I18nManager: Language loaded successfully: {}", actualCode.toStdString());
    return true;
}

bool I18nManager::loadTranslation(const QString& languageCode)
{
    // Skip loading for English (base language)
    if (languageCode == "en" || languageCode.startsWith("en_")) {
        LOG_DEBUG("I18nManager: Using base English language");
        return true;
    }

    // Create translator for Qt system translations
    auto qtTranslator = std::make_unique<QTranslator>();
    QString qtTransPath = QDir(m_translationPath).filePath(QString("qt_%1").arg(languageCode));
    
    if (qtTranslator->load(qtTransPath)) {
        LOG_DEBUG("I18nManager: Loaded Qt translation: {}", qtTransPath.toStdString());
        qApp->installTranslator(qtTranslator.get());
        m_translators.push_back(std::move(qtTranslator));
    }

    // Create translator for application translations
    auto appTranslator = std::make_unique<QTranslator>();
    
    // Try multiple possible paths
    QStringList searchPaths = {
        m_translationPath,
        QDir(m_translationPath).filePath("i18n"),
        QDir(m_translationPath).filePath("translations"),
        QDir(QApplication::applicationDirPath()).filePath("../../app/i18n"),
        QDir(QApplication::applicationDirPath()).filePath("../app/i18n"),
        ":/i18n"  // Resource path
    };

    bool loaded = false;
    for (const QString& path : searchPaths) {
        QString transFile = QDir(path).filePath(QString("app_%1").arg(languageCode));
        
        // Try with .qm extension
        if (QFile::exists(transFile + ".qm")) {
            transFile += ".qm";
        }
        
        if (appTranslator->load(transFile)) {
            LOG_INFO("I18nManager: Loaded application translation from: {}", transFile.toStdString());
            qApp->installTranslator(appTranslator.get());
            m_translators.push_back(std::move(appTranslator));
            loaded = true;
            break;
        }
    }

    if (!loaded) {
        LOG_WARNING("I18nManager: Could not find translation file for: {}", languageCode.toStdString());
        return false;
    }

    return true;
}

void I18nManager::removeTranslators()
{
    for (auto& translator : m_translators) {
        if (translator) {
            qApp->removeTranslator(translator.get());
        }
    }
    m_translators.clear();
}

QString I18nManager::getSystemLanguageCode() const
{
    QLocale systemLocale = QLocale::system();
    QString langCode = systemLocale.name(); // Returns format like "zh_CN" or "en_US"
    
    // Simplify to just language code for our purposes
    if (langCode.startsWith("zh")) {
        return "zh";
    } else if (langCode.startsWith("en")) {
        return "en";
    }
    
    // Default to English for unsupported languages
    return "en";
}

QStringList I18nManager::availableLanguages() const
{
    return QStringList() << "en" << "zh";
}

QString I18nManager::currentLanguageCode() const
{
    return languageToCode(m_currentLanguage);
}

QString I18nManager::currentLanguageName() const
{
    return languageToName(m_currentLanguage);
}

QString I18nManager::languageToCode(Language lang)
{
    switch (lang) {
        case English:
            return "en";
        case Chinese:
            return "zh";
        case System:
            return "system";
        default:
            return "en";
    }
}

I18nManager::Language I18nManager::codeToLanguage(const QString& code)
{
    if (code == "zh" || code.startsWith("zh_")) {
        return Chinese;
    } else if (code == "en" || code.startsWith("en_")) {
        return English;
    } else if (code == "system") {
        return System;
    }
    return English; // Default
}

QString I18nManager::languageToName(Language lang)
{
    switch (lang) {
        case English:
            return QObject::tr("English");
        case Chinese:
            return QObject::tr("简体中文");
        case System:
            return QObject::tr("System Default");
        default:
            return QObject::tr("Unknown");
    }
}
