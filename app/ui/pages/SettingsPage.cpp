#include "SettingsPage.h"

// Business Logic
#include "managers/I18nManager.h"
#include "managers/StyleManager.h"

// Settings widgets
#include "ui/widgets/AccessibilitySettingsWidget.h"
#include "ui/widgets/AnnotationSettingsWidget.h"
#include "ui/widgets/CacheSettingsWidget.h"
#include "ui/widgets/DocumentSettingsWidget.h"
#include "ui/widgets/LoggingSettingsWidget.h"
#include "ui/widgets/PluginSettingsWidget.h"
#include "ui/widgets/SearchSettingsWidget.h"
#include "ui/widgets/ShortcutSettingsWidget.h"
#include "ui/widgets/SystemTraySettingsWidget.h"

// ElaWidgetTools
#include "ElaCheckBox.h"
#include "ElaColorDialog.h"
#include "ElaComboBox.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaSlider.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ElaToggleSwitch.h"

// Qt
#include <QEvent>
#include <QHBoxLayout>
#include <QListWidget>
#include <QSettings>
#include <QStackedWidget>
#include <QVBoxLayout>

// Logging
#include "logging/SimpleLogging.h"

// ============================================================================
// Construction and destruction
// ============================================================================

SettingsPage::SettingsPage(QWidget* parent)
    : ElaScrollPage(parent),
      m_navigationList(nullptr),
      m_contentStack(nullptr),
      m_appearanceWidget(nullptr),
      m_viewerWidget(nullptr),
      m_documentWidget(nullptr),
      m_annotationWidget(nullptr),
      m_searchWidget(nullptr),
      m_cacheWidget(nullptr),
      m_systemTrayWidget(nullptr),
      m_shortcutsWidget(nullptr),
      m_accessibilityWidget(nullptr),
      m_pluginWidget(nullptr),
      m_loggingWidget(nullptr),
      m_themeCombo(nullptr),
      m_languageCombo(nullptr),
      m_fontSizeSpin(nullptr),
      m_animationsSwitch(nullptr),
      m_defaultZoomCombo(nullptr),
      m_defaultViewModeCombo(nullptr),
      m_rememberLastPageSwitch(nullptr),
      m_smoothScrollSwitch(nullptr),
      m_renderQualityCombo(nullptr),
      m_antiAliasingSwitch(nullptr),
      m_saveBtn(nullptr),
      m_cancelBtn(nullptr),
      m_resetBtn(nullptr),
      m_i18nManager(nullptr),
      m_styleManager(nullptr),
      m_hasUnsavedChanges(false) {
    SLOG_INFO("SettingsPage: Constructor started");

    setWindowTitle(tr("Settings"));
    setTitleVisible(false);
    setContentsMargins(2, 2, 0, 0);

    setupUi();
    connectSignals();
    loadSettings();

    SLOG_INFO("SettingsPage: Constructor completed");
}

SettingsPage::~SettingsPage() { SLOG_INFO("SettingsPage: Destructor called"); }

// ============================================================================
// UI Setup
// ============================================================================

void SettingsPage::setupUi() {
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(16);

    // Left navigation panel
    QWidget* navPanel = new QWidget(this);
    navPanel->setFixedWidth(200);
    QVBoxLayout* navLayout = new QVBoxLayout(navPanel);
    navLayout->setContentsMargins(0, 0, 0, 0);

    m_navigationList = new QListWidget(this);
    m_navigationList->setSpacing(2);
    m_navigationList->setFrameShape(QFrame::NoFrame);
    navLayout->addWidget(m_navigationList);

    mainLayout->addWidget(navPanel);

    // Right content area
    QWidget* contentPanel = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentPanel);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(16);

    m_contentStack = new QStackedWidget(this);
    contentLayout->addWidget(m_contentStack, 1);

    // Setup sections
    setupNavigation();
    setupSections();
    setupButtons();

    // Action buttons at bottom
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveBtn);
    buttonLayout->addWidget(m_cancelBtn);
    buttonLayout->addWidget(m_resetBtn);
    contentLayout->addLayout(buttonLayout);

    mainLayout->addWidget(contentPanel, 1);

    addCentralWidget(centralWidget, true, true, 0.5);
}

