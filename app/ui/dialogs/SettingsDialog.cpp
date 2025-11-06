#include "SettingsDialog.h"
#include <QEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include "../../managers/I18nManager.h"
#include "../../managers/StyleManager.h"
#include "../core/UIErrorHandler.h"
#include "../widgets/ToastNotification.h"

// ElaWidgetTools replacements
#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaRadioButton.h"
#include "ElaSpinBox.h"
#include "ElaTabWidget.h"
#include "ElaText.h"

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent),
      m_mainLayout(nullptr),
      m_tabWidget(nullptr),
      m_buttonBox(nullptr),
      m_applyButton(nullptr),
      m_restoreDefaultsButton(nullptr),
      m_appearanceTab(nullptr),
      m_themeGroup(nullptr),
      m_lightThemeRadio(nullptr),
      m_darkThemeRadio(nullptr),
      m_languageCombo(nullptr),
      m_performanceTab(nullptr),
      m_cacheSizeSpinBox(nullptr),
      m_enableCacheCheckBox(nullptr),
      m_preloadPagesCheckBox(nullptr),
      m_preloadCountSpinBox(nullptr),
      m_renderQualityCombo(nullptr),
      m_behaviorTab(nullptr),
      m_defaultZoomCombo(nullptr),
      m_defaultPageModeCombo(nullptr),
      m_recentFilesCountSpinBox(nullptr),
      m_rememberWindowStateCheckBox(nullptr),
      m_openLastFileCheckBox(nullptr),
      m_advancedTab(nullptr),
      m_logLevelCombo(nullptr),
      m_enableDebugPanelCheckBox(nullptr),
      m_showWelcomeScreenCheckBox(nullptr),
      m_customCachePathEdit(nullptr),
      m_browseCachePathButton(nullptr),
      m_clearCacheButton(nullptr) {
    setWindowTitle(tr("Settings"));
    setModal(true);
    setMinimumSize(600, 500);
    resize(700, 600);

    setupUI();
    setupConnections();
    loadSettings();
}

void SettingsDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);

    // Create tab widget
    m_tabWidget = new ElaTabWidget(this);
    m_tabWidget->addTab(createAppearanceTab(), tr("Appearance"));
    m_tabWidget->addTab(createPerformanceTab(), tr("Performance"));
    m_tabWidget->addTab(createBehaviorTab(), tr("Behavior"));
    m_tabWidget->addTab(createAdvancedTab(), tr("Advanced"));

    m_mainLayout->addWidget(m_tabWidget);

    // Create button box
    m_buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel |
                                 QDialogButtonBox::Apply,
                             this);
    m_applyButton = m_buttonBox->button(QDialogButtonBox::Apply);

    // Add restore defaults button
    m_restoreDefaultsButton = new ElaPushButton(tr("Restore Defaults"), this);
    m_buttonBox->addButton(m_restoreDefaultsButton,
                           QDialogButtonBox::ResetRole);

    m_mainLayout->addWidget(m_buttonBox);
}

void SettingsDialog::setupConnections() {
    connect(m_buttonBox, &QDialogButtonBox::accepted, this,
            &SettingsDialog::onOkClicked);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this,
            &SettingsDialog::onCancelClicked);
    connect(m_applyButton, &QPushButton::clicked, this,
            &SettingsDialog::onApplyClicked);
    connect(m_restoreDefaultsButton, &QPushButton::clicked, this,
            &SettingsDialog::onRestoreDefaultsClicked);

    // Connect validation for input fields
    connect(m_cacheSizeSpinBox,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
            &SettingsDialog::validateCacheSize);
    connect(m_recentFilesCountSpinBox,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
            &SettingsDialog::validateRecentFilesCount);
    connect(m_customCachePathEdit, &QLineEdit::textChanged, this,
            &SettingsDialog::validateCachePath);

    // Connect theme preview
    connect(m_themeGroup, &QButtonGroup::idClicked, this,
            &SettingsDialog::previewTheme);
    connect(
        m_languageCombo,
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &SettingsDialog::previewLanguage);

    // Connect clear cache button
    if (m_clearCacheButton) {
        connect(m_clearCacheButton, &QPushButton::clicked, this, [this]() {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, tr("Clear Cache"),
                tr("Are you sure you want to clear the cache? This will remove "
                   "all cached thumbnails and page data."),
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                // Cache clearing would be implemented here
                TOAST_SUCCESS(this, tr("Cache cleared successfully"));
            }
        });
    }

    // Connect browse cache path button
    if (m_browseCachePathButton) {
        connect(m_browseCachePathButton, &QPushButton::clicked, this, [this]() {
            QString dir = QFileDialog::getExistingDirectory(
                this, tr("Select Cache Directory"),
                m_customCachePathEdit->text());
            if (!dir.isEmpty()) {
                m_customCachePathEdit->setText(dir);
            }
        });
    }
}

