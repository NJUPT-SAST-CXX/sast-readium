#include "AccessibilitySettingsWidget.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "ElaComboBox.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaSlider.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ElaToggleSwitch.h"

#include "logging/SimpleLogging.h"
#include "model/AccessibilityModel.h"

AccessibilitySettingsWidget::AccessibilitySettingsWidget(QWidget* parent)
    : QWidget(parent),
      m_mainLayout(nullptr),
      m_screenReaderSwitch(nullptr),
      m_announcePageChangesSwitch(nullptr),
      m_announceZoomChangesSwitch(nullptr),
      m_highContrastSwitch(nullptr),
      m_customColorsBtn(nullptr),
      m_ttsSwitch(nullptr),
      m_ttsVoiceCombo(nullptr),
      m_ttsRateSlider(nullptr),
      m_ttsVolumeSlider(nullptr),
      m_ttsRateLabel(nullptr),
      m_ttsVolumeLabel(nullptr),
      m_textScaleSlider(nullptr),
      m_textScaleLabel(nullptr),
      m_boldTextSwitch(nullptr),
      m_reduceMotionSwitch(nullptr),
      m_reduceTransparencySwitch(nullptr),
      m_enhancedKeyboardSwitch(nullptr),
      m_focusIndicatorSwitch(nullptr),
      m_focusIndicatorWidthSpin(nullptr),
      m_model(nullptr) {
    setupUi();
    loadSettings();
}

AccessibilitySettingsWidget::~AccessibilitySettingsWidget() = default;