void SettingsPage::setupNavigation() {
    m_navigationList->addItem(tr("Appearance"));
    m_navigationList->addItem(tr("Viewer"));
    m_navigationList->addItem(tr("Document"));
    m_navigationList->addItem(tr("Annotations"));
    m_navigationList->addItem(tr("Search"));
    m_navigationList->addItem(tr("Cache"));
    m_navigationList->addItem(tr("System Tray"));
    m_navigationList->addItem(tr("Shortcuts"));
    m_navigationList->addItem(tr("Accessibility"));
    m_navigationList->addItem(tr("Plugins"));
    m_navigationList->addItem(tr("Logging"));
    m_navigationList->setCurrentRow(0);
}

void SettingsPage::setupSections() {
    // Appearance section
    m_appearanceWidget = createAppearanceSection();
    m_contentStack->addWidget(m_appearanceWidget);

    // Viewer section
    m_viewerWidget = createViewerSection();
    m_contentStack->addWidget(m_viewerWidget);

    // Document section
    m_documentWidget = new DocumentSettingsWidget(this);
    m_contentStack->addWidget(m_documentWidget);

    // Annotations section
    m_annotationWidget = new AnnotationSettingsWidget(this);
    m_contentStack->addWidget(m_annotationWidget);

    // Search section
    m_searchWidget = new SearchSettingsWidget(this);
    m_contentStack->addWidget(m_searchWidget);

    // Cache section
    m_cacheWidget = new CacheSettingsWidget(this);
    m_contentStack->addWidget(m_cacheWidget);

    // System Tray section
    m_systemTrayWidget = new SystemTraySettingsWidget(this);
    m_contentStack->addWidget(m_systemTrayWidget);

    // Shortcuts section
    m_shortcutsWidget = new ShortcutSettingsWidget(this);
    m_contentStack->addWidget(m_shortcutsWidget);

    // Accessibility section
    m_accessibilityWidget = new AccessibilitySettingsWidget(this);
    m_contentStack->addWidget(m_accessibilityWidget);

    // Plugins section
    m_pluginWidget = new PluginSettingsWidget(this);
    m_contentStack->addWidget(m_pluginWidget);

    // Logging section
    m_loggingWidget = new LoggingSettingsWidget(this);
    m_contentStack->addWidget(m_loggingWidget);
}

void SettingsPage::setupButtons() {
    m_saveBtn = new ElaPushButton(tr("Save"), this);
    m_cancelBtn = new ElaPushButton(tr("Cancel"), this);
    m_resetBtn = new ElaPushButton(tr("Reset to Defaults"), this);
}

