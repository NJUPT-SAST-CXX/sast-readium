#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QString>
#include "ElaScrollPage.h"

// Forward declarations
class ElaComboBox;
class ElaCheckBox;
class ElaSlider;
class ElaPushButton;
class ElaLineEdit;
class QGroupBox;
class QFormLayout;
class QLabel;
class ElaText;
class I18nManager;
class StyleManager;

/**
 * @brief SettingsPage - 设置页面
 *
 * 提供应用程序设置：
 * - 外观设置（主题、语言）
 * - 查看器设置（默认缩放、视图模式）
 * - 性能设置（缓存大小、渲染质量）
 * - 高级设置（快捷键、插件）
 * Inherits from ElaScrollPage following ElaWidgetTools example pattern
 */
class SettingsPage : public ElaScrollPage {
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);
    ~SettingsPage() override;

    // 设置管理
    void loadSettings();
    void saveSettings();
    void resetToDefaults();

    // 业务逻辑集成
    void setI18nManager(I18nManager* manager);
    void setStyleManager(StyleManager* manager);

signals:
    void settingsChanged();
    void themeChanged(const QString& theme);
    void languageChanged(const QString& language);

protected:
    void changeEvent(QEvent* event) override;

private:
    // 外观设置
    ElaComboBox* m_themeCombo;
    ElaComboBox* m_languageCombo;

    // 查看器设置
    ElaComboBox* m_defaultZoomCombo;
    ElaComboBox* m_defaultViewModeCombo;
    ElaCheckBox* m_rememberLastPageCheck;
    ElaCheckBox* m_smoothScrollCheck;

    // 性能设置
    ElaSlider* m_cacheSizeSlider;
    ElaComboBox* m_renderQualityCombo;
    ElaCheckBox* m_hardwareAccelCheck;

    // 高级设置
    ElaCheckBox* m_autoSaveCheck;
    ElaLineEdit* m_autoSaveIntervalEdit;
    ElaPushButton* m_resetShortcutsBtn;
    ElaPushButton* m_managePluginsBtn;

    // 按钮
    ElaPushButton* m_saveBtn;
    ElaPushButton* m_cancelBtn;
    ElaPushButton* m_resetBtn;

    // 业务逻辑
    I18nManager* m_i18nManager;
    StyleManager* m_styleManager;

    // 分组引用（用于重翻译）
    QGroupBox* m_appearanceGroup{nullptr};
    QFormLayout* m_appearanceFormLayout{nullptr};
    QGroupBox* m_viewerGroup{nullptr};
    QFormLayout* m_viewerFormLayout{nullptr};
    QGroupBox* m_performanceGroup{nullptr};
    QFormLayout* m_performanceFormLayout{nullptr};
    QGroupBox* m_advancedGroup{nullptr};
    QFormLayout* m_advancedFormLayout{nullptr};
    ElaText* m_autoSaveIntervalLabel{nullptr};
    ElaText* m_autoSaveIntervalUnitLabel{nullptr};

    void setupUi();
    void setupAppearanceSection();
    void setupViewerSection();
    void setupPerformanceSection();
    void setupAdvancedSection();
    void setupButtons();
    void connectSignals();
    void retranslateUi();
    void applySettings();

    QGroupBox* createAppearanceGroup();
    QGroupBox* createViewerGroup();
    QGroupBox* createPerformanceGroup();
    QGroupBox* createAdvancedGroup();

    void updateAppearanceTexts();
    void updateViewerTexts();
    void updatePerformanceTexts();
    void updateAdvancedTexts();
    void updateButtonTexts();
};

#endif  // SETTINGSPAGE_H
