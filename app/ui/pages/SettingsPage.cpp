#include "SettingsPage.h"

// Business Logic
#include "managers/I18nManager.h"
#include "managers/StyleManager.h"

// ElaWidgetTools
#include "ElaCheckBox.h"
#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaSlider.h"

#include "ElaText.h"
// Qt
#include <QEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>

// Logging
#include "logging/SimpleLogging.h"

// ============================================================================
// 构造和析构
// ============================================================================

SettingsPage::SettingsPage(QWidget* parent)
    : ElaScrollPage(parent), m_i18nManager(nullptr), m_styleManager(nullptr) {
    SLOG_INFO("SettingsPage: Constructor started");

    // Set window title for navigation
    setWindowTitle("Settings");

    // Set title visibility (following example pattern)
    setTitleVisible(false);

    // Set margins following HomePage pattern for consistency
    setContentsMargins(2, 2, 0, 0);

    setupUi();
    connectSignals();
    loadSettings();

    SLOG_INFO("SettingsPage: Constructor completed");
}

SettingsPage::~SettingsPage() { SLOG_INFO("SettingsPage: Destructor called"); }

// ============================================================================
// UI 初始化
// ============================================================================

void SettingsPage::setupUi() {
    // Create central widget
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(20);

    // Create content widget
    QWidget* scrollWidget = new QWidget(centralWidget);
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setSpacing(20);

    // 外观设置
    setupAppearanceSection();
    scrollLayout->addWidget(createAppearanceGroup());

    // 查看器设置
    setupViewerSection();
    scrollLayout->addWidget(createViewerGroup());

    // 性能设置
    setupPerformanceSection();
    scrollLayout->addWidget(createPerformanceGroup());

    // 高级设置
    setupAdvancedSection();
    scrollLayout->addWidget(createAdvancedGroup());

    scrollLayout->addStretch();

    mainLayout->addWidget(scrollWidget);

    // 按钮
    setupButtons();
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveBtn);
    buttonLayout->addWidget(m_cancelBtn);
    buttonLayout->addWidget(m_resetBtn);
    mainLayout->addLayout(buttonLayout);

    // Add central widget using ElaScrollPage method (following example pattern)
    addCentralWidget(centralWidget, true, true, 0.5);
}

void SettingsPage::setupAppearanceSection() {
    m_themeCombo = new ElaComboBox(this);
    m_themeCombo->addItem(tr("Light"));
    m_themeCombo->addItem(tr("Dark"));
    m_themeCombo->addItem(tr("Auto"));

    m_languageCombo = new ElaComboBox(this);
    m_languageCombo->addItem(tr("English"), "en");
    m_languageCombo->addItem(tr("中文"), "zh_CN");
}

void SettingsPage::setupViewerSection() {
    m_defaultZoomCombo = new ElaComboBox(this);
    m_defaultZoomCombo->addItem(tr("Fit Width"), -1);
    m_defaultZoomCombo->addItem(tr("Fit Page"), -2);
    m_defaultZoomCombo->addItem(tr("Fit Height"), -3);
    m_defaultZoomCombo->addItem("50%", 0.5);
    m_defaultZoomCombo->addItem("75%", 0.75);
    m_defaultZoomCombo->addItem("100%", 1.0);
    m_defaultZoomCombo->addItem("125%", 1.25);
    m_defaultZoomCombo->addItem("150%", 1.5);
    m_defaultZoomCombo->addItem("200%", 2.0);

    m_defaultViewModeCombo = new ElaComboBox(this);
    m_defaultViewModeCombo->addItem(tr("Single Page"), 0);
    m_defaultViewModeCombo->addItem(tr("Continuous"), 1);
    m_defaultViewModeCombo->addItem(tr("Two Pages"), 2);
    m_defaultViewModeCombo->addItem(tr("Book Mode"), 3);

    m_rememberLastPageCheck = new ElaCheckBox(tr("Remember last page"), this);
    m_smoothScrollCheck = new ElaCheckBox(tr("Smooth scrolling"), this);
}

void SettingsPage::setupPerformanceSection() {
    m_cacheSizeSlider = new ElaSlider(Qt::Horizontal, this);
    m_cacheSizeSlider->setRange(10, 100);
    m_cacheSizeSlider->setValue(50);

    m_renderQualityCombo = new ElaComboBox(this);
    m_renderQualityCombo->addItem(tr("Low"), 0);
    m_renderQualityCombo->addItem(tr("Medium"), 1);
    m_renderQualityCombo->addItem(tr("High"), 2);
    m_renderQualityCombo->addItem(tr("Very High"), 3);

    m_hardwareAccelCheck = new ElaCheckBox(tr("Hardware acceleration"), this);
}

