#pragma once

#include <QWidget>

// Forward declarations
class QVBoxLayout;
class ElaToggleSwitch;
class ElaSpinBox;
class ElaComboBox;
class ElaText;

/**
 * @brief Widget for system tray settings configuration
 *
 * Provides UI for configuring system tray behavior including:
 * - Enable/disable system tray
 * - Minimize to tray behavior
 * - Notification settings
 * - Recent files in tray menu
 */
class SystemTraySettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit SystemTraySettingsWidget(QWidget* parent = nullptr);
    ~SystemTraySettingsWidget() override;

    void loadSettings();
    void saveSettings();
    void resetToDefaults();

signals:
    void settingsChanged();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onTrayEnabledToggled(bool enabled);
    void onMinimizeToTrayToggled(bool enabled);
    void onNotificationsToggled(bool enabled);

private:
    void setupUi();
    void retranslateUi();
    void updateControlsState();

    // UI Components
    QVBoxLayout* m_mainLayout;

    // Basic settings
    ElaToggleSwitch* m_enableTraySwitch;
    ElaToggleSwitch* m_minimizeToTraySwitch;
    ElaToggleSwitch* m_closeToTraySwitch;
    ElaToggleSwitch* m_startMinimizedSwitch;

    // Notifications
    ElaToggleSwitch* m_showNotificationsSwitch;
    ElaToggleSwitch* m_enhancedNotificationsSwitch;
    ElaComboBox* m_notificationTypesCombo;

    // Recent files
    ElaToggleSwitch* m_showRecentFilesSwitch;
    ElaSpinBox* m_recentFilesCountSpin;

    // Quick actions
    ElaToggleSwitch* m_showQuickActionsSwitch;
    ElaToggleSwitch* m_showStatusIndicatorsSwitch;
    ElaToggleSwitch* m_dynamicTooltipSwitch;
};
