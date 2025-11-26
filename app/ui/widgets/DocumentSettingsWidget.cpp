#include "DocumentSettingsWidget.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QSettings>
#include <QVBoxLayout>

#include "ElaComboBox.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ElaToggleSwitch.h"

DocumentSettingsWidget::DocumentSettingsWidget(QWidget* parent)
    : QWidget(parent), m_mainLayout(nullptr) {
    setupUi();
    loadSettings();
}

DocumentSettingsWidget::~DocumentSettingsWidget() = default;

void DocumentSettingsWidget::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(16);

    // Recent Files Section
    auto* recentArea = new ElaScrollPageArea(this);
    auto* recentLayout = new QVBoxLayout(recentArea);
    recentLayout->setContentsMargins(16, 12, 16, 12);

    auto* recentTitle = new ElaText(tr("Recent Files"), this);
    recentTitle->setTextPixelSize(14);
    recentLayout->addWidget(recentTitle);

    auto* maxRow = new QHBoxLayout();
    maxRow->addWidget(new ElaText(tr("Maximum recent files:"), this));
    m_maxRecentFilesSpin = new ElaSpinBox(this);
    m_maxRecentFilesSpin->setRange(5, 50);
    m_maxRecentFilesSpin->setValue(20);
    maxRow->addWidget(m_maxRecentFilesSpin);
    maxRow->addStretch();
    recentLayout->addLayout(maxRow);

    auto* cleanRow = new QHBoxLayout();
    cleanRow->addWidget(new ElaText(tr("Auto-clean invalid files"), this));
    cleanRow->addStretch();
    m_autoCleanupSwitch = new ElaToggleSwitch(this);
    m_autoCleanupSwitch->setIsToggled(true);
    cleanRow->addWidget(m_autoCleanupSwitch);
    recentLayout->addLayout(cleanRow);

    auto* showRow = new QHBoxLayout();
    showRow->addWidget(new ElaText(tr("Show recent files on startup"), this));
    showRow->addStretch();
    m_showRecentOnStartSwitch = new ElaToggleSwitch(this);
    m_showRecentOnStartSwitch->setIsToggled(true);
    showRow->addWidget(m_showRecentOnStartSwitch);
    recentLayout->addLayout(showRow);

    m_mainLayout->addWidget(recentArea);

    // Opening Behavior Section
    auto* openArea = new ElaScrollPageArea(this);
    auto* openLayout = new QVBoxLayout(openArea);
    openLayout->setContentsMargins(16, 12, 16, 12);

    auto* openTitle = new ElaText(tr("Opening Behavior"), this);
    openTitle->setTextPixelSize(14);
    openLayout->addWidget(openTitle);

    auto* posRow = new QHBoxLayout();
    posRow->addWidget(new ElaText(tr("Remember last page position"), this));
    posRow->addStretch();
    m_rememberPositionSwitch = new ElaToggleSwitch(this);
    m_rememberPositionSwitch->setIsToggled(true);
    posRow->addWidget(m_rememberPositionSwitch);
    openLayout->addLayout(posRow);

    auto* zoomRow = new QHBoxLayout();
    zoomRow->addWidget(new ElaText(tr("Remember zoom level"), this));
    zoomRow->addStretch();
    m_rememberZoomSwitch = new ElaToggleSwitch(this);
    m_rememberZoomSwitch->setIsToggled(true);
    zoomRow->addWidget(m_rememberZoomSwitch);
    openLayout->addLayout(zoomRow);

    auto* actionRow = new QHBoxLayout();
    actionRow->addWidget(new ElaText(tr("Default open action:"), this));
    m_defaultOpenActionCombo = new ElaComboBox(this);
    m_defaultOpenActionCombo->addItem(tr("Open in new tab"), "tab");
    m_defaultOpenActionCombo->addItem(tr("Open in new window"), "window");
    m_defaultOpenActionCombo->addItem(tr("Replace current"), "replace");
    actionRow->addWidget(m_defaultOpenActionCombo);
    actionRow->addStretch();
    openLayout->addLayout(actionRow);

    m_mainLayout->addWidget(openArea);

    // First Run Section
    auto* firstArea = new ElaScrollPageArea(this);
    auto* firstLayout = new QVBoxLayout(firstArea);
    firstLayout->setContentsMargins(16, 12, 16, 12);

    auto* firstTitle = new ElaText(tr("First Run Experience"), this);
    firstTitle->setTextPixelSize(14);
    firstLayout->addWidget(firstTitle);

    auto* onbRow = new QHBoxLayout();
    onbRow->addWidget(new ElaText(tr("Show onboarding on first run"), this));
    onbRow->addStretch();
    m_showOnboardingSwitch = new ElaToggleSwitch(this);
    m_showOnboardingSwitch->setIsToggled(true);
    onbRow->addWidget(m_showOnboardingSwitch);
    firstLayout->addLayout(onbRow);

    auto* tipsRow = new QHBoxLayout();
    tipsRow->addWidget(new ElaText(tr("Show tips and hints"), this));
    tipsRow->addStretch();
    m_showTipsSwitch = new ElaToggleSwitch(this);
    m_showTipsSwitch->setIsToggled(true);
    tipsRow->addWidget(m_showTipsSwitch);
    firstLayout->addLayout(tipsRow);

    m_mainLayout->addWidget(firstArea);

    // Auto-save Section
    auto* saveArea = new ElaScrollPageArea(this);
    auto* saveLayout = new QVBoxLayout(saveArea);
    saveLayout->setContentsMargins(16, 12, 16, 12);

    auto* saveTitle = new ElaText(tr("Auto-save"), this);
    saveTitle->setTextPixelSize(14);
    saveLayout->addWidget(saveTitle);

    auto* autoRow = new QHBoxLayout();
    autoRow->addWidget(new ElaText(tr("Auto-save session state"), this));
    autoRow->addStretch();
    m_autoSaveStateSwitch = new ElaToggleSwitch(this);
    m_autoSaveStateSwitch->setIsToggled(true);
    autoRow->addWidget(m_autoSaveStateSwitch);
    saveLayout->addLayout(autoRow);

    auto* intRow = new QHBoxLayout();
    intRow->addWidget(new ElaText(tr("Auto-save interval:"), this));
    m_autoSaveIntervalSpin = new ElaSpinBox(this);
    m_autoSaveIntervalSpin->setRange(1, 30);
    m_autoSaveIntervalSpin->setValue(5);
    m_autoSaveIntervalSpin->setSuffix(tr(" min"));
    intRow->addWidget(m_autoSaveIntervalSpin);
    intRow->addStretch();
    saveLayout->addLayout(intRow);

    m_mainLayout->addWidget(saveArea);

    // File Handling Section
    auto* fileArea = new ElaScrollPageArea(this);
    auto* fileLayout = new QVBoxLayout(fileArea);
    fileLayout->setContentsMargins(16, 12, 16, 12);

    auto* fileTitle = new ElaText(tr("File Handling"), this);
    fileTitle->setTextPixelSize(14);
    fileLayout->addWidget(fileTitle);

    auto* confRow = new QHBoxLayout();
    confRow->addWidget(new ElaText(tr("Confirm before closing"), this));
    confRow->addStretch();
    m_confirmCloseSwitch = new ElaToggleSwitch(this);
    m_confirmCloseSwitch->setIsToggled(true);
    confRow->addWidget(m_confirmCloseSwitch);
    fileLayout->addLayout(confRow);

    auto* reloadRow = new QHBoxLayout();
    reloadRow->addWidget(new ElaText(tr("Auto-reload modified files"), this));
    reloadRow->addStretch();
    m_reloadModifiedSwitch = new ElaToggleSwitch(this);
    reloadRow->addWidget(m_reloadModifiedSwitch);
    fileLayout->addLayout(reloadRow);

    m_mainLayout->addWidget(fileArea);
    m_mainLayout->addStretch();
}

