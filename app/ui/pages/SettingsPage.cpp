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
    setWindowTitle(tr("Settings"));

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
    m_appearanceGroup = createAppearanceGroup();
    scrollLayout->addWidget(m_appearanceGroup);

    // 查看器设置
    setupViewerSection();
    m_viewerGroup = createViewerGroup();
    scrollLayout->addWidget(m_viewerGroup);

    // 性能设置
    setupPerformanceSection();
    m_performanceGroup = createPerformanceGroup();
    scrollLayout->addWidget(m_performanceGroup);

    // 高级设置
    setupAdvancedSection();
    m_advancedGroup = createAdvancedGroup();
    scrollLayout->addWidget(m_advancedGroup);

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
    m_defaultZoomCombo->addItem(tr("50%"), 0.5);
    m_defaultZoomCombo->addItem(tr("75%"), 0.75);
    m_defaultZoomCombo->addItem(tr("100%"), 1.0);
    m_defaultZoomCombo->addItem(tr("125%"), 1.25);
    m_defaultZoomCombo->addItem(tr("150%"), 1.5);
    m_defaultZoomCombo->addItem(tr("200%"), 2.0);

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
    m_saveBtn = new ElaPushButton(this);
    m_cancelBtn = new ElaPushButton(this);
    m_resetBtn = new ElaPushButton(this);
    updateButtonTexts();
}

QGroupBox* SettingsPage::createAppearanceGroup() {
    QGroupBox* group = new QGroupBox(tr("Appearance"), this);
    QFormLayout* layout = new QFormLayout(group);
    layout->addRow(tr("Theme:"), m_themeCombo);
    layout->addRow(tr("Language:"), m_languageCombo);
    m_appearanceFormLayout = layout;
    return group;
}

QGroupBox* SettingsPage::createViewerGroup() {
    QGroupBox* group = new QGroupBox(tr("Viewer"), this);
    QFormLayout* layout = new QFormLayout(group);
    layout->addRow(tr("Default zoom:"), m_defaultZoomCombo);
    layout->addRow(tr("Default view mode:"), m_defaultViewModeCombo);
    layout->addRow(m_rememberLastPageCheck);
    layout->addRow(m_smoothScrollCheck);
    m_viewerFormLayout = layout;
    return group;
}

QGroupBox* SettingsPage::createPerformanceGroup() {
    QGroupBox* group = new QGroupBox(tr("Performance"), this);
    QFormLayout* layout = new QFormLayout(group);
    layout->addRow(tr("Cache size (MB):"), m_cacheSizeSlider);
    layout->addRow(tr("Render quality:"), m_renderQualityCombo);
    layout->addRow(m_hardwareAccelCheck);
    m_performanceFormLayout = layout;
    return group;
}

