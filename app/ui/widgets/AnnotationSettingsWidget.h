#pragma once

#include <QWidget>

// Forward declarations
class QVBoxLayout;
class ElaToggleSwitch;
class ElaSlider;
class ElaComboBox;
class ElaSpinBox;
class ElaLineEdit;
class ElaPushButton;
class ElaText;

/**
 * @brief Widget for annotation and highlight settings
 *
 * Provides UI for configuring annotation behavior including:
 * - Default highlight color and opacity
 * - Annotation author settings
 * - Auto-save preferences
 * - Export settings
 */
class AnnotationSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit AnnotationSettingsWidget(QWidget* parent = nullptr);
    ~AnnotationSettingsWidget() override;

    void loadSettings();
    void saveSettings();
    void resetToDefaults();

signals:
    void settingsChanged();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onColorChanged(int index);
    void onOpacityChanged(int value);
    void onAutoSaveToggled(bool enabled);
    void onChooseCustomColor();

private:
    void setupUi();
    void retranslateUi();
    void updateControlsState();

    // UI Components
    QVBoxLayout* m_mainLayout;

    // Highlight settings
    ElaComboBox* m_defaultColorCombo;
    ElaPushButton* m_customColorBtn;
    ElaSlider* m_opacitySlider;
    ElaText* m_opacityLabel;

    // Annotation settings
    ElaLineEdit* m_defaultAuthorEdit;
    ElaToggleSwitch* m_autoSaveSwitch;
    ElaSpinBox* m_autoSaveIntervalSpin;

    // Display settings
    ElaToggleSwitch* m_showAnnotationIconsSwitch;
    ElaToggleSwitch* m_showHighlightPopupsSwitch;
    ElaComboBox* m_annotationFontSizeCombo;

    // Export settings
    ElaComboBox* m_defaultExportFormatCombo;
    ElaToggleSwitch* m_includeNotesInExportSwitch;

    // Current custom color
    QColor m_customColor;
};
