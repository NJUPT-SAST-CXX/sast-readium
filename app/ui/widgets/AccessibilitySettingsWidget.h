#pragma once

#include <QWidget>

// Forward declarations
class QVBoxLayout;
class ElaToggleSwitch;
class ElaSlider;
class ElaComboBox;
class ElaPushButton;
class ElaSpinBox;
class ElaText;
class AccessibilityModel;

/**
 * @brief Widget for accessibility settings configuration
 *
 * Provides UI for configuring accessibility features including:
 * - Screen reader support
 * - High contrast mode
 * - Text-to-speech settings
 * - Keyboard navigation
 * - Text scaling
 */
class AccessibilitySettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit AccessibilitySettingsWidget(QWidget* parent = nullptr);
    ~AccessibilitySettingsWidget() override;

    void setAccessibilityModel(AccessibilityModel* model);
    void loadSettings();
    void saveSettings();
    void resetToDefaults();

signals:
    void settingsChanged();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onScreenReaderToggled(bool enabled);
    void onHighContrastToggled(bool enabled);
    void onTtsToggled(bool enabled);
    void onTtsRateChanged(int value);
    void onTtsVolumeChanged(int value);
    void onTextScaleChanged(int value);
    void onReduceMotionToggled(bool enabled);
    void onFocusIndicatorToggled(bool enabled);

private:
    void setupUi();
    void retranslateUi();
    void updateTtsControlsState();

    // UI Components
    QVBoxLayout* m_mainLayout;

    // Screen Reader
    ElaToggleSwitch* m_screenReaderSwitch;
    ElaToggleSwitch* m_announcePageChangesSwitch;
    ElaToggleSwitch* m_announceZoomChangesSwitch;

    // High Contrast
    ElaToggleSwitch* m_highContrastSwitch;
    ElaPushButton* m_customColorsBtn;

    // Text-to-Speech
    ElaToggleSwitch* m_ttsSwitch;
    ElaComboBox* m_ttsVoiceCombo;
    ElaSlider* m_ttsRateSlider;
    ElaSlider* m_ttsVolumeSlider;
    ElaText* m_ttsRateLabel;
    ElaText* m_ttsVolumeLabel;

    // Visual
    ElaSlider* m_textScaleSlider;
    ElaText* m_textScaleLabel;
    ElaToggleSwitch* m_boldTextSwitch;

    // Motion
    ElaToggleSwitch* m_reduceMotionSwitch;
    ElaToggleSwitch* m_reduceTransparencySwitch;

    // Keyboard
    ElaToggleSwitch* m_enhancedKeyboardSwitch;
    ElaToggleSwitch* m_focusIndicatorSwitch;
    ElaSpinBox* m_focusIndicatorWidthSpin;

    // Model reference
    AccessibilityModel* m_model;
};