QWidget* SettingsPage::createAppearanceSection() {
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    // Theme section
    auto* themeArea = new ElaScrollPageArea(this);
    auto* themeLayout = new QVBoxLayout(themeArea);
    themeLayout->setContentsMargins(16, 12, 16, 12);

    auto* themeTitle = new ElaText(tr("Theme"), this);
    themeTitle->setTextPixelSize(14);
    themeLayout->addWidget(themeTitle);

    auto* themeRow = new QHBoxLayout();
    auto* themeLabel = new ElaText(tr("Application theme:"), this);
    themeRow->addWidget(themeLabel);
    m_themeCombo = new ElaComboBox(this);
    m_themeCombo->addItem(tr("Light"), "light");
    m_themeCombo->addItem(tr("Dark"), "dark");
    m_themeCombo->addItem(tr("Auto"), "auto");
    themeRow->addWidget(m_themeCombo);
    themeRow->addStretch();
    themeLayout->addLayout(themeRow);

    layout->addWidget(themeArea);

    // Language section
    auto* langArea = new ElaScrollPageArea(this);
    auto* langLayout = new QVBoxLayout(langArea);
    langLayout->setContentsMargins(16, 12, 16, 12);

    auto* langTitle = new ElaText(tr("Language"), this);
    langTitle->setTextPixelSize(14);
    langLayout->addWidget(langTitle);

    auto* langRow = new QHBoxLayout();
    auto* langLabel = new ElaText(tr("Interface language:"), this);
    langRow->addWidget(langLabel);
    m_languageCombo = new ElaComboBox(this);
    m_languageCombo->addItem(tr("English"), "en");
    m_languageCombo->addItem(tr("中文"), "zh_CN");
    langRow->addWidget(m_languageCombo);
    langRow->addStretch();
    langLayout->addLayout(langRow);

    layout->addWidget(langArea);

    // Font section
    auto* fontArea = new ElaScrollPageArea(this);
    auto* fontLayout = new QVBoxLayout(fontArea);
    fontLayout->setContentsMargins(16, 12, 16, 12);

    auto* fontTitle = new ElaText(tr("Font"), this);
    fontTitle->setTextPixelSize(14);
    fontLayout->addWidget(fontTitle);

    auto* fontRow = new QHBoxLayout();
    auto* fontLabel = new ElaText(tr("Font size:"), this);
    fontRow->addWidget(fontLabel);
    m_fontSizeSpin = new ElaSpinBox(this);
    m_fontSizeSpin->setRange(8, 24);
    m_fontSizeSpin->setValue(12);
    m_fontSizeSpin->setSuffix(" pt");
    fontRow->addWidget(m_fontSizeSpin);
    fontRow->addStretch();
    fontLayout->addLayout(fontRow);

    layout->addWidget(fontArea);

    // Effects section
    auto* effectsArea = new ElaScrollPageArea(this);
    auto* effectsLayout = new QVBoxLayout(effectsArea);
    effectsLayout->setContentsMargins(16, 12, 16, 12);

    auto* effectsTitle = new ElaText(tr("Effects"), this);
    effectsTitle->setTextPixelSize(14);
    effectsLayout->addWidget(effectsTitle);

    auto* animRow = new QHBoxLayout();
    auto* animLabel = new ElaText(tr("Enable animations"), this);
    animRow->addWidget(animLabel);
    animRow->addStretch();
    m_animationsSwitch = new ElaToggleSwitch(this);
    m_animationsSwitch->setIsToggled(true);
    animRow->addWidget(m_animationsSwitch);
    effectsLayout->addLayout(animRow);

    layout->addWidget(effectsArea);
    layout->addStretch();

    return widget;
}