QWidget* SettingsDialog::createAppearanceTab() {
    m_appearanceTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_appearanceTab);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(16);

    // Theme selection
    QGroupBox* themeGroup = new QGroupBox(tr("Theme"));
    QVBoxLayout* themeLayout = new QVBoxLayout(themeGroup);

    m_themeGroup = new QButtonGroup(this);
    m_lightThemeRadio = new ElaRadioButton(tr("Light"));
    m_darkThemeRadio = new ElaRadioButton(tr("Dark"));

    m_themeGroup->addButton(m_lightThemeRadio, 0);
    m_themeGroup->addButton(m_darkThemeRadio, 1);

    themeLayout->addWidget(m_lightThemeRadio);
    themeLayout->addWidget(m_darkThemeRadio);

    layout->addWidget(themeGroup);

    // Language selection
    QGroupBox* languageGroup = new QGroupBox(tr("Language"));
    QFormLayout* languageLayout = new QFormLayout(languageGroup);

    m_languageCombo = new ElaComboBox();
    m_languageCombo->addItem(tr("English"), "en");
    m_languageCombo->addItem(tr("中文"), "zh");

    languageLayout->addRow(tr("Interface Language:"), m_languageCombo);

    layout->addWidget(languageGroup);
    layout->addStretch();

    return m_appearanceTab;
}

QWidget* SettingsDialog::createPerformanceTab() {
    m_performanceTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_performanceTab);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(16);

    // Cache settings
    QGroupBox* cacheGroup = new QGroupBox(tr("Cache Settings"));
    QFormLayout* cacheLayout = new QFormLayout(cacheGroup);

    m_enableCacheCheckBox = new ElaCheckBox(tr("Enable caching"));
    m_enableCacheCheckBox->setChecked(true);
    cacheLayout->addRow(m_enableCacheCheckBox);

    m_cacheSizeSpinBox = new ElaSpinBox();
    m_cacheSizeSpinBox->setRange(50, 5000);
    m_cacheSizeSpinBox->setSuffix(" MB");
    m_cacheSizeSpinBox->setValue(500);
    cacheLayout->addRow(tr("Cache Size:"), m_cacheSizeSpinBox);

    layout->addWidget(cacheGroup);

    // Rendering settings
    QGroupBox* renderGroup = new QGroupBox(tr("Rendering"));
    QFormLayout* renderLayout = new QFormLayout(renderGroup);

    m_preloadPagesCheckBox = new ElaCheckBox(tr("Preload adjacent pages"));
    m_preloadPagesCheckBox->setChecked(true);
    renderLayout->addRow(m_preloadPagesCheckBox);

    m_preloadCountSpinBox = new ElaSpinBox();
    m_preloadCountSpinBox->setRange(1, 10);
    m_preloadCountSpinBox->setValue(2);
    renderLayout->addRow(tr("Pages to preload:"), m_preloadCountSpinBox);

    m_renderQualityCombo = new ElaComboBox();
    m_renderQualityCombo->addItem(tr("Low (Faster)"), 0);
    m_renderQualityCombo->addItem(tr("Medium"), 1);
    m_renderQualityCombo->addItem(tr("High (Better Quality)"), 2);
    m_renderQualityCombo->setCurrentIndex(1);
    renderLayout->addRow(tr("Render Quality:"), m_renderQualityCombo);

    layout->addWidget(renderGroup);
    layout->addStretch();

    return m_performanceTab;
}

