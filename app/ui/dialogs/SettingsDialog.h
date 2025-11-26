#pragma once

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>

#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

/**
 * @brief Application settings dialog
 *
 * @details Provides a comprehensive settings interface for:
 * - Appearance (theme, language)
 * - Performance (cache settings, rendering options)
 * - Behavior (default zoom, page mode, recent files)
 * - Advanced (logging, debug options)
 */
class ElaComboBox;
class ElaCheckBox;
class ElaRadioButton;
class ElaSpinBox;
class ElaPushButton;
class ElaTabWidget;
class ElaLineEdit;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() = default;

signals:
    /**
     * @brief Emitted when settings are applied
     */
    void settingsApplied();

    /**
     * @brief Emitted when theme is changed
     * @param theme Theme name ("light" or "dark")
     */
    void themeChanged(const QString& theme);

    /**
     * @brief Emitted when language is changed
     * @param languageCode Language code ("en" or "zh")
     */
    void languageChanged(const QString& languageCode);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onApplyClicked();
    void onOkClicked();
    void onCancelClicked();
    void onRestoreDefaultsClicked();
    void validateCacheSize(int value);
    void validateRecentFilesCount(int value);
    void validateCachePath(const QString& path);
    void previewTheme(int themeId);
    void previewLanguage(int languageIndex);

private:
    void setupUI();
    void setupConnections();
    void retranslateUi();
    void loadSettings();
    void saveSettings();
    void applySettings();
    void restoreDefaults();

    // Tab creation methods
    QWidget* createAppearanceTab();
    QWidget* createPerformanceTab();
    QWidget* createBehaviorTab();
    QWidget* createAdvancedTab();

    // UI Components
    QVBoxLayout* m_mainLayout;
    ElaTabWidget* m_tabWidget;
    QDialogButtonBox* m_buttonBox;
    QPushButton* m_applyButton;
    ElaPushButton* m_restoreDefaultsButton;

    // Appearance tab
    QWidget* m_appearanceTab;
    QButtonGroup* m_themeGroup;
    ElaRadioButton* m_lightThemeRadio;
    ElaRadioButton* m_darkThemeRadio;
    ElaComboBox* m_languageCombo;

    // Performance tab
    QWidget* m_performanceTab;
    ElaSpinBox* m_cacheSizeSpinBox;
    ElaCheckBox* m_enableCacheCheckBox;
    ElaCheckBox* m_preloadPagesCheckBox;
    ElaSpinBox* m_preloadCountSpinBox;
    ElaComboBox* m_renderQualityCombo;

    // Behavior tab
    QWidget* m_behaviorTab;
    ElaComboBox* m_defaultZoomCombo;
    ElaComboBox* m_defaultPageModeCombo;
    ElaSpinBox* m_recentFilesCountSpinBox;
    ElaCheckBox* m_rememberWindowStateCheckBox;
    ElaCheckBox* m_openLastFileCheckBox;

    // Advanced tab
    QWidget* m_advancedTab;
    ElaComboBox* m_logLevelCombo;
    ElaCheckBox* m_enableDebugPanelCheckBox;
    ElaCheckBox* m_showWelcomeScreenCheckBox;
    ElaLineEdit* m_customCachePathEdit;
    ElaPushButton* m_browseCachePathButton;
    ElaPushButton* m_clearCacheButton;
};