void AccessibilitySettingsWidget::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(16);

    // Screen Reader Section
    auto* screenReaderArea = new ElaScrollPageArea(this);
    auto* screenReaderLayout = new QVBoxLayout(screenReaderArea);
    screenReaderLayout->setContentsMargins(16, 12, 16, 12);

    auto* screenReaderTitle = new ElaText(tr("Screen Reader"), this);
    screenReaderTitle->setTextPixelSize(14);
    screenReaderLayout->addWidget(screenReaderTitle);

    auto* screenReaderRow = new QHBoxLayout();
    auto* screenReaderLabel =
        new ElaText(tr("Enable screen reader support"), this);
    screenReaderRow->addWidget(screenReaderLabel);
    screenReaderRow->addStretch();
    m_screenReaderSwitch = new ElaToggleSwitch(this);
    screenReaderRow->addWidget(m_screenReaderSwitch);
    screenReaderLayout->addLayout(screenReaderRow);

    auto* announcePageRow = new QHBoxLayout();
    auto* announcePageLabel = new ElaText(tr("Announce page changes"), this);
    announcePageRow->addWidget(announcePageLabel);
    announcePageRow->addStretch();
    m_announcePageChangesSwitch = new ElaToggleSwitch(this);
    announcePageRow->addWidget(m_announcePageChangesSwitch);
    screenReaderLayout->addLayout(announcePageRow);

    auto* announceZoomRow = new QHBoxLayout();
    auto* announceZoomLabel = new ElaText(tr("Announce zoom changes"), this);
    announceZoomRow->addWidget(announceZoomLabel);
    announceZoomRow->addStretch();
    m_announceZoomChangesSwitch = new ElaToggleSwitch(this);
    announceZoomRow->addWidget(m_announceZoomChangesSwitch);
    screenReaderLayout->addLayout(announceZoomRow);

    m_mainLayout->addWidget(screenReaderArea);

    // High Contrast Section
    auto* contrastArea = new ElaScrollPageArea(this);
    auto* contrastLayout = new QVBoxLayout(contrastArea);
    contrastLayout->setContentsMargins(16, 12, 16, 12);

    auto* contrastTitle = new ElaText(tr("High Contrast"), this);
    contrastTitle->setTextPixelSize(14);
    contrastLayout->addWidget(contrastTitle);

    auto* contrastRow = new QHBoxLayout();
    auto* contrastLabel = new ElaText(tr("Enable high contrast mode"), this);
    contrastRow->addWidget(contrastLabel);
    contrastRow->addStretch();
    m_highContrastSwitch = new ElaToggleSwitch(this);
    contrastRow->addWidget(m_highContrastSwitch);
    contrastLayout->addLayout(contrastRow);

    m_customColorsBtn = new ElaPushButton(tr("Customize Colors..."), this);
    contrastLayout->addWidget(m_customColorsBtn);

    m_mainLayout->addWidget(contrastArea);

    // Text-to-Speech Section
    auto* ttsArea = new ElaScrollPageArea(this);
    auto* ttsLayout = new QVBoxLayout(ttsArea);
    ttsLayout->setContentsMargins(16, 12, 16, 12);

    auto* ttsTitle = new ElaText(tr("Text-to-Speech"), this);
    ttsTitle->setTextPixelSize(14);
    ttsLayout->addWidget(ttsTitle);

    auto* ttsRow = new QHBoxLayout();
    auto* ttsLabel = new ElaText(tr("Enable text-to-speech"), this);
    ttsRow->addWidget(ttsLabel);
    ttsRow->addStretch();
    m_ttsSwitch = new ElaToggleSwitch(this);
    ttsRow->addWidget(m_ttsSwitch);
    ttsLayout->addLayout(ttsRow);

    auto* voiceRow = new QHBoxLayout();
    auto* voiceLabel = new ElaText(tr("Voice:"), this);
    voiceRow->addWidget(voiceLabel);
    m_ttsVoiceCombo = new ElaComboBox(this);
    m_ttsVoiceCombo->addItem(tr("System Default"));
    voiceRow->addWidget(m_ttsVoiceCombo);
    voiceRow->addStretch();
    ttsLayout->addLayout(voiceRow);

    auto* rateRow = new QHBoxLayout();
    auto* rateLabel = new ElaText(tr("Speed:"), this);
    rateRow->addWidget(rateLabel);
    m_ttsRateSlider = new ElaSlider(Qt::Horizontal, this);
    m_ttsRateSlider->setRange(0, 100);
    m_ttsRateSlider->setValue(50);
    rateRow->addWidget(m_ttsRateSlider);
    m_ttsRateLabel = new ElaText("1.0x", this);
    m_ttsRateLabel->setFixedWidth(40);
    rateRow->addWidget(m_ttsRateLabel);
    ttsLayout->addLayout(rateRow);

    auto* volumeRow = new QHBoxLayout();
    auto* volumeLabel = new ElaText(tr("Volume:"), this);
    volumeRow->addWidget(volumeLabel);
    m_ttsVolumeSlider = new ElaSlider(Qt::Horizontal, this);
    m_ttsVolumeSlider->setRange(0, 100);
    m_ttsVolumeSlider->setValue(100);
    volumeRow->addWidget(m_ttsVolumeSlider);
    m_ttsVolumeLabel = new ElaText("100%", this);
    m_ttsVolumeLabel->setFixedWidth(40);
    volumeRow->addWidget(m_ttsVolumeLabel);
    ttsLayout->addLayout(volumeRow);

    m_mainLayout->addWidget(ttsArea);

    // Visual Section
    auto* visualArea = new ElaScrollPageArea(this);
    auto* visualLayout = new QVBoxLayout(visualArea);
    visualLayout->setContentsMargins(16, 12, 16, 12);

    auto* visualTitle = new ElaText(tr("Visual"), this);
    visualTitle->setTextPixelSize(14);
    visualLayout->addWidget(visualTitle);

    auto* scaleRow = new QHBoxLayout();
    auto* scaleLabel = new ElaText(tr("Text Scale:"), this);
    scaleRow->addWidget(scaleLabel);
    m_textScaleSlider = new ElaSlider(Qt::Horizontal, this);
    m_textScaleSlider->setRange(50, 200);
    m_textScaleSlider->setValue(100);
    scaleRow->addWidget(m_textScaleSlider);
    m_textScaleLabel = new ElaText("100%", this);
    m_textScaleLabel->setFixedWidth(40);
    scaleRow->addWidget(m_textScaleLabel);
    visualLayout->addLayout(scaleRow);

    auto* boldRow = new QHBoxLayout();
    auto* boldLabel = new ElaText(tr("Bold text"), this);
    boldRow->addWidget(boldLabel);
    boldRow->addStretch();
    m_boldTextSwitch = new ElaToggleSwitch(this);
    boldRow->addWidget(m_boldTextSwitch);
    visualLayout->addLayout(boldRow);

    m_mainLayout->addWidget(visualArea);

    // Motion Section
    auto* motionArea = new ElaScrollPageArea(this);
    auto* motionLayout = new QVBoxLayout(motionArea);
    motionLayout->setContentsMargins(16, 12, 16, 12);

    auto* motionTitle = new ElaText(tr("Motion & Effects"), this);
    motionTitle->setTextPixelSize(14);
    motionLayout->addWidget(motionTitle);

    auto* reduceMotionRow = new QHBoxLayout();
    auto* reduceMotionLabel = new ElaText(tr("Reduce motion"), this);
    reduceMotionRow->addWidget(reduceMotionLabel);
    reduceMotionRow->addStretch();
    m_reduceMotionSwitch = new ElaToggleSwitch(this);
    reduceMotionRow->addWidget(m_reduceMotionSwitch);
    motionLayout->addLayout(reduceMotionRow);

    auto* reduceTransRow = new QHBoxLayout();
    auto* reduceTransLabel = new ElaText(tr("Reduce transparency"), this);
    reduceTransRow->addWidget(reduceTransLabel);
    reduceTransRow->addStretch();
    m_reduceTransparencySwitch = new ElaToggleSwitch(this);
    reduceTransRow->addWidget(m_reduceTransparencySwitch);
    motionLayout->addLayout(reduceTransRow);

    m_mainLayout->addWidget(motionArea);

    // Keyboard Section
    auto* keyboardArea = new ElaScrollPageArea(this);
    auto* keyboardLayout = new QVBoxLayout(keyboardArea);
    keyboardLayout->setContentsMargins(16, 12, 16, 12);

    auto* keyboardTitle = new ElaText(tr("Keyboard Navigation"), this);
    keyboardTitle->setTextPixelSize(14);
    keyboardLayout->addWidget(keyboardTitle);

    auto* enhancedRow = new QHBoxLayout();
    auto* enhancedLabel = new ElaText(tr("Enhanced keyboard navigation"), this);
    enhancedRow->addWidget(enhancedLabel);
    enhancedRow->addStretch();
    m_enhancedKeyboardSwitch = new ElaToggleSwitch(this);
    enhancedRow->addWidget(m_enhancedKeyboardSwitch);
    keyboardLayout->addLayout(enhancedRow);

    auto* focusRow = new QHBoxLayout();
    auto* focusLabel = new ElaText(tr("Show focus indicator"), this);
    focusRow->addWidget(focusLabel);
    focusRow->addStretch();
    m_focusIndicatorSwitch = new ElaToggleSwitch(this);
    focusRow->addWidget(m_focusIndicatorSwitch);
    keyboardLayout->addLayout(focusRow);

    auto* focusWidthRow = new QHBoxLayout();
    auto* focusWidthLabel = new ElaText(tr("Focus indicator width:"), this);
    focusWidthRow->addWidget(focusWidthLabel);
    m_focusIndicatorWidthSpin = new ElaSpinBox(this);
    m_focusIndicatorWidthSpin->setRange(1, 5);
    m_focusIndicatorWidthSpin->setValue(2);
    m_focusIndicatorWidthSpin->setSuffix(" px");
    focusWidthRow->addWidget(m_focusIndicatorWidthSpin);
    focusWidthRow->addStretch();
    keyboardLayout->addLayout(focusWidthRow);

    m_mainLayout->addWidget(keyboardArea);
    m_mainLayout->addStretch();

    // Connect signals
    connect(m_screenReaderSwitch, &ElaToggleSwitch::toggled, this,
            &AccessibilitySettingsWidget::onScreenReaderToggled);
    connect(m_highContrastSwitch, &ElaToggleSwitch::toggled, this,
            &AccessibilitySettingsWidget::onHighContrastToggled);
    connect(m_ttsSwitch, &ElaToggleSwitch::toggled, this,
            &AccessibilitySettingsWidget::onTtsToggled);
    connect(m_ttsRateSlider, &ElaSlider::valueChanged, this,
            &AccessibilitySettingsWidget::onTtsRateChanged);
    connect(m_ttsVolumeSlider, &ElaSlider::valueChanged, this,
            &AccessibilitySettingsWidget::onTtsVolumeChanged);
    connect(m_textScaleSlider, &ElaSlider::valueChanged, this,
            &AccessibilitySettingsWidget::onTextScaleChanged);
    connect(m_reduceMotionSwitch, &ElaToggleSwitch::toggled, this,
            &AccessibilitySettingsWidget::onReduceMotionToggled);
    connect(m_focusIndicatorSwitch, &ElaToggleSwitch::toggled, this,
            &AccessibilitySettingsWidget::onFocusIndicatorToggled);
}