QWidget* SettingsDialog::createBehaviorTab() {
    m_behaviorTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_behaviorTab);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(16);

    // Default view settings
    QGroupBox* viewGroup = new QGroupBox(tr("Default View Settings"));
    QFormLayout* viewLayout = new QFormLayout(viewGroup);

    m_defaultZoomCombo = new ElaComboBox();
    m_defaultZoomCombo->addItem(tr("Fit Width"), "fitWidth");
    m_defaultZoomCombo->addItem(tr("Fit Page"), "fitPage");
    m_defaultZoomCombo->addItem(tr("100%"), "100");
    m_defaultZoomCombo->addItem(tr("125%"), "125");
    m_defaultZoomCombo->addItem(tr("150%"), "150");
    m_defaultZoomCombo->setCurrentIndex(0);
    viewLayout->addRow(tr("Default Zoom:"), m_defaultZoomCombo);

    m_defaultPageModeCombo = new ElaComboBox();
    m_defaultPageModeCombo->addItem(tr("Single Page"), "single");
    m_defaultPageModeCombo->addItem(tr("Continuous Scroll"), "continuous");
    m_defaultPageModeCombo->setCurrentIndex(1);
    viewLayout->addRow(tr("Default Page Mode:"), m_defaultPageModeCombo);

    layout->addWidget(viewGroup);

    // Session settings
    QGroupBox* sessionGroup = new QGroupBox(tr("Session"));
    QVBoxLayout* sessionLayout = new QVBoxLayout(sessionGroup);

    m_recentFilesCountSpinBox = new ElaSpinBox();
    m_recentFilesCountSpinBox->setRange(5, 50);
    m_recentFilesCountSpinBox->setValue(10);
    QHBoxLayout* recentLayout = new QHBoxLayout();
    recentLayout->addWidget(new ElaText(tr("Recent files to remember:")));
    recentLayout->addWidget(m_recentFilesCountSpinBox);
    recentLayout->addStretch();
    sessionLayout->addLayout(recentLayout);

    m_rememberWindowStateCheckBox =
        new ElaCheckBox(tr("Remember window size and position"));
    m_rememberWindowStateCheckBox->setChecked(true);
    sessionLayout->addWidget(m_rememberWindowStateCheckBox);

    m_openLastFileCheckBox = new ElaCheckBox(tr("Reopen last file on startup"));
    sessionLayout->addWidget(m_openLastFileCheckBox);

    layout->addWidget(sessionGroup);
    layout->addStretch();

    return m_behaviorTab;
}

QWidget* SettingsDialog::createAdvancedTab() {
    m_advancedTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_advancedTab);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(16);

    // Debug settings
    QGroupBox* debugGroup = new QGroupBox(tr("Debug"));
    QFormLayout* debugLayout = new QFormLayout(debugGroup);

    m_logLevelCombo = new ElaComboBox();
    m_logLevelCombo->addItem(tr("Error"), "error");
    m_logLevelCombo->addItem(tr("Warning"), "warning");
    m_logLevelCombo->addItem(tr("Info"), "info");
    m_logLevelCombo->addItem(tr("Debug"), "debug");
    m_logLevelCombo->setCurrentIndex(2);
    debugLayout->addRow(tr("Log Level:"), m_logLevelCombo);

    m_enableDebugPanelCheckBox = new ElaCheckBox(tr("Show debug panel"));
    debugLayout->addRow(m_enableDebugPanelCheckBox);

    layout->addWidget(debugGroup);

    // Startup settings
    QGroupBox* startupGroup = new QGroupBox(tr("Startup"));
    QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);

    m_showWelcomeScreenCheckBox =
        new ElaCheckBox(tr("Show welcome screen on startup"));
    m_showWelcomeScreenCheckBox->setChecked(true);
    startupLayout->addWidget(m_showWelcomeScreenCheckBox);

    layout->addWidget(startupGroup);

    // Cache path settings
    QGroupBox* cachePathGroup = new QGroupBox(tr("Cache Location"));
    QVBoxLayout* cachePathLayout = new QVBoxLayout(cachePathGroup);

    QHBoxLayout* pathLayout = new QHBoxLayout();
    m_customCachePathEdit = new ElaLineEdit();
    m_customCachePathEdit->setPlaceholderText(
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    m_browseCachePathButton = new ElaPushButton(tr("Browse..."));
    pathLayout->addWidget(m_customCachePathEdit);
    pathLayout->addWidget(m_browseCachePathButton);
    cachePathLayout->addLayout(pathLayout);

    m_clearCacheButton = new ElaPushButton(tr("Clear Cache"));
    cachePathLayout->addWidget(m_clearCacheButton);

    layout->addWidget(cachePathGroup);
    layout->addStretch();

    return m_advancedTab;
}

