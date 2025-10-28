#pragma once

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTabWidget>
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
    QTabWidget* m_tabWidget;
    QDialogButtonBox* m_buttonBox;
    QPushButton* m_applyButton;
    QPushButton* m_restoreDefaultsButton;

    // Appearance tab
    QWidget* m_appearanceTab;
    QButtonGroup* m_themeGroup;
    QRadioButton* m_lightThemeRadio;
    QRadioButton* m_darkThemeRadio;
    QComboBox* m_languageCombo;

    // Performance tab
    QWidget* m_performanceTab;
    QSpinBox* m_cacheSizeSpinBox;
    QCheckBox* m_enableCacheCheckBox;
    QCheckBox* m_preloadPagesCheckBox;
    QSpinBox* m_preloadCountSpinBox;
    QComboBox* m_renderQualityCombo;

    // Behavior tab
    QWidget* m_behaviorTab;
    QComboBox* m_defaultZoomCombo;
    QComboBox* m_defaultPageModeCombo;
    QSpinBox* m_recentFilesCountSpinBox;
    QCheckBox* m_rememberWindowStateCheckBox;
    QCheckBox* m_openLastFileCheckBox;

    // Advanced tab
    QWidget* m_advancedTab;
    QComboBox* m_logLevelCombo;
    QCheckBox* m_enableDebugPanelCheckBox;
    QCheckBox* m_showWelcomeScreenCheckBox;
    QLineEdit* m_customCachePathEdit;
    QPushButton* m_browseCachePathButton;
    QPushButton* m_clearCacheButton;
};
