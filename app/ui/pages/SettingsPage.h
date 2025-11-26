#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QString>
#include "ElaScrollPage.h"

// Forward declarations
class QStackedWidget;
class QListWidget;
class ElaComboBox;
class ElaSlider;
class ElaPushButton;
class ElaLineEdit;
class ElaText;
class ElaToggleSwitch;
class ElaSpinBox;
class I18nManager;
class StyleManager;
class ConfigurationManager;

// Settings widgets
class ShortcutSettingsWidget;
class AccessibilitySettingsWidget;
class SystemTraySettingsWidget;
class LoggingSettingsWidget;
class CacheSettingsWidget;
class AnnotationSettingsWidget;
class SearchSettingsWidget;
class PluginSettingsWidget;
class DocumentSettingsWidget;

/**
 * @brief SettingsPage - Comprehensive settings page
 *
 * Provides a modern settings interface with multiple sections:
 * - Appearance (theme, language, font)
 * - Viewer (zoom, view mode, scrolling)
 * - Cache & Performance
 * - System Tray
 * - Keyboard Shortcuts
 * - Accessibility
 * - Logging & Debug
 *
 * Uses ElaWidgetTools components for consistent styling.
 * Inherits from ElaScrollPage following ElaWidgetTools example pattern.
 */
class SettingsPage : public ElaScrollPage {
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);
    ~SettingsPage() override;

    // Settings management
    void loadSettings();
    void saveSettings();
    void resetToDefaults();

    // Business logic integration
    void setI18nManager(I18nManager* manager);
    void setStyleManager(StyleManager* manager);

    // Section navigation
    enum class SettingsSection {
        Appearance,
        Viewer,
        Document,
        Annotations,
        Search,
        Cache,
        SystemTray,
        Shortcuts,
        Accessibility,
        Plugins,
        Logging
    };
    void navigateToSection(SettingsSection section);

signals:
    void settingsChanged();
    void themeChanged(const QString& theme);
    void languageChanged(const QString& language);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onSectionChanged(int index);
    void onSaveClicked();
    void onCancelClicked();
    void onResetClicked();
    void onSettingsModified();

private:
    void setupUi();
    void setupNavigation();
    void setupSections();
    void setupButtons();
    void connectSignals();
    void retranslateUi();
    void applySettings();

    // Create section widgets
    QWidget* createAppearanceSection();
    QWidget* createViewerSection();

    // Navigation
    QListWidget* m_navigationList;
    QStackedWidget* m_contentStack;

    // Section widgets
    QWidget* m_appearanceWidget;
    QWidget* m_viewerWidget;
    DocumentSettingsWidget* m_documentWidget;
    AnnotationSettingsWidget* m_annotationWidget;
    SearchSettingsWidget* m_searchWidget;
    CacheSettingsWidget* m_cacheWidget;
    SystemTraySettingsWidget* m_systemTrayWidget;
    ShortcutSettingsWidget* m_shortcutsWidget;
    AccessibilitySettingsWidget* m_accessibilityWidget;
    PluginSettingsWidget* m_pluginWidget;
    LoggingSettingsWidget* m_loggingWidget;

    // Appearance section controls
    ElaComboBox* m_themeCombo;
    ElaComboBox* m_languageCombo;
    ElaSpinBox* m_fontSizeSpin;
    ElaToggleSwitch* m_animationsSwitch;

    // Viewer section controls
    ElaComboBox* m_defaultZoomCombo;
    ElaComboBox* m_defaultViewModeCombo;
    ElaToggleSwitch* m_rememberLastPageSwitch;
    ElaToggleSwitch* m_smoothScrollSwitch;
    ElaComboBox* m_renderQualityCombo;
    ElaToggleSwitch* m_antiAliasingSwitch;

    // Action buttons
    ElaPushButton* m_saveBtn;
    ElaPushButton* m_cancelBtn;
    ElaPushButton* m_resetBtn;

    // Business logic references
    I18nManager* m_i18nManager;
    StyleManager* m_styleManager;

    // State tracking
    bool m_hasUnsavedChanges;
};

#endif  // SETTINGSPAGE_H