QWidget* SettingsPage::createViewerSection() {
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    // Default view section
    auto* viewArea = new ElaScrollPageArea(this);
    auto* viewLayout = new QVBoxLayout(viewArea);
    viewLayout->setContentsMargins(16, 12, 16, 12);

    auto* viewTitle = new ElaText(tr("Default View Settings"), this);
    viewTitle->setTextPixelSize(14);
    viewLayout->addWidget(viewTitle);

    auto* zoomRow = new QHBoxLayout();
    auto* zoomLabel = new ElaText(tr("Default zoom:"), this);
    zoomRow->addWidget(zoomLabel);
    m_defaultZoomCombo = new ElaComboBox(this);
    m_defaultZoomCombo->addItem(tr("Fit Width"), "fitWidth");
    m_defaultZoomCombo->addItem(tr("Fit Page"), "fitPage");
    m_defaultZoomCombo->addItem(tr("Fit Height"), "fitHeight");
    m_defaultZoomCombo->addItem(tr("50%"), "50");
    m_defaultZoomCombo->addItem(tr("75%"), "75");
    m_defaultZoomCombo->addItem(tr("100%"), "100");
    m_defaultZoomCombo->addItem(tr("125%"), "125");
    m_defaultZoomCombo->addItem(tr("150%"), "150");
    m_defaultZoomCombo->addItem(tr("200%"), "200");
    zoomRow->addWidget(m_defaultZoomCombo);
    zoomRow->addStretch();
    viewLayout->addLayout(zoomRow);

    auto* modeRow = new QHBoxLayout();
    auto* modeLabel = new ElaText(tr("Default view mode:"), this);
    modeRow->addWidget(modeLabel);
    m_defaultViewModeCombo = new ElaComboBox(this);
    m_defaultViewModeCombo->addItem(tr("Single Page"), "single");
    m_defaultViewModeCombo->addItem(tr("Continuous"), "continuous");
    m_defaultViewModeCombo->addItem(tr("Two Pages"), "twoPage");
    m_defaultViewModeCombo->addItem(tr("Book Mode"), "book");
    modeRow->addWidget(m_defaultViewModeCombo);
    modeRow->addStretch();
    viewLayout->addLayout(modeRow);

    layout->addWidget(viewArea);

    // Behavior section
    auto* behaviorArea = new ElaScrollPageArea(this);
    auto* behaviorLayout = new QVBoxLayout(behaviorArea);
    behaviorLayout->setContentsMargins(16, 12, 16, 12);

    auto* behaviorTitle = new ElaText(tr("Behavior"), this);
    behaviorTitle->setTextPixelSize(14);
    behaviorLayout->addWidget(behaviorTitle);

    auto* rememberRow = new QHBoxLayout();
    auto* rememberLabel = new ElaText(tr("Remember last page position"), this);
    rememberRow->addWidget(rememberLabel);
    rememberRow->addStretch();
    m_rememberLastPageSwitch = new ElaToggleSwitch(this);
    m_rememberLastPageSwitch->setIsToggled(true);
    rememberRow->addWidget(m_rememberLastPageSwitch);
    behaviorLayout->addLayout(rememberRow);

    auto* scrollRow = new QHBoxLayout();
    auto* scrollLabel = new ElaText(tr("Smooth scrolling"), this);
    scrollRow->addWidget(scrollLabel);
    scrollRow->addStretch();
    m_smoothScrollSwitch = new ElaToggleSwitch(this);
    m_smoothScrollSwitch->setIsToggled(true);
    scrollRow->addWidget(m_smoothScrollSwitch);
    behaviorLayout->addLayout(scrollRow);

    layout->addWidget(behaviorArea);

    // Rendering section
    auto* renderArea = new ElaScrollPageArea(this);
    auto* renderLayout = new QVBoxLayout(renderArea);
    renderLayout->setContentsMargins(16, 12, 16, 12);

    auto* renderTitle = new ElaText(tr("Rendering"), this);
    renderTitle->setTextPixelSize(14);
    renderLayout->addWidget(renderTitle);

    auto* qualityRow = new QHBoxLayout();
    auto* qualityLabel = new ElaText(tr("Render quality:"), this);
    qualityRow->addWidget(qualityLabel);
    m_renderQualityCombo = new ElaComboBox(this);
    m_renderQualityCombo->addItem(tr("Low (Faster)"), "low");
    m_renderQualityCombo->addItem(tr("Medium"), "medium");
    m_renderQualityCombo->addItem(tr("High"), "high");
    m_renderQualityCombo->addItem(tr("Very High (Best Quality)"), "veryHigh");
    m_renderQualityCombo->setCurrentIndex(2);
    qualityRow->addWidget(m_renderQualityCombo);
    qualityRow->addStretch();
    renderLayout->addLayout(qualityRow);

    auto* aaRow = new QHBoxLayout();
    auto* aaLabel = new ElaText(tr("Anti-aliasing"), this);
    aaRow->addWidget(aaLabel);
    aaRow->addStretch();
    m_antiAliasingSwitch = new ElaToggleSwitch(this);
    m_antiAliasingSwitch->setIsToggled(true);
    aaRow->addWidget(m_antiAliasingSwitch);
    renderLayout->addLayout(aaRow);

    layout->addWidget(renderArea);
    layout->addStretch();

    return widget;
}