QGroupBox* SettingsPage::createAdvancedGroup() {
    QGroupBox* group = new QGroupBox(tr("Advanced"), this);
    QFormLayout* layout = new QFormLayout(group);
    layout->addRow(m_autoSaveCheck);

    QHBoxLayout* intervalLayout = new QHBoxLayout();
    m_autoSaveIntervalLabel = new ElaText(tr("Auto-save interval:"), this);
    intervalLayout->addWidget(m_autoSaveIntervalLabel);
    intervalLayout->addWidget(m_autoSaveIntervalEdit);
    m_autoSaveIntervalUnitLabel = new ElaText(tr("minutes"), this);
    intervalLayout->addWidget(m_autoSaveIntervalUnitLabel);
    layout->addRow(intervalLayout);

    layout->addRow(m_resetShortcutsBtn);
    layout->addRow(m_managePluginsBtn);
    m_advancedFormLayout = layout;
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
    if (m_styleManager != nullptr) {
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
    if (m_i18nManager != nullptr) {
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
    ElaScrollPage::changeEvent(event);
}

void SettingsPage::retranslateUi() {
    SLOG_INFO("SettingsPage: Retranslating UI");
    setWindowTitle(tr("Settings"));

    updateAppearanceTexts();
    updateViewerTexts();
    updatePerformanceTexts();
    updateAdvancedTexts();
    updateButtonTexts();
}

void SettingsPage::updateAppearanceTexts() {
    if (m_appearanceGroup != nullptr) {
        m_appearanceGroup->setTitle(tr("Appearance"));
    }

    if (m_themeCombo != nullptr && m_themeCombo->count() >= 3) {
        m_themeCombo->setItemText(0, tr("Light"));
        m_themeCombo->setItemText(1, tr("Dark"));
        m_themeCombo->setItemText(2, tr("Auto"));
    }

    if (m_languageCombo != nullptr) {
        int englishIndex = m_languageCombo->findData("en");
        if (englishIndex >= 0) {
            m_languageCombo->setItemText(englishIndex, tr("English"));
        }
        int chineseIndex = m_languageCombo->findData("zh_CN");
        if (chineseIndex >= 0) {
            m_languageCombo->setItemText(chineseIndex, tr("中文"));
        }
    }

    if (m_appearanceFormLayout != nullptr) {
        if (auto* themeLabel = qobject_cast<QLabel*>(
                m_appearanceFormLayout->labelForField(m_themeCombo))) {
            themeLabel->setText(tr("Theme:"));
        }
        if (auto* languageLabel = qobject_cast<QLabel*>(
                m_appearanceFormLayout->labelForField(m_languageCombo))) {
            languageLabel->setText(tr("Language:"));
        }
    }
}

void SettingsPage::updateViewerTexts() {
    if (m_viewerGroup != nullptr) {
        m_viewerGroup->setTitle(tr("Viewer"));
    }

    if (m_defaultZoomCombo != nullptr) {
        const struct {
            double value;
            QString text;
        } zoomOptions[] = {
            {-1, tr("Fit Width")}, {-2, tr("Fit Page")}, {-3, tr("Fit Height")},
            {0.5, tr("50%")},      {0.75, tr("75%")},    {1.0, tr("100%")},
            {1.25, tr("125%")},    {1.5, tr("150%")},    {2.0, tr("200%")}};

        for (const auto& option : zoomOptions) {
            int index = m_defaultZoomCombo->findData(option.value);
            if (index >= 0) {
                m_defaultZoomCombo->setItemText(index, option.text);
            }
        }
    }

    if (m_defaultViewModeCombo != nullptr) {
        const struct {
            int value;
            QString text;
        } viewOptions[] = {{0, tr("Single Page")},
                           {1, tr("Continuous")},
                           {2, tr("Two Pages")},
                           {3, tr("Book Mode")}};

        for (const auto& option : viewOptions) {
            int index = m_defaultViewModeCombo->findData(option.value);
            if (index >= 0) {
                m_defaultViewModeCombo->setItemText(index, option.text);
            }
        }
    }

    if (m_rememberLastPageCheck != nullptr) {
        m_rememberLastPageCheck->setText(tr("Remember last page"));
    }
    if (m_smoothScrollCheck != nullptr) {
        m_smoothScrollCheck->setText(tr("Smooth scrolling"));
    }

    if (m_viewerFormLayout != nullptr) {
        if (auto* zoomLabel = qobject_cast<QLabel*>(
                m_viewerFormLayout->labelForField(m_defaultZoomCombo))) {
            zoomLabel->setText(tr("Default zoom:"));
        }
        if (auto* viewModeLabel = qobject_cast<QLabel*>(
                m_viewerFormLayout->labelForField(m_defaultViewModeCombo))) {
            viewModeLabel->setText(tr("Default view mode:"));
        }
    }
}

void SettingsPage::updatePerformanceTexts() {
    if (m_performanceGroup != nullptr) {
        m_performanceGroup->setTitle(tr("Performance"));
    }

    if (m_renderQualityCombo != nullptr) {
        const struct {
            int value;
            QString text;
        } qualityOptions[] = {{0, tr("Low")},
                              {1, tr("Medium")},
                              {2, tr("High")},
                              {3, tr("Very High")}};

        for (const auto& option : qualityOptions) {
            int index = m_renderQualityCombo->findData(option.value);
            if (index >= 0) {
                m_renderQualityCombo->setItemText(index, option.text);
            }
        }
    }

    if (m_cacheSizeSlider != nullptr) {
        // Slider has no text to translate.
    }

    if (m_hardwareAccelCheck != nullptr) {
        m_hardwareAccelCheck->setText(tr("Hardware acceleration"));
    }

    if (m_performanceFormLayout != nullptr) {
        if (auto* cacheLabel = qobject_cast<QLabel*>(
                m_performanceFormLayout->labelForField(m_cacheSizeSlider))) {
            cacheLabel->setText(tr("Cache size (MB):"));
        }
        if (auto* renderLabel = qobject_cast<QLabel*>(
                m_performanceFormLayout->labelForField(m_renderQualityCombo))) {
            renderLabel->setText(tr("Render quality:"));
        }
    }
}

void SettingsPage::updateAdvancedTexts() {
    if (m_advancedGroup != nullptr) {
        m_advancedGroup->setTitle(tr("Advanced"));
    }

    if (m_autoSaveCheck != nullptr) {
        m_autoSaveCheck->setText(tr("Auto-save settings"));
    }

    if (m_autoSaveIntervalEdit != nullptr) {
        m_autoSaveIntervalEdit->setPlaceholderText(tr("Minutes"));
    }

    if (m_autoSaveIntervalLabel != nullptr) {
        m_autoSaveIntervalLabel->setText(tr("Auto-save interval:"));
    }

    if (m_autoSaveIntervalUnitLabel != nullptr) {
        m_autoSaveIntervalUnitLabel->setText(tr("minutes"));
    }

    if (m_resetShortcutsBtn != nullptr) {
        m_resetShortcutsBtn->setText(tr("Reset Shortcuts"));
    }

    if (m_managePluginsBtn != nullptr) {
        m_managePluginsBtn->setText(tr("Manage Plugins"));
    }

    if (m_advancedFormLayout != nullptr && m_autoSaveCheck != nullptr) {
        // The checkbox row does not require additional labels.
    }
}

void SettingsPage::updateButtonTexts() {
    if (m_saveBtn != nullptr) {
        m_saveBtn->setText(tr("Save"));
    }
    if (m_cancelBtn != nullptr) {
        m_cancelBtn->setText(tr("Cancel"));
    }
    if (m_resetBtn != nullptr) {
        m_resetBtn->setText(tr("Reset to Defaults"));
    }
}