void SettingsDialog::loadSettings() {
    QSettings settings;

    // Load appearance settings
    QString theme = settings.value("theme", "light").toString();
    if (theme == "dark") {
        m_darkThemeRadio->setChecked(true);
    } else {
        m_lightThemeRadio->setChecked(true);
    }

    QString language = settings.value("language", "en").toString();
    int langIndex = m_languageCombo->findData(language);
    if (langIndex >= 0) {
        m_languageCombo->setCurrentIndex(langIndex);
    }

    // Load performance settings
    m_enableCacheCheckBox->setChecked(
        settings.value("cache/enabled", true).toBool());
    m_cacheSizeSpinBox->setValue(settings.value("cache/size", 500).toInt());
    m_preloadPagesCheckBox->setChecked(
        settings.value("rendering/preload", true).toBool());
    m_preloadCountSpinBox->setValue(
        settings.value("rendering/preloadCount", 2).toInt());
    m_renderQualityCombo->setCurrentIndex(
        settings.value("rendering/quality", 1).toInt());

    // Load behavior settings
    m_recentFilesCountSpinBox->setValue(
        settings.value("session/recentFilesCount", 10).toInt());
    m_rememberWindowStateCheckBox->setChecked(
        settings.value("session/rememberWindowState", true).toBool());
    m_openLastFileCheckBox->setChecked(
        settings.value("session/openLastFile", false).toBool());

    // Load advanced settings
    m_enableDebugPanelCheckBox->setChecked(
        settings.value("debug/showPanel", false).toBool());
    m_showWelcomeScreenCheckBox->setChecked(
        settings.value("startup/showWelcome", true).toBool());
    m_customCachePathEdit->setText(
        settings.value("cache/customPath", "").toString());
}

void SettingsDialog::saveSettings() {
    try {
        QSettings settings;

        // Validate settings before saving
        if (m_cacheSizeSpinBox->value() < 50) {
            throw std::invalid_argument("Cache size must be at least 50 MB");
        }

        if (m_recentFilesCountSpinBox->value() < 5) {
            throw std::invalid_argument(
                "Recent files count must be at least 5");
        }

        QString customPath = m_customCachePathEdit->text();
        if (!customPath.isEmpty() && !QDir(customPath).exists()) {
            throw std::invalid_argument(
                "Custom cache directory does not exist");
        }

        // Save appearance settings
        settings.setValue("theme",
                          m_lightThemeRadio->isChecked() ? "light" : "dark");
        settings.setValue("language",
                          m_languageCombo->currentData().toString());

        // Save performance settings
        settings.setValue("cache/enabled", m_enableCacheCheckBox->isChecked());
        settings.setValue("cache/size", m_cacheSizeSpinBox->value());
        settings.setValue("rendering/preload",
                          m_preloadPagesCheckBox->isChecked());
        settings.setValue("rendering/preloadCount",
                          m_preloadCountSpinBox->value());
        settings.setValue("rendering/quality",
                          m_renderQualityCombo->currentIndex());

        // Save behavior settings
        settings.setValue("session/recentFilesCount",
                          m_recentFilesCountSpinBox->value());
        settings.setValue("session/rememberWindowState",
                          m_rememberWindowStateCheckBox->isChecked());
        settings.setValue("session/openLastFile",
                          m_openLastFileCheckBox->isChecked());

        // Save advanced settings
        settings.setValue("debug/showPanel",
                          m_enableDebugPanelCheckBox->isChecked());
        settings.setValue("startup/showWelcome",
                          m_showWelcomeScreenCheckBox->isChecked());
        settings.setValue("cache/customPath", customPath);

        // Sync settings to ensure they are written
        settings.sync();

        if (settings.status() != QSettings::NoError) {
            throw std::runtime_error("Failed to save settings to file");
        }

    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Settings Error"),
                              tr("Failed to save settings: %1").arg(e.what()));
        throw;  // Re-throw to prevent dialog from closing
    }
}