void SettingsPage::connectSignals() {
    // Navigation
    connect(m_navigationList, &QListWidget::currentRowChanged, this,
            &SettingsPage::onSectionChanged);

    // Save button
    connect(m_saveBtn, &ElaPushButton::clicked, this,
            &SettingsPage::onSaveClicked);

    // Cancel button
    connect(m_cancelBtn, &ElaPushButton::clicked, this,
            &SettingsPage::onCancelClicked);

    // Reset button
    connect(m_resetBtn, &ElaPushButton::clicked, this,
            &SettingsPage::onResetClicked);

    // Theme change
    connect(m_themeCombo, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, [this](int index) {
                QString theme = m_themeCombo->itemData(index).toString();
                emit themeChanged(theme);
                m_hasUnsavedChanges = true;
            });

    // Language change
    connect(m_languageCombo,
            QOverload<int>::of(&ElaComboBox::currentIndexChanged), this,
            [this](int index) {
                QString language = m_languageCombo->itemData(index).toString();
                emit languageChanged(language);
                m_hasUnsavedChanges = true;
            });

    // Connect sub-widget signals
    if (m_documentWidget) {
        connect(m_documentWidget, &DocumentSettingsWidget::settingsChanged,
                this, &SettingsPage::onSettingsModified);
    }
    if (m_annotationWidget) {
        connect(m_annotationWidget, &AnnotationSettingsWidget::settingsChanged,
                this, &SettingsPage::onSettingsModified);
    }
    if (m_searchWidget) {
        connect(m_searchWidget, &SearchSettingsWidget::settingsChanged, this,
                &SettingsPage::onSettingsModified);
    }
    if (m_cacheWidget) {
        connect(m_cacheWidget, &CacheSettingsWidget::settingsChanged, this,
                &SettingsPage::onSettingsModified);
    }
    if (m_systemTrayWidget) {
        connect(m_systemTrayWidget, &SystemTraySettingsWidget::settingsChanged,
                this, &SettingsPage::onSettingsModified);
    }
    if (m_shortcutsWidget) {
        connect(m_shortcutsWidget, &ShortcutSettingsWidget::shortcutsChanged,
                this, &SettingsPage::onSettingsModified);
    }
    if (m_accessibilityWidget) {
        connect(m_accessibilityWidget,
                &AccessibilitySettingsWidget::settingsChanged, this,
                &SettingsPage::onSettingsModified);
    }
    if (m_pluginWidget) {
        connect(m_pluginWidget, &PluginSettingsWidget::settingsChanged, this,
                &SettingsPage::onSettingsModified);
    }
    if (m_loggingWidget) {
        connect(m_loggingWidget, &LoggingSettingsWidget::settingsChanged, this,
                &SettingsPage::onSettingsModified);
    }
}

void SettingsPage::onSectionChanged(int index) {
    m_contentStack->setCurrentIndex(index);
}

void SettingsPage::onSaveClicked() {
    saveSettings();
    applySettings();
    m_hasUnsavedChanges = false;
    emit settingsChanged();
}

void SettingsPage::onCancelClicked() {
    loadSettings();
    m_hasUnsavedChanges = false;
}

void SettingsPage::onResetClicked() { resetToDefaults(); }

void SettingsPage::onSettingsModified() { m_hasUnsavedChanges = true; }

void SettingsPage::navigateToSection(SettingsSection section) {
    m_navigationList->setCurrentRow(static_cast<int>(section));
}

// ============================================================================
// Settings Management
// ============================================================================