void SettingsPage::setupAdvancedSection() {
    m_autoSaveCheck = new ElaCheckBox(tr("Auto-save settings"), this);
    m_autoSaveIntervalEdit = new ElaLineEdit(this);
    m_autoSaveIntervalEdit->setText("5");
    m_autoSaveIntervalEdit->setPlaceholderText(tr("Minutes"));

    m_resetShortcutsBtn = new ElaPushButton(tr("Reset Shortcuts"), this);
    m_managePluginsBtn = new ElaPushButton(tr("Manage Plugins"), this);
}

void SettingsPage::setupButtons() {
    m_saveBtn = new ElaPushButton(tr("Save"), this);
    m_cancelBtn = new ElaPushButton(tr("Cancel"), this);
    m_resetBtn = new ElaPushButton(tr("Reset to Defaults"), this);
}

QGroupBox* SettingsPage::createAppearanceGroup() {
    QGroupBox* group = new QGroupBox(tr("Appearance"), this);
    QFormLayout* layout = new QFormLayout(group);
    layout->addRow(tr("Theme:"), m_themeCombo);
    layout->addRow(tr("Language:"), m_languageCombo);
    return group;
}

QGroupBox* SettingsPage::createViewerGroup() {
    QGroupBox* group = new QGroupBox(tr("Viewer"), this);
    QFormLayout* layout = new QFormLayout(group);
    layout->addRow(tr("Default zoom:"), m_defaultZoomCombo);
    layout->addRow(tr("Default view mode:"), m_defaultViewModeCombo);
    layout->addRow(m_rememberLastPageCheck);
    layout->addRow(m_smoothScrollCheck);
    return group;
}

QGroupBox* SettingsPage::createPerformanceGroup() {
    QGroupBox* group = new QGroupBox(tr("Performance"), this);
    QFormLayout* layout = new QFormLayout(group);
    layout->addRow(tr("Cache size (MB):"), m_cacheSizeSlider);
    layout->addRow(tr("Render quality:"), m_renderQualityCombo);
    layout->addRow(m_hardwareAccelCheck);
    return group;
}

QGroupBox* SettingsPage::createAdvancedGroup() {
    QGroupBox* group = new QGroupBox(tr("Advanced"), this);
    QFormLayout* layout = new QFormLayout(group);
    layout->addRow(m_autoSaveCheck);

    QHBoxLayout* intervalLayout = new QHBoxLayout();
    intervalLayout->addWidget(new ElaText(tr("Auto-save interval:"), this));
    intervalLayout->addWidget(m_autoSaveIntervalEdit);
    intervalLayout->addWidget(new ElaText(tr("minutes"), this));
    layout->addRow(intervalLayout);

    layout->addRow(m_resetShortcutsBtn);
    layout->addRow(m_managePluginsBtn);
    return group;
}

void SettingsPage::connectSignals() {
    // 保存按钮
    connect(m_saveBtn, &ElaPushButton::clicked, this, [this]() {
        saveSettings();
        applySettings();
        emit settingsChanged();
    });

    // 取消按钮
    connect(m_cancelBtn, &ElaPushButton::clicked, this,
            &SettingsPage::loadSettings);

    // 重置按钮
    connect(m_resetBtn, &ElaPushButton::clicked, this,
            &SettingsPage::resetToDefaults);

    // 主题变化
    connect(m_themeCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, [this](int index) {
                QString theme;
                switch (index) {
                    case 0:
                        theme = "light";
                        break;
                    case 1:
                        theme = "dark";
                        break;
                    case 2:
                        theme = "auto";
                        break;
                }
                emit themeChanged(theme);
            });

    // 语言变化
    connect(m_languageCombo,
            QOverload<int>::of(&ElaComboBox::currentIndexChanged), this,
            [this](int index) {
                QString language = m_languageCombo->itemData(index).toString();
                emit languageChanged(language);
            });
}

// ============================================================================
// 设置管理
// ============================================================================