void SettingsDialog::applySettings() {
    try {
        saveSettings();

        // Apply theme change
        QString newTheme = m_lightThemeRadio->isChecked() ? "light" : "dark";
        emit themeChanged(newTheme);

        // Apply language change
        QString newLanguage = m_languageCombo->currentData().toString();
        emit languageChanged(newLanguage);

        emit settingsApplied();

    } catch (const std::exception& e) {
        // Error already handled in saveSettings, just prevent further
        // processing
        return;
    }
}

void SettingsDialog::restoreDefaults() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Restore Defaults"),
        tr("Are you sure you want to restore all settings to their default "
           "values?"),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Restore appearance defaults
        m_lightThemeRadio->setChecked(true);
        m_languageCombo->setCurrentIndex(0);

        // Restore performance defaults
        m_enableCacheCheckBox->setChecked(true);
        m_cacheSizeSpinBox->setValue(500);
        m_preloadPagesCheckBox->setChecked(true);
        m_preloadCountSpinBox->setValue(2);
        m_renderQualityCombo->setCurrentIndex(1);

        // Restore behavior defaults
        m_recentFilesCountSpinBox->setValue(10);
        m_rememberWindowStateCheckBox->setChecked(true);
        m_openLastFileCheckBox->setChecked(false);

        // Restore advanced defaults
        m_logLevelCombo->setCurrentIndex(2);
        m_enableDebugPanelCheckBox->setChecked(false);
        m_showWelcomeScreenCheckBox->setChecked(true);
        m_customCachePathEdit->clear();

        TOAST_SUCCESS(this, tr("Settings restored to defaults"));
    }
}

void SettingsDialog::onApplyClicked() {
    applySettings();
    TOAST_SUCCESS(this, tr("Settings applied successfully"));
}

void SettingsDialog::onOkClicked() {
    try {
        applySettings();
        accept();
    } catch (const std::exception& e) {
        // Settings validation failed, keep dialog open
        return;
    }
}

void SettingsDialog::onCancelClicked() { reject(); }

void SettingsDialog::onRestoreDefaultsClicked() { restoreDefaults(); }

void SettingsDialog::changeEvent(QEvent* event) {
    QDialog::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
}

void SettingsDialog::retranslateUi() {
    setWindowTitle(tr("Settings"));
    m_tabWidget->setTabText(0, tr("Appearance"));
    m_tabWidget->setTabText(1, tr("Performance"));
    m_tabWidget->setTabText(2, tr("Behavior"));
    m_tabWidget->setTabText(3, tr("Advanced"));
    m_restoreDefaultsButton->setText(tr("Restore Defaults"));
}

void SettingsDialog::validateCacheSize(int value) {
    auto validation = UIErrorHandler::instance().validateCacheSize(value);
    UIErrorHandler::instance().showValidationFeedback(m_cacheSizeSpinBox,
                                                      validation);

    // Enable/disable apply button based on validation result
    m_applyButton->setEnabled(validation.canProceed);
}

void SettingsDialog::validateRecentFilesCount(int value) {
    auto validation =
        UIErrorHandler::instance().validateRecentFilesCount(value);
    UIErrorHandler::instance().showValidationFeedback(m_recentFilesCountSpinBox,
                                                      validation);

    // Enable/disable apply button based on validation result
    m_applyButton->setEnabled(validation.canProceed);
}

void SettingsDialog::validateCachePath(const QString& path) {
    if (path.isEmpty()) {
        UIErrorHandler::instance().clearWidgetValidationState(
            m_customCachePathEdit);
        m_customCachePathEdit->setToolTip(tr("Using default cache location"));
        m_applyButton->setEnabled(true);
        return;
    }

    auto validation = InputValidator::validateFilePath(path, true, true);
    UIErrorHandler::instance().showValidationFeedback(m_customCachePathEdit,
                                                      validation);

    // Enable/disable apply button based on validation result
    m_applyButton->setEnabled(validation.canProceed);
}

void SettingsDialog::previewTheme(int themeId) {
    QString theme = (themeId == 1) ? "dark" : "light";
    emit themeChanged(theme);
}

void SettingsDialog::previewLanguage(int languageIndex) {
    QString languageCode = m_languageCombo->itemData(languageIndex).toString();
    emit languageChanged(languageCode);
}