void SettingsPage::loadSettings() {
    SLOG_INFO("SettingsPage: Loading settings");

    QSettings settings("SAST", "Readium");

    // Appearance
    int themeIndex = m_themeCombo->findData(
        settings.value("appearance/theme", "light").toString());
    if (themeIndex >= 0) {
        m_themeCombo->setCurrentIndex(themeIndex);
    }

    QString language = settings.value("appearance/language", "en").toString();
    int langIndex = m_languageCombo->findData(language);
    if (langIndex >= 0) {
        m_languageCombo->setCurrentIndex(langIndex);
    }

    m_fontSizeSpin->setValue(settings.value("appearance/fontSize", 12).toInt());
    m_animationsSwitch->setIsToggled(
        settings.value("appearance/animations", true).toBool());

    // Viewer
    int zoomIndex = m_defaultZoomCombo->findData(
        settings.value("viewer/defaultZoom", "fitWidth").toString());
    if (zoomIndex >= 0) {
        m_defaultZoomCombo->setCurrentIndex(zoomIndex);
    }

    int modeIndex = m_defaultViewModeCombo->findData(
        settings.value("viewer/defaultViewMode", "continuous").toString());
    if (modeIndex >= 0) {
        m_defaultViewModeCombo->setCurrentIndex(modeIndex);
    }

    m_rememberLastPageSwitch->setIsToggled(
        settings.value("viewer/rememberLastPage", true).toBool());
    m_smoothScrollSwitch->setIsToggled(
        settings.value("viewer/smoothScroll", true).toBool());

    int qualityIndex = m_renderQualityCombo->findData(
        settings.value("viewer/renderQuality", "high").toString());
    if (qualityIndex >= 0) {
        m_renderQualityCombo->setCurrentIndex(qualityIndex);
    }

    m_antiAliasingSwitch->setIsToggled(
        settings.value("viewer/antiAliasing", true).toBool());

    // Load sub-widget settings
    if (m_documentWidget)
        m_documentWidget->loadSettings();
    if (m_annotationWidget)
        m_annotationWidget->loadSettings();
    if (m_searchWidget)
        m_searchWidget->loadSettings();
    if (m_cacheWidget)
        m_cacheWidget->loadSettings();
    if (m_systemTrayWidget)
        m_systemTrayWidget->loadSettings();
    if (m_shortcutsWidget)
        m_shortcutsWidget->loadShortcuts();
    if (m_accessibilityWidget)
        m_accessibilityWidget->loadSettings();
    if (m_pluginWidget)
        m_pluginWidget->loadSettings();
    if (m_loggingWidget)
        m_loggingWidget->loadSettings();
}

void SettingsPage::saveSettings() {
    SLOG_INFO("SettingsPage: Saving settings");

    QSettings settings("SAST", "Readium");

    // Appearance
    settings.setValue("appearance/theme",
                      m_themeCombo->currentData().toString());
    settings.setValue("appearance/language",
                      m_languageCombo->currentData().toString());
    settings.setValue("appearance/fontSize", m_fontSizeSpin->value());
    settings.setValue("appearance/animations",
                      m_animationsSwitch->getIsToggled());

    // Viewer
    settings.setValue("viewer/defaultZoom",
                      m_defaultZoomCombo->currentData().toString());
    settings.setValue("viewer/defaultViewMode",
                      m_defaultViewModeCombo->currentData().toString());
    settings.setValue("viewer/rememberLastPage",
                      m_rememberLastPageSwitch->getIsToggled());
    settings.setValue("viewer/smoothScroll",
                      m_smoothScrollSwitch->getIsToggled());
    settings.setValue("viewer/renderQuality",
                      m_renderQualityCombo->currentData().toString());
    settings.setValue("viewer/antiAliasing",
                      m_antiAliasingSwitch->getIsToggled());

    // Save sub-widget settings
    if (m_documentWidget)
        m_documentWidget->saveSettings();
    if (m_annotationWidget)
        m_annotationWidget->saveSettings();
    if (m_searchWidget)
        m_searchWidget->saveSettings();
    if (m_cacheWidget)
        m_cacheWidget->saveSettings();
    if (m_systemTrayWidget)
        m_systemTrayWidget->saveSettings();
    if (m_shortcutsWidget)
        m_shortcutsWidget->saveShortcuts();
    if (m_accessibilityWidget)
        m_accessibilityWidget->saveSettings();
    if (m_pluginWidget)
        m_pluginWidget->saveSettings();
    if (m_loggingWidget)
        m_loggingWidget->saveSettings();
}