void DocumentSettingsWidget::loadSettings() {
    QSettings s("SAST", "Readium");
    s.beginGroup("Document");
    m_maxRecentFilesSpin->setValue(s.value("max_recent", 20).toInt());
    m_autoCleanupSwitch->setIsToggled(s.value("auto_cleanup", true).toBool());
    m_showRecentOnStartSwitch->setIsToggled(
        s.value("show_recent", true).toBool());
    m_rememberPositionSwitch->setIsToggled(
        s.value("remember_pos", true).toBool());
    m_rememberZoomSwitch->setIsToggled(s.value("remember_zoom", true).toBool());
    int idx = m_defaultOpenActionCombo->findData(s.value("open_action", "tab"));
    if (idx >= 0)
        m_defaultOpenActionCombo->setCurrentIndex(idx);
    m_showOnboardingSwitch->setIsToggled(s.value("onboarding", true).toBool());
    m_showTipsSwitch->setIsToggled(s.value("tips", true).toBool());
    m_autoSaveStateSwitch->setIsToggled(s.value("auto_save", true).toBool());
    m_autoSaveIntervalSpin->setValue(s.value("save_interval", 5).toInt());
    m_confirmCloseSwitch->setIsToggled(s.value("confirm_close", true).toBool());
    m_reloadModifiedSwitch->setIsToggled(
        s.value("reload_modified", false).toBool());
    s.endGroup();
    updateControlsState();
}

