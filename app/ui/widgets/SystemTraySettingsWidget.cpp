#include "SystemTraySettingsWidget.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QSettings>
#include <QVBoxLayout>

#include "ElaComboBox.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ElaToggleSwitch.h"

#include "logging/SimpleLogging.h"
#include "managers/SystemTrayManager.h"

SystemTraySettingsWidget::SystemTraySettingsWidget(QWidget* parent)
    : QWidget(parent),
      m_mainLayout(nullptr),
      m_enableTraySwitch(nullptr),
      m_minimizeToTraySwitch(nullptr),
      m_closeToTraySwitch(nullptr),
      m_startMinimizedSwitch(nullptr),
      m_showNotificationsSwitch(nullptr),
      m_enhancedNotificationsSwitch(nullptr),
      m_notificationTypesCombo(nullptr),
      m_showRecentFilesSwitch(nullptr),
      m_recentFilesCountSpin(nullptr),
      m_showQuickActionsSwitch(nullptr),
      m_showStatusIndicatorsSwitch(nullptr),
      m_dynamicTooltipSwitch(nullptr) {
    setupUi();
    loadSettings();
}

SystemTraySettingsWidget::~SystemTraySettingsWidget() = default;

void SystemTraySettingsWidget::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(16);

    // Basic Settings Section
    auto* basicArea = new ElaScrollPageArea(this);
    auto* basicLayout = new QVBoxLayout(basicArea);
    basicLayout->setContentsMargins(16, 12, 16, 12);

    auto* basicTitle = new ElaText(tr("Basic Settings"), this);
    basicTitle->setTextPixelSize(14);
    basicLayout->addWidget(basicTitle);

    auto* enableRow = new QHBoxLayout();
    auto* enableLabel = new ElaText(tr("Enable system tray icon"), this);
    enableRow->addWidget(enableLabel);
    enableRow->addStretch();
    m_enableTraySwitch = new ElaToggleSwitch(this);
    enableRow->addWidget(m_enableTraySwitch);
    basicLayout->addLayout(enableRow);

    auto* minimizeRow = new QHBoxLayout();
    auto* minimizeLabel =
        new ElaText(tr("Minimize to tray instead of taskbar"), this);
    minimizeRow->addWidget(minimizeLabel);
    minimizeRow->addStretch();
    m_minimizeToTraySwitch = new ElaToggleSwitch(this);
    minimizeRow->addWidget(m_minimizeToTraySwitch);
    basicLayout->addLayout(minimizeRow);

    auto* closeRow = new QHBoxLayout();
    auto* closeLabel = new ElaText(tr("Close to tray instead of exit"), this);
    closeRow->addWidget(closeLabel);
    closeRow->addStretch();
    m_closeToTraySwitch = new ElaToggleSwitch(this);
    closeRow->addWidget(m_closeToTraySwitch);
    basicLayout->addLayout(closeRow);

    auto* startRow = new QHBoxLayout();
    auto* startLabel = new ElaText(tr("Start minimized to tray"), this);
    startRow->addWidget(startLabel);
    startRow->addStretch();
    m_startMinimizedSwitch = new ElaToggleSwitch(this);
    startRow->addWidget(m_startMinimizedSwitch);
    basicLayout->addLayout(startRow);

    m_mainLayout->addWidget(basicArea);

    // Notifications Section
    auto* notifyArea = new ElaScrollPageArea(this);
    auto* notifyLayout = new QVBoxLayout(notifyArea);
    notifyLayout->setContentsMargins(16, 12, 16, 12);

    auto* notifyTitle = new ElaText(tr("Notifications"), this);
    notifyTitle->setTextPixelSize(14);
    notifyLayout->addWidget(notifyTitle);

    auto* showNotifyRow = new QHBoxLayout();
    auto* showNotifyLabel = new ElaText(tr("Show tray notifications"), this);
    showNotifyRow->addWidget(showNotifyLabel);
    showNotifyRow->addStretch();
    m_showNotificationsSwitch = new ElaToggleSwitch(this);
    showNotifyRow->addWidget(m_showNotificationsSwitch);
    notifyLayout->addLayout(showNotifyRow);

    auto* enhancedRow = new QHBoxLayout();
    auto* enhancedLabel = new ElaText(tr("Enhanced notifications"), this);
    enhancedRow->addWidget(enhancedLabel);
    enhancedRow->addStretch();
    m_enhancedNotificationsSwitch = new ElaToggleSwitch(this);
    enhancedRow->addWidget(m_enhancedNotificationsSwitch);
    notifyLayout->addLayout(enhancedRow);

    auto* typesRow = new QHBoxLayout();
    auto* typesLabel = new ElaText(tr("Notification types:"), this);
    typesRow->addWidget(typesLabel);
    m_notificationTypesCombo = new ElaComboBox(this);
    m_notificationTypesCombo->addItem(tr("All"), "all");
    m_notificationTypesCombo->addItem(tr("Document events only"), "document");
    m_notificationTypesCombo->addItem(tr("Errors only"), "error");
    m_notificationTypesCombo->addItem(tr("Status changes only"), "status");
    typesRow->addWidget(m_notificationTypesCombo);
    typesRow->addStretch();
    notifyLayout->addLayout(typesRow);

    m_mainLayout->addWidget(notifyArea);

    // Tray Menu Section
    auto* menuArea = new ElaScrollPageArea(this);
    auto* menuLayout = new QVBoxLayout(menuArea);
    menuLayout->setContentsMargins(16, 12, 16, 12);

    auto* menuTitle = new ElaText(tr("Tray Menu"), this);
    menuTitle->setTextPixelSize(14);
    menuLayout->addWidget(menuTitle);

    auto* recentRow = new QHBoxLayout();
    auto* recentLabel = new ElaText(tr("Show recent files"), this);
    recentRow->addWidget(recentLabel);
    recentRow->addStretch();
    m_showRecentFilesSwitch = new ElaToggleSwitch(this);
    recentRow->addWidget(m_showRecentFilesSwitch);
    menuLayout->addLayout(recentRow);

    auto* countRow = new QHBoxLayout();
    auto* countLabel = new ElaText(tr("Recent files count:"), this);
    countRow->addWidget(countLabel);
    m_recentFilesCountSpin = new ElaSpinBox(this);
    m_recentFilesCountSpin->setRange(3, 15);
    m_recentFilesCountSpin->setValue(5);
    countRow->addWidget(m_recentFilesCountSpin);
    countRow->addStretch();
    menuLayout->addLayout(countRow);

    auto* quickRow = new QHBoxLayout();
    auto* quickLabel = new ElaText(tr("Show quick actions"), this);
    quickRow->addWidget(quickLabel);
    quickRow->addStretch();
    m_showQuickActionsSwitch = new ElaToggleSwitch(this);
    quickRow->addWidget(m_showQuickActionsSwitch);
    menuLayout->addLayout(quickRow);

    m_mainLayout->addWidget(menuArea);

    // Visual Section
    auto* visualArea = new ElaScrollPageArea(this);
    auto* visualLayout = new QVBoxLayout(visualArea);
    visualLayout->setContentsMargins(16, 12, 16, 12);

    auto* visualTitle = new ElaText(tr("Visual"), this);
    visualTitle->setTextPixelSize(14);
    visualLayout->addWidget(visualTitle);

    auto* statusRow = new QHBoxLayout();
    auto* statusLabel = new ElaText(tr("Show status indicators"), this);
    statusRow->addWidget(statusLabel);
    statusRow->addStretch();
    m_showStatusIndicatorsSwitch = new ElaToggleSwitch(this);
    statusRow->addWidget(m_showStatusIndicatorsSwitch);
    visualLayout->addLayout(statusRow);

    auto* tooltipRow = new QHBoxLayout();
    auto* tooltipLabel = new ElaText(tr("Dynamic tooltip"), this);
    tooltipRow->addWidget(tooltipLabel);
    tooltipRow->addStretch();
    m_dynamicTooltipSwitch = new ElaToggleSwitch(this);
    tooltipRow->addWidget(m_dynamicTooltipSwitch);
    visualLayout->addLayout(tooltipRow);

    m_mainLayout->addWidget(visualArea);
    m_mainLayout->addStretch();

    // Connect signals
    connect(m_enableTraySwitch, &ElaToggleSwitch::toggled, this,
            &SystemTraySettingsWidget::onTrayEnabledToggled);
    connect(m_minimizeToTraySwitch, &ElaToggleSwitch::toggled, this,
            &SystemTraySettingsWidget::onMinimizeToTrayToggled);
    connect(m_showNotificationsSwitch, &ElaToggleSwitch::toggled, this,
            &SystemTraySettingsWidget::onNotificationsToggled);
}