void SettingsPage::resetToDefaults() {
    SLOG_INFO("SettingsPage: Resetting to defaults");

    // Appearance
    m_themeCombo->setCurrentIndex(0);
    m_languageCombo->setCurrentIndex(0);
    m_fontSizeSpin->setValue(12);
    m_animationsSwitch->setIsToggled(true);

    // Viewer
    m_defaultZoomCombo->setCurrentIndex(0);
    m_defaultViewModeCombo->setCurrentIndex(1);
    m_rememberLastPageSwitch->setIsToggled(true);
    m_smoothScrollSwitch->setIsToggled(true);
    m_renderQualityCombo->setCurrentIndex(2);
    m_antiAliasingSwitch->setIsToggled(true);

    // Reset sub-widgets
    if (m_documentWidget)
        m_documentWidget->resetToDefaults();
    if (m_annotationWidget)
        m_annotationWidget->resetToDefaults();
    if (m_searchWidget)
        m_searchWidget->resetToDefaults();
    if (m_cacheWidget)
        m_cacheWidget->resetToDefaults();
    if (m_systemTrayWidget)
        m_systemTrayWidget->resetToDefaults();
    if (m_shortcutsWidget)
        m_shortcutsWidget->resetToDefaults();
    if (m_accessibilityWidget)
        m_accessibilityWidget->resetToDefaults();
    if (m_pluginWidget)
        m_pluginWidget->resetToDefaults();
    if (m_loggingWidget)
        m_loggingWidget->resetToDefaults();
}

void SettingsPage::applySettings() {
    SLOG_INFO("SettingsPage: Applying settings");

    // Apply theme
    if (m_styleManager != nullptr) {
        QString themeStr = m_themeCombo->currentData().toString();
        Theme theme = Theme::Light;
        if (themeStr == "dark") {
            theme = Theme::Dark;
        }
        m_styleManager->setTheme(theme);
    }

    // Apply language
    if (m_i18nManager != nullptr) {
        QString languageCode = m_languageCombo->currentData().toString();
        m_i18nManager->loadLanguage(languageCode);
    }
}

// ============================================================================
// Business Logic Integration
// ============================================================================

void SettingsPage::setI18nManager(I18nManager* manager) {
    m_i18nManager = manager;
}

void SettingsPage::setStyleManager(StyleManager* manager) {
    m_styleManager = manager;
}

// ============================================================================
// Event Handling
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

    // Update navigation items
    if (m_navigationList && m_navigationList->count() >= 11) {
        m_navigationList->item(0)->setText(tr("Appearance"));
        m_navigationList->item(1)->setText(tr("Viewer"));
        m_navigationList->item(2)->setText(tr("Document"));
        m_navigationList->item(3)->setText(tr("Annotations"));
        m_navigationList->item(4)->setText(tr("Search"));
        m_navigationList->item(5)->setText(tr("Cache"));
        m_navigationList->item(6)->setText(tr("System Tray"));
        m_navigationList->item(7)->setText(tr("Shortcuts"));
        m_navigationList->item(8)->setText(tr("Accessibility"));
        m_navigationList->item(9)->setText(tr("Plugins"));
        m_navigationList->item(10)->setText(tr("Logging"));
    }

    // Update combo box items
    if (m_themeCombo && m_themeCombo->count() >= 3) {
        m_themeCombo->setItemText(0, tr("Light"));
        m_themeCombo->setItemText(1, tr("Dark"));
        m_themeCombo->setItemText(2, tr("Auto"));
    }

    if (m_languageCombo) {
        int englishIndex = m_languageCombo->findData("en");
        if (englishIndex >= 0) {
            m_languageCombo->setItemText(englishIndex, tr("English"));
        }
        int chineseIndex = m_languageCombo->findData("zh_CN");
        if (chineseIndex >= 0) {
            m_languageCombo->setItemText(chineseIndex, tr("中文"));
        }
    }

    // Update buttons
    if (m_saveBtn)
        m_saveBtn->setText(tr("Save"));
    if (m_cancelBtn)
        m_cancelBtn->setText(tr("Cancel"));
    if (m_resetBtn)
        m_resetBtn->setText(tr("Reset to Defaults"));
}