void AccessibilitySettingsWidget::setAccessibilityModel(
    AccessibilityModel* model) {
    m_model = model;
    loadSettings();
}

void AccessibilitySettingsWidget::loadSettings() {
    if (!m_model) {
        return;
    }

    m_screenReaderSwitch->setIsToggled(m_model->isScreenReaderEnabled());
    m_announcePageChangesSwitch->setIsToggled(
        m_model->shouldAnnouncePageChanges());
    m_announceZoomChangesSwitch->setIsToggled(
        m_model->shouldAnnounceZoomChanges());
    m_highContrastSwitch->setIsToggled(m_model->isHighContrastMode());
    m_ttsSwitch->setIsToggled(m_model->isTtsEnabled());
    m_ttsRateSlider->setValue(
        static_cast<int>((m_model->ttsRate() + 1.0) * 50));
    m_ttsVolumeSlider->setValue(static_cast<int>(m_model->ttsVolume() * 100));
    m_textScaleSlider->setValue(
        static_cast<int>(m_model->textScaleFactor() * 100));
    m_boldTextSwitch->setIsToggled(m_model->isBoldTextEnabled());
    m_reduceMotionSwitch->setIsToggled(m_model->shouldReduceMotion());
    m_reduceTransparencySwitch->setIsToggled(
        m_model->shouldReduceTransparency());
    m_enhancedKeyboardSwitch->setIsToggled(
        m_model->isEnhancedKeyboardNavigationEnabled());
    m_focusIndicatorSwitch->setIsToggled(m_model->isFocusIndicatorVisible());
    m_focusIndicatorWidthSpin->setValue(m_model->focusIndicatorWidth());

    updateTtsControlsState();
}