void SystemTraySettingsWidget::loadSettings() {
    QSettings settings("SAST", "Readium");
    settings.beginGroup("UI");

    m_enableTraySwitch->setIsToggled(
        settings.value("system_tray_enabled", true).toBool());
    m_minimizeToTraySwitch->setIsToggled(
        settings.value("minimize_to_tray", true).toBool());
    m_closeToTraySwitch->setIsToggled(
        settings.value("close_to_tray", false).toBool());
    m_startMinimizedSwitch->setIsToggled(
        settings.value("start_minimized", false).toBool());
    m_showNotificationsSwitch->setIsToggled(
        settings.value("show_tray_notifications", true).toBool());
    m_enhancedNotificationsSwitch->setIsToggled(
        settings.value("enhanced_notifications", true).toBool());
    m_showRecentFilesSwitch->setIsToggled(
        settings.value("show_recent_files", true).toBool());
    m_recentFilesCountSpin->setValue(
        settings.value("recent_files_count", 5).toInt());
    m_showQuickActionsSwitch->setIsToggled(
        settings.value("show_quick_actions", true).toBool());
    m_showStatusIndicatorsSwitch->setIsToggled(
        settings.value("show_status_indicators", true).toBool());
    m_dynamicTooltipSwitch->setIsToggled(
        settings.value("dynamic_tooltip", true).toBool());

    settings.endGroup();
    updateControlsState();
}

