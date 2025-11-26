#pragma once

#include <QWidget>

// Forward declarations
class QVBoxLayout;
class ElaToggleSwitch;
class ElaComboBox;
class ElaSpinBox;
class ElaLineEdit;
class ElaPushButton;
class ElaText;

/**
 * @brief Widget for logging settings configuration
 *
 * Provides UI for configuring logging behavior including:
 * - Log level
 * - File logging
 * - Console logging
 * - Performance logging
 */
class LoggingSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit LoggingSettingsWidget(QWidget* parent = nullptr);
    ~LoggingSettingsWidget() override;

    void loadSettings();
    void saveSettings();
    void resetToDefaults();

signals:
    void settingsChanged();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onFileLoggingToggled(bool enabled);
    void onBrowseLogPath();
    void onOpenLogFolder();
    void onClearLogs();

private:
    void setupUi();
    void retranslateUi();
    void updateControlsState();

    // UI Components
    QVBoxLayout* m_mainLayout;

    // Global settings
    ElaComboBox* m_globalLevelCombo;
    ElaToggleSwitch* m_asyncLoggingSwitch;
    ElaSpinBox* m_flushIntervalSpin;

    // Console logging
    ElaToggleSwitch* m_consoleLoggingSwitch;
    ElaToggleSwitch* m_coloredOutputSwitch;

    // File logging
    ElaToggleSwitch* m_fileLoggingSwitch;
    ElaLineEdit* m_logPathEdit;
    ElaPushButton* m_browsePathBtn;
    ElaSpinBox* m_maxFileSizeSpin;
    ElaSpinBox* m_maxFilesSpin;
    ElaToggleSwitch* m_rotateOnStartupSwitch;

    // Performance logging
    ElaToggleSwitch* m_perfLoggingSwitch;
    ElaSpinBox* m_perfThresholdSpin;

    // Debug
    ElaToggleSwitch* m_memoryLoggingSwitch;
    ElaToggleSwitch* m_threadIdSwitch;
    ElaToggleSwitch* m_sourceLocationSwitch;

    // Actions
    ElaPushButton* m_openLogFolderBtn;
    ElaPushButton* m_clearLogsBtn;
};
