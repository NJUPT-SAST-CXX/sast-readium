#pragma once

#include <QWidget>

class QVBoxLayout;
class ElaToggleSwitch;
class ElaSpinBox;
class ElaComboBox;
class ElaText;

class DocumentSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit DocumentSettingsWidget(QWidget* parent = nullptr);
    ~DocumentSettingsWidget() override;

    void loadSettings();
    void saveSettings();
    void resetToDefaults();

signals:
    void settingsChanged();

protected:
    void changeEvent(QEvent* event) override;

private:
    void setupUi();
    void retranslateUi();
    void updateControlsState();

    QVBoxLayout* m_mainLayout;

    // Recent files
    ElaSpinBox* m_maxRecentFilesSpin;
    ElaToggleSwitch* m_autoCleanupSwitch;
    ElaToggleSwitch* m_showRecentOnStartSwitch;

    // Opening behavior
    ElaToggleSwitch* m_rememberPositionSwitch;
    ElaToggleSwitch* m_rememberZoomSwitch;
    ElaComboBox* m_defaultOpenActionCombo;

    // First run
    ElaToggleSwitch* m_showOnboardingSwitch;
    ElaToggleSwitch* m_showTipsSwitch;

    // Auto-save
    ElaToggleSwitch* m_autoSaveStateSwitch;
    ElaSpinBox* m_autoSaveIntervalSpin;

    // File handling
    ElaToggleSwitch* m_confirmCloseSwitch;
    ElaToggleSwitch* m_reloadModifiedSwitch;
};