void DocumentSettingsWidget::saveSettings() {
    QSettings s("SAST", "Readium");
    s.beginGroup("Document");
    s.setValue("max_recent", m_maxRecentFilesSpin->value());
    s.setValue("auto_cleanup", m_autoCleanupSwitch->getIsToggled());
    s.setValue("show_recent", m_showRecentOnStartSwitch->getIsToggled());
    s.setValue("remember_pos", m_rememberPositionSwitch->getIsToggled());
    s.setValue("remember_zoom", m_rememberZoomSwitch->getIsToggled());
    s.setValue("open_action",
               m_defaultOpenActionCombo->currentData().toString());
    s.setValue("onboarding", m_showOnboardingSwitch->getIsToggled());
    s.setValue("tips", m_showTipsSwitch->getIsToggled());
    s.setValue("auto_save", m_autoSaveStateSwitch->getIsToggled());
    s.setValue("save_interval", m_autoSaveIntervalSpin->value());
    s.setValue("confirm_close", m_confirmCloseSwitch->getIsToggled());
    s.setValue("reload_modified", m_reloadModifiedSwitch->getIsToggled());
    s.endGroup();
    emit settingsChanged();
}

void DocumentSettingsWidget::resetToDefaults() {
    m_maxRecentFilesSpin->setValue(20);
    m_autoCleanupSwitch->setIsToggled(true);
    m_showRecentOnStartSwitch->setIsToggled(true);
    m_rememberPositionSwitch->setIsToggled(true);
    m_rememberZoomSwitch->setIsToggled(true);
    m_defaultOpenActionCombo->setCurrentIndex(0);
    m_showOnboardingSwitch->setIsToggled(true);
    m_showTipsSwitch->setIsToggled(true);
    m_autoSaveStateSwitch->setIsToggled(true);
    m_autoSaveIntervalSpin->setValue(5);
    m_confirmCloseSwitch->setIsToggled(true);
    m_reloadModifiedSwitch->setIsToggled(false);
    updateControlsState();
    emit settingsChanged();
}

void DocumentSettingsWidget::updateControlsState() {
    m_autoSaveIntervalSpin->setEnabled(m_autoSaveStateSwitch->getIsToggled());
}

void DocumentSettingsWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange)
        retranslateUi();
    QWidget::changeEvent(event);
}

void DocumentSettingsWidget::retranslateUi() {}
