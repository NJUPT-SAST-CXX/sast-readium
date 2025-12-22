#pragma once

#include <QDialog>
#include <QPointer>
#include <memory>

// Forward declarations
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class ElaText;
class ElaLineEdit;
class ElaPushButton;
class ElaCheckBox;
class ElaMessageBar;
class PluginConfigWidget;
class PluginConfigModel;
class PluginManager;

/**
 * @brief PluginConfigDialog - Complete plugin configuration dialog
 *
 * This dialog provides a full interface for viewing and editing
 * plugin configuration settings. Features:
 * - Plugin information header (name, version, description)
 * - Grouped configuration editor (using PluginConfigWidget)
 * - Action buttons (Save, Reset, Import, Export)
 * - Validation and error display
 * - Advanced settings toggle
 */
class PluginConfigDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param pluginName Name of the plugin to configure
     * @param parent Parent widget
     */
    explicit PluginConfigDialog(const QString& pluginName,
                                QWidget* parent = nullptr);
    ~PluginConfigDialog() override;

    /**
     * @brief Get the plugin name
     * @return Plugin name
     */
    QString pluginName() const { return m_pluginName; }

    /**
     * @brief Check if configuration was modified
     * @return True if configuration was changed
     */
    bool isModified() const;

    /**
     * @brief Static helper to show configuration dialog
     * @param pluginName Plugin name
     * @param parent Parent widget
     * @return True if dialog was accepted and configuration saved
     */
    static bool showConfigDialog(const QString& pluginName,
                                 QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private slots:
    void onSaveClicked();
    void onResetClicked();
    void onImportClicked();
    void onExportClicked();
    void onShowAdvancedToggled(bool checked);
    void onConfigurationChanged();
    void onValidationStateChanged(bool isValid);

private:
    void setupUI();
    void setupHeader();
    void setupConfigWidget();
    void setupButtons();
    void retranslateUI();
    void loadPluginInfo();
    void updateValidationDisplay();
    bool confirmUnsavedChanges();

    // Plugin info
    QString m_pluginName;
    QPointer<PluginManager> m_pluginManager;

    // Model
    std::unique_ptr<PluginConfigModel> m_configModel;

    // UI Components - Header
    QWidget* m_headerWidget;
    ElaText* m_pluginNameLabel;
    ElaText* m_pluginVersionLabel;
    ElaText* m_pluginDescriptionLabel;

    // UI Components - Config
    ElaLineEdit* m_searchEdit;
    PluginConfigWidget* m_configWidget;
    ElaCheckBox* m_showAdvancedCheck;

    // UI Components - Buttons
    QWidget* m_buttonWidget;
    ElaPushButton* m_saveBtn;
    ElaPushButton* m_resetBtn;
    ElaPushButton* m_importBtn;
    ElaPushButton* m_exportBtn;
    ElaPushButton* m_cancelBtn;

    // UI Components - Validation
    ElaText* m_validationLabel;

    // State
    bool m_hasUnsavedChanges;
};