void AccessibilitySettingsWidget::saveSettings() {
    if (!m_model) {
        return;
    }

    m_model->setScreenReaderEnabled(m_screenReaderSwitch->getIsToggled());
    m_model->setShouldAnnouncePageChanges(
        m_announcePageChangesSwitch->getIsToggled());
    m_model->setShouldAnnounceZoomChanges(
        m_announceZoomChangesSwitch->getIsToggled());
    m_model->setHighContrastMode(m_highContrastSwitch->getIsToggled());
    m_model->setTtsEnabled(m_ttsSwitch->getIsToggled());
    m_model->setTtsRate((m_ttsRateSlider->value() / 50.0) - 1.0);
    m_model->setTtsVolume(m_ttsVolumeSlider->value() / 100.0);
    m_model->setTextScaleFactor(m_textScaleSlider->value() / 100.0);
    m_model->setBoldTextEnabled(m_boldTextSwitch->getIsToggled());
    m_model->setReduceMotion(m_reduceMotionSwitch->getIsToggled());
    m_model->setReduceTransparency(m_reduceTransparencySwitch->getIsToggled());
    m_model->setEnhancedKeyboardNavigationEnabled(
        m_enhancedKeyboardSwitch->getIsToggled());
    m_model->setFocusIndicatorVisible(m_focusIndicatorSwitch->getIsToggled());
    m_model->setFocusIndicatorWidth(m_focusIndicatorWidthSpin->value());

    m_model->saveSettings();
    emit settingsChanged();
}

void AccessibilitySettingsWidget::resetToDefaults() {
    if (m_model) {
        m_model->resetToDefaults();
        loadSettings();
        emit settingsChanged();
    }
}

void AccessibilitySettingsWidget::onScreenReaderToggled(bool enabled) {
    Q_UNUSED(enabled)
    emit settingsChanged();
}

void AccessibilitySettingsWidget::onHighContrastToggled(bool enabled) {
    Q_UNUSED(enabled)
    emit settingsChanged();
}

void AccessibilitySettingsWidget::onTtsToggled(bool enabled) {
    Q_UNUSED(enabled)
    updateTtsControlsState();
    emit settingsChanged();
}

void AccessibilitySettingsWidget::onTtsRateChanged(int value) {
    double rate = (value / 50.0) - 1.0;
    m_ttsRateLabel->setText(
        QString("%1x").arg(QString::number(rate + 1.0, 'f', 1)));
    emit settingsChanged();
}

void AccessibilitySettingsWidget::onTtsVolumeChanged(int value) {
    m_ttsVolumeLabel->setText(QString("%1%").arg(value));
    emit settingsChanged();
}

void AccessibilitySettingsWidget::onTextScaleChanged(int value) {
    m_textScaleLabel->setText(QString("%1%").arg(value));
    emit settingsChanged();
}

void AccessibilitySettingsWidget::onReduceMotionToggled(bool enabled) {
    Q_UNUSED(enabled)
    emit settingsChanged();
}

void AccessibilitySettingsWidget::onFocusIndicatorToggled(bool enabled) {
    m_focusIndicatorWidthSpin->setEnabled(enabled);
    emit settingsChanged();
}

void AccessibilitySettingsWidget::updateTtsControlsState() {
    bool enabled = m_ttsSwitch->getIsToggled();
    m_ttsVoiceCombo->setEnabled(enabled);
    m_ttsRateSlider->setEnabled(enabled);
    m_ttsVolumeSlider->setEnabled(enabled);
}

void AccessibilitySettingsWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void AccessibilitySettingsWidget::retranslateUi() {
    // Retranslation would be implemented here
}