void SystemTraySettingsWidget::saveSettings() {
    QSettings settings("SAST", "Readium");
    settings.beginGroup("UI");

    settings.setValue("system_tray_enabled",
                      m_enableTraySwitch->getIsToggled());
    settings.setValue("minimize_to_tray",
                      m_minimizeToTraySwitch->getIsToggled());
    settings.setValue("close_to_tray", m_closeToTraySwitch->getIsToggled());
    settings.setValue("start_minimized",
                      m_startMinimizedSwitch->getIsToggled());
    settings.setValue("show_tray_notifications",
                      m_showNotificationsSwitch->getIsToggled());
    settings.setValue("enhanced_notifications",
                      m_enhancedNotificationsSwitch->getIsToggled());
    settings.setValue("notification_types",
                      m_notificationTypesCombo->currentData().toString());
    settings.setValue("show_recent_files",
                      m_showRecentFilesSwitch->getIsToggled());
    settings.setValue("recent_files_count", m_recentFilesCountSpin->value());
    settings.setValue("show_quick_actions",
                      m_showQuickActionsSwitch->getIsToggled());
    settings.setValue("show_status_indicators",
                      m_showStatusIndicatorsSwitch->getIsToggled());
    settings.setValue("dynamic_tooltip",
                      m_dynamicTooltipSwitch->getIsToggled());

    settings.endGroup();
    emit settingsChanged();
}

void SystemTraySettingsWidget::resetToDefaults() {
    m_enableTraySwitch->setIsToggled(true);
    m_minimizeToTraySwitch->setIsToggled(true);
    m_closeToTraySwitch->setIsToggled(false);
    m_startMinimizedSwitch->setIsToggled(false);
    m_showNotificationsSwitch->setIsToggled(true);
    m_enhancedNotificationsSwitch->setIsToggled(true);
    m_notificationTypesCombo->setCurrentIndex(0);
    m_showRecentFilesSwitch->setIsToggled(true);
    m_recentFilesCountSpin->setValue(5);
    m_showQuickActionsSwitch->setIsToggled(true);
    m_showStatusIndicatorsSwitch->setIsToggled(true);
    m_dynamicTooltipSwitch->setIsToggled(true);
    updateControlsState();
    emit settingsChanged();
}

void SystemTraySettingsWidget::onTrayEnabledToggled(bool enabled) {
    Q_UNUSED(enabled)
    updateControlsState();
    emit settingsChanged();
}

void SystemTraySettingsWidget::onMinimizeToTrayToggled(bool enabled) {
    Q_UNUSED(enabled)
    emit settingsChanged();
}

void SystemTraySettingsWidget::onNotificationsToggled(bool enabled) {
    m_enhancedNotificationsSwitch->setEnabled(enabled);
    m_notificationTypesCombo->setEnabled(enabled);
    emit settingsChanged();
}

void SystemTraySettingsWidget::updateControlsState() {
    bool enabled = m_enableTraySwitch->getIsToggled();
    m_minimizeToTraySwitch->setEnabled(enabled);
    m_closeToTraySwitch->setEnabled(enabled);
    m_startMinimizedSwitch->setEnabled(enabled);
    m_showNotificationsSwitch->setEnabled(enabled);
    m_enhancedNotificationsSwitch->setEnabled(
        enabled && m_showNotificationsSwitch->getIsToggled());
    m_notificationTypesCombo->setEnabled(
        enabled && m_showNotificationsSwitch->getIsToggled());
    m_showRecentFilesSwitch->setEnabled(enabled);
    m_recentFilesCountSpin->setEnabled(enabled &&
                                       m_showRecentFilesSwitch->getIsToggled());
    m_showQuickActionsSwitch->setEnabled(enabled);
    m_showStatusIndicatorsSwitch->setEnabled(enabled);
    m_dynamicTooltipSwitch->setEnabled(enabled);
}

void SystemTraySettingsWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void SystemTraySettingsWidget::retranslateUi() {
    // Retranslation would be implemented here
}