void SettingsPage::loadSettings() {
    SLOG_INFO("SettingsPage: Loading settings");

    QSettings settings("SAST", "Readium");

    // 外观
    m_themeCombo->setCurrentIndex(
        settings.value("appearance/theme", 0).toInt());
    QString language = settings.value("appearance/language", "en").toString();
    int langIndex = m_languageCombo->findData(language);
    if (langIndex >= 0) {
        m_languageCombo->setCurrentIndex(langIndex);
    }

    // 查看器
    m_defaultZoomCombo->setCurrentIndex(
        settings.value("viewer/defaultZoom", 5).toInt());
    m_defaultViewModeCombo->setCurrentIndex(
        settings.value("viewer/defaultViewMode", 1).toInt());
    m_rememberLastPageCheck->setChecked(
        settings.value("viewer/rememberLastPage", true).toBool());
    m_smoothScrollCheck->setChecked(
        settings.value("viewer/smoothScroll", true).toBool());

    // 性能
    m_cacheSizeSlider->setValue(
        settings.value("performance/cacheSize", 50).toInt());
    m_renderQualityCombo->setCurrentIndex(
        settings.value("performance/renderQuality", 2).toInt());
    m_hardwareAccelCheck->setChecked(
        settings.value("performance/hardwareAccel", true).toBool());

    // 高级
    m_autoSaveCheck->setChecked(
        settings.value("advanced/autoSave", true).toBool());
    m_autoSaveIntervalEdit->setText(
        settings.value("advanced/autoSaveInterval", "5").toString());
}

void SettingsPage::saveSettings() {
    SLOG_INFO("SettingsPage: Saving settings");

    QSettings settings("SAST", "Readium");

    // 外观
    settings.setValue("appearance/theme", m_themeCombo->currentIndex());
    settings.setValue("appearance/language",
                      m_languageCombo->currentData().toString());

    // 查看器
    settings.setValue("viewer/defaultZoom", m_defaultZoomCombo->currentIndex());
    settings.setValue("viewer/defaultViewMode",
                      m_defaultViewModeCombo->currentIndex());
    settings.setValue("viewer/rememberLastPage",
                      m_rememberLastPageCheck->isChecked());
    settings.setValue("viewer/smoothScroll", m_smoothScrollCheck->isChecked());

    // 性能
    settings.setValue("performance/cacheSize", m_cacheSizeSlider->value());
    settings.setValue("performance/renderQuality",
                      m_renderQualityCombo->currentIndex());
    settings.setValue("performance/hardwareAccel",
                      m_hardwareAccelCheck->isChecked());

    // 高级
    settings.setValue("advanced/autoSave", m_autoSaveCheck->isChecked());
    settings.setValue("advanced/autoSaveInterval",
                      m_autoSaveIntervalEdit->text());
}

void SettingsPage::resetToDefaults() {
    SLOG_INFO("SettingsPage: Resetting to defaults");

    m_themeCombo->setCurrentIndex(0);
    m_languageCombo->setCurrentIndex(0);
    m_defaultZoomCombo->setCurrentIndex(5);
    m_defaultViewModeCombo->setCurrentIndex(1);
    m_rememberLastPageCheck->setChecked(true);
    m_smoothScrollCheck->setChecked(true);
    m_cacheSizeSlider->setValue(50);
    m_renderQualityCombo->setCurrentIndex(2);
    m_hardwareAccelCheck->setChecked(true);
    m_autoSaveCheck->setChecked(true);
    m_autoSaveIntervalEdit->setText("5");
}

void SettingsPage::applySettings() {
    SLOG_INFO("SettingsPage: Applying settings");

    // 应用主题
    if (m_styleManager) {
        Theme theme = Theme::Light;
        switch (m_themeCombo->currentIndex()) {
            case 0:
                theme = Theme::Light;
                break;
            case 1:
                theme = Theme::Dark;
                break;
            case 2:
                theme = Theme::Light;
                break;  // "auto" defaults to Light for now
        }
        m_styleManager->setTheme(theme);
    }

    // 应用语言
    if (m_i18nManager) {
        QString languageCode = m_languageCombo->currentData().toString();
        m_i18nManager->loadLanguage(languageCode);
    }
}

// ============================================================================
// 业务逻辑集成
// ============================================================================

void SettingsPage::setI18nManager(I18nManager* manager) {
    m_i18nManager = manager;
}

void SettingsPage::setStyleManager(StyleManager* manager) {
    m_styleManager = manager;
}

// ============================================================================
// 事件处理
// ============================================================================

void SettingsPage::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void SettingsPage::retranslateUi() {
    SLOG_INFO("SettingsPage: Retranslating UI");
    // 所有组件都会自动处理语言变化
}
