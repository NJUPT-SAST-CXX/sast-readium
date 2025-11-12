#pragma once

#include <QAction>
#include <QActionGroup>
#include <QButtonGroup>
#include <QColorDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include "../../model/AnnotationModel.h"
#include "ElaComboBox.h"
#include "ElaPushButton.h"
#include "ElaSlider.h"
#include "ElaSpinBox.h"
#include "ElaText.h"

/**
 * Toolbar for annotation tools and controls
 */
class AnnotationToolbar : public QWidget {
    Q_OBJECT

public:
    explicit AnnotationToolbar(QWidget* parent = nullptr);
    ~AnnotationToolbar() = default;

    // Tool selection
    AnnotationType getCurrentTool() const { return m_currentTool; }
    void setCurrentTool(AnnotationType tool);

    // Tool properties
    QColor getCurrentColor() const { return m_currentColor; }
    void setCurrentColor(const QColor& color);

    double getCurrentOpacity() const { return m_currentOpacity; }
    void setCurrentOpacity(double opacity);

    double getCurrentLineWidth() const { return m_currentLineWidth; }
    void setCurrentLineWidth(double width);

    int getCurrentFontSize() const { return m_currentFontSize; }
    void setCurrentFontSize(int size);

    QString getCurrentFontFamily() const { return m_currentFontFamily; }
    void setCurrentFontFamily(const QString& family);

    // UI state
    void setEnabled(bool enabled);
    void resetToDefaults();

signals:
    void toolChanged(AnnotationType tool);
    void colorChanged(const QColor& color);
    void opacityChanged(double opacity);
    void lineWidthChanged(double width);
    void fontSizeChanged(int size);
    void fontFamilyChanged(const QString& family);
    void clearAllAnnotations();
    void saveAnnotations();
    void loadAnnotations();

private slots:
    void onToolButtonClicked();
    void onColorButtonClicked();
    void onOpacitySliderChanged(int value);
    void onLineWidthChanged(int value);
    void onFontSizeChanged(int size);
    void onFontFamilyChanged(const QString& family);

protected:
    void changeEvent(QEvent* event) override;

private:
    void setupUI();
    void setupConnections();
    void updateToolButtons();
    void updateColorButton();
    void updatePropertyControls();
    void retranslateUi();

    // Tool selection
    QGroupBox* m_toolGroup;
    QHBoxLayout* m_toolLayout;
    QButtonGroup* m_toolButtonGroup;

    ElaPushButton* m_highlightBtn;
    ElaPushButton* m_noteBtn;
    ElaPushButton* m_freeTextBtn;
    ElaPushButton* m_underlineBtn;
    ElaPushButton* m_strikeOutBtn;
    ElaPushButton* m_rectangleBtn;
    ElaPushButton* m_circleBtn;
    ElaPushButton* m_lineBtn;
    ElaPushButton* m_arrowBtn;
    ElaPushButton* m_inkBtn;

    // Properties
    QGroupBox* m_propertiesGroup;
    QVBoxLayout* m_propertiesLayout;

    ElaPushButton* m_colorButton;
    QColorDialog* m_colorDialog;

    ElaText* m_opacityLabel;
    ElaSlider* m_opacitySlider;

    ElaText* m_lineWidthLabel;
    ElaSpinBox* m_lineWidthSpinBox;

    ElaText* m_fontSizeLabel;
    ElaSpinBox* m_fontSizeSpinBox;

    ElaText* m_fontFamilyLabel;
    ElaComboBox* m_fontFamilyCombo;

    // Actions
    QGroupBox* m_actionsGroup;
    QHBoxLayout* m_actionsLayout;

    ElaPushButton* m_clearAllBtn;
    ElaPushButton* m_saveBtn;
    ElaPushButton* m_loadBtn;

    // Current state
    AnnotationType m_currentTool;
    QColor m_currentColor;
    double m_currentOpacity;
    double m_currentLineWidth;
    int m_currentFontSize;
    QString m_currentFontFamily;
};
