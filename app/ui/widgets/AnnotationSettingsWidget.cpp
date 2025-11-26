#include "AnnotationSettingsWidget.h"

#include <QColorDialog>
#include <QEvent>
#include <QHBoxLayout>
#include <QSettings>
#include <QVBoxLayout>

#include "ElaColorDialog.h"
#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaSlider.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ElaToggleSwitch.h"

#include "logging/SimpleLogging.h"

AnnotationSettingsWidget::AnnotationSettingsWidget(QWidget* parent)
    : QWidget(parent),
      m_mainLayout(nullptr),
      m_defaultColorCombo(nullptr),
      m_customColorBtn(nullptr),
      m_opacitySlider(nullptr),
      m_opacityLabel(nullptr),
      m_defaultAuthorEdit(nullptr),
      m_autoSaveSwitch(nullptr),
      m_autoSaveIntervalSpin(nullptr),
      m_showAnnotationIconsSwitch(nullptr),
      m_showHighlightPopupsSwitch(nullptr),
      m_annotationFontSizeCombo(nullptr),
      m_defaultExportFormatCombo(nullptr),
      m_includeNotesInExportSwitch(nullptr),
      m_customColor(Qt::yellow) {
    setupUi();
    loadSettings();
}

AnnotationSettingsWidget::~AnnotationSettingsWidget() = default;

void AnnotationSettingsWidget::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(16);

    // Highlight Section
    auto* highlightArea = new ElaScrollPageArea(this);
    auto* highlightLayout = new QVBoxLayout(highlightArea);
    highlightLayout->setContentsMargins(16, 12, 16, 12);

    auto* highlightTitle = new ElaText(tr("Highlight Settings"), this);
    highlightTitle->setTextPixelSize(14);
    highlightLayout->addWidget(highlightTitle);

    auto* colorRow = new QHBoxLayout();
    auto* colorLabel = new ElaText(tr("Default color:"), this);
    colorRow->addWidget(colorLabel);
    m_defaultColorCombo = new ElaComboBox(this);
    m_defaultColorCombo->addItem(tr("Yellow"), "yellow");
    m_defaultColorCombo->addItem(tr("Green"), "green");
    m_defaultColorCombo->addItem(tr("Blue"), "blue");
    m_defaultColorCombo->addItem(tr("Pink"), "pink");
    m_defaultColorCombo->addItem(tr("Orange"), "orange");
    m_defaultColorCombo->addItem(tr("Purple"), "purple");
    m_defaultColorCombo->addItem(tr("Custom..."), "custom");
    colorRow->addWidget(m_defaultColorCombo);
    m_customColorBtn = new ElaPushButton(tr("Choose..."), this);
    m_customColorBtn->setVisible(false);
    colorRow->addWidget(m_customColorBtn);
    colorRow->addStretch();
    highlightLayout->addLayout(colorRow);

    auto* opacityRow = new QHBoxLayout();
    auto* opacityLabel = new ElaText(tr("Opacity:"), this);
    opacityRow->addWidget(opacityLabel);
    m_opacitySlider = new ElaSlider(Qt::Horizontal, this);
    m_opacitySlider->setRange(10, 100);
    m_opacitySlider->setValue(40);
    opacityRow->addWidget(m_opacitySlider, 1);
    m_opacityLabel = new ElaText("40%", this);
    m_opacityLabel->setFixedWidth(40);
    opacityRow->addWidget(m_opacityLabel);
    highlightLayout->addLayout(opacityRow);

    m_mainLayout->addWidget(highlightArea);

    // Annotation Section
    auto* annotationArea = new ElaScrollPageArea(this);
    auto* annotationLayout = new QVBoxLayout(annotationArea);
    annotationLayout->setContentsMargins(16, 12, 16, 12);

    auto* annotationTitle = new ElaText(tr("Annotation Settings"), this);
    annotationTitle->setTextPixelSize(14);
    annotationLayout->addWidget(annotationTitle);

    auto* authorRow = new QHBoxLayout();
    auto* authorLabel = new ElaText(tr("Default author:"), this);
    authorRow->addWidget(authorLabel);
    m_defaultAuthorEdit = new ElaLineEdit(this);
    m_defaultAuthorEdit->setPlaceholderText(tr("Enter your name"));
    authorRow->addWidget(m_defaultAuthorEdit, 1);
    annotationLayout->addLayout(authorRow);

    auto* autoSaveRow = new QHBoxLayout();
    auto* autoSaveLabel = new ElaText(tr("Auto-save annotations"), this);
    autoSaveRow->addWidget(autoSaveLabel);
    autoSaveRow->addStretch();
    m_autoSaveSwitch = new ElaToggleSwitch(this);
    autoSaveRow->addWidget(m_autoSaveSwitch);
    annotationLayout->addLayout(autoSaveRow);

    auto* intervalRow = new QHBoxLayout();
    auto* intervalLabel = new ElaText(tr("Auto-save interval:"), this);
    intervalRow->addWidget(intervalLabel);
    m_autoSaveIntervalSpin = new ElaSpinBox(this);
    m_autoSaveIntervalSpin->setRange(1, 60);
    m_autoSaveIntervalSpin->setValue(5);
    m_autoSaveIntervalSpin->setSuffix(tr(" min"));
    intervalRow->addWidget(m_autoSaveIntervalSpin);
    intervalRow->addStretch();
    annotationLayout->addLayout(intervalRow);

    m_mainLayout->addWidget(annotationArea);

    // Display Section
    auto* displayArea = new ElaScrollPageArea(this);
    auto* displayLayout = new QVBoxLayout(displayArea);
    displayLayout->setContentsMargins(16, 12, 16, 12);

    auto* displayTitle = new ElaText(tr("Display"), this);
    displayTitle->setTextPixelSize(14);
    displayLayout->addWidget(displayTitle);

    auto* iconsRow = new QHBoxLayout();
    auto* iconsLabel = new ElaText(tr("Show annotation icons"), this);
    iconsRow->addWidget(iconsLabel);
    iconsRow->addStretch();
    m_showAnnotationIconsSwitch = new ElaToggleSwitch(this);
    m_showAnnotationIconsSwitch->setIsToggled(true);
    iconsRow->addWidget(m_showAnnotationIconsSwitch);
    displayLayout->addLayout(iconsRow);

    auto* popupsRow = new QHBoxLayout();
    auto* popupsLabel = new ElaText(tr("Show highlight popups on hover"), this);
    popupsRow->addWidget(popupsLabel);
    popupsRow->addStretch();
    m_showHighlightPopupsSwitch = new ElaToggleSwitch(this);
    m_showHighlightPopupsSwitch->setIsToggled(true);
    popupsRow->addWidget(m_showHighlightPopupsSwitch);
    displayLayout->addLayout(popupsRow);

    auto* fontRow = new QHBoxLayout();
    auto* fontLabel = new ElaText(tr("Annotation font size:"), this);
    fontRow->addWidget(fontLabel);
    m_annotationFontSizeCombo = new ElaComboBox(this);
    m_annotationFontSizeCombo->addItem(tr("Small"), "small");
    m_annotationFontSizeCombo->addItem(tr("Medium"), "medium");
    m_annotationFontSizeCombo->addItem(tr("Large"), "large");
    m_annotationFontSizeCombo->setCurrentIndex(1);
    fontRow->addWidget(m_annotationFontSizeCombo);
    fontRow->addStretch();
    displayLayout->addLayout(fontRow);

    m_mainLayout->addWidget(displayArea);

    // Export Section
    auto* exportArea = new ElaScrollPageArea(this);
    auto* exportLayout = new QVBoxLayout(exportArea);
    exportLayout->setContentsMargins(16, 12, 16, 12);

    auto* exportTitle = new ElaText(tr("Export"), this);
    exportTitle->setTextPixelSize(14);
    exportLayout->addWidget(exportTitle);

    auto* formatRow = new QHBoxLayout();
    auto* formatLabel = new ElaText(tr("Default export format:"), this);
    formatRow->addWidget(formatLabel);
    m_defaultExportFormatCombo = new ElaComboBox(this);
    m_defaultExportFormatCombo->addItem("JSON", "json");
    m_defaultExportFormatCombo->addItem("Markdown", "markdown");
    m_defaultExportFormatCombo->addItem(tr("Plain Text"), "text");
    m_defaultExportFormatCombo->addItem("HTML", "html");
    m_defaultExportFormatCombo->addItem("CSV", "csv");
    formatRow->addWidget(m_defaultExportFormatCombo);
    formatRow->addStretch();
    exportLayout->addLayout(formatRow);

    auto* includeNotesRow = new QHBoxLayout();
    auto* includeNotesLabel = new ElaText(tr("Include notes in export"), this);
    includeNotesRow->addWidget(includeNotesLabel);
    includeNotesRow->addStretch();
    m_includeNotesInExportSwitch = new ElaToggleSwitch(this);
    m_includeNotesInExportSwitch->setIsToggled(true);
    includeNotesRow->addWidget(m_includeNotesInExportSwitch);
    exportLayout->addLayout(includeNotesRow);

    m_mainLayout->addWidget(exportArea);
    m_mainLayout->addStretch();

    // Connect signals
    connect(m_defaultColorCombo,
            QOverload<int>::of(&ElaComboBox::currentIndexChanged), this,
            &AnnotationSettingsWidget::onColorChanged);
    connect(m_customColorBtn, &ElaPushButton::clicked, this,
            &AnnotationSettingsWidget::onChooseCustomColor);
    connect(m_opacitySlider, &ElaSlider::valueChanged, this,
            &AnnotationSettingsWidget::onOpacityChanged);
    connect(m_autoSaveSwitch, &ElaToggleSwitch::toggled, this,
            &AnnotationSettingsWidget::onAutoSaveToggled);
}

void AnnotationSettingsWidget::loadSettings() {
    QSettings settings("SAST", "Readium");
    settings.beginGroup("Annotations");

    int colorIndex = m_defaultColorCombo->findData(
        settings.value("default_color", "yellow").toString());
    if (colorIndex >= 0) {
        m_defaultColorCombo->setCurrentIndex(colorIndex);
    }

    m_customColor =
        QColor(settings.value("custom_color", "#FFFF00").toString());
    m_opacitySlider->setValue(settings.value("opacity", 40).toInt());
    m_defaultAuthorEdit->setText(
        settings.value("default_author", "").toString());
    m_autoSaveSwitch->setIsToggled(settings.value("auto_save", true).toBool());
    m_autoSaveIntervalSpin->setValue(
        settings.value("auto_save_interval", 5).toInt());
    m_showAnnotationIconsSwitch->setIsToggled(
        settings.value("show_icons", true).toBool());
    m_showHighlightPopupsSwitch->setIsToggled(
        settings.value("show_popups", true).toBool());

    int fontIndex = m_annotationFontSizeCombo->findData(
        settings.value("font_size", "medium").toString());
    if (fontIndex >= 0) {
        m_annotationFontSizeCombo->setCurrentIndex(fontIndex);
    }

    int formatIndex = m_defaultExportFormatCombo->findData(
        settings.value("export_format", "json").toString());
    if (formatIndex >= 0) {
        m_defaultExportFormatCombo->setCurrentIndex(formatIndex);
    }

    m_includeNotesInExportSwitch->setIsToggled(
        settings.value("include_notes", true).toBool());

    settings.endGroup();
    updateControlsState();
}

void AnnotationSettingsWidget::saveSettings() {
    QSettings settings("SAST", "Readium");
    settings.beginGroup("Annotations");

    settings.setValue("default_color",
                      m_defaultColorCombo->currentData().toString());
    settings.setValue("custom_color", m_customColor.name());
    settings.setValue("opacity", m_opacitySlider->value());
    settings.setValue("default_author", m_defaultAuthorEdit->text());
    settings.setValue("auto_save", m_autoSaveSwitch->getIsToggled());
    settings.setValue("auto_save_interval", m_autoSaveIntervalSpin->value());
    settings.setValue("show_icons",
                      m_showAnnotationIconsSwitch->getIsToggled());
    settings.setValue("show_popups",
                      m_showHighlightPopupsSwitch->getIsToggled());
    settings.setValue("font_size",
                      m_annotationFontSizeCombo->currentData().toString());
    settings.setValue("export_format",
                      m_defaultExportFormatCombo->currentData().toString());
    settings.setValue("include_notes",
                      m_includeNotesInExportSwitch->getIsToggled());

    settings.endGroup();
    emit settingsChanged();
}

void AnnotationSettingsWidget::resetToDefaults() {
    m_defaultColorCombo->setCurrentIndex(0);  // Yellow
    m_customColor = Qt::yellow;
    m_opacitySlider->setValue(40);
    m_defaultAuthorEdit->clear();
    m_autoSaveSwitch->setIsToggled(true);
    m_autoSaveIntervalSpin->setValue(5);
    m_showAnnotationIconsSwitch->setIsToggled(true);
    m_showHighlightPopupsSwitch->setIsToggled(true);
    m_annotationFontSizeCombo->setCurrentIndex(1);   // Medium
    m_defaultExportFormatCombo->setCurrentIndex(0);  // JSON
    m_includeNotesInExportSwitch->setIsToggled(true);
    updateControlsState();
    emit settingsChanged();
}

void AnnotationSettingsWidget::onColorChanged(int index) {
    QString colorData = m_defaultColorCombo->itemData(index).toString();
    m_customColorBtn->setVisible(colorData == "custom");
    emit settingsChanged();
}

void AnnotationSettingsWidget::onOpacityChanged(int value) {
    m_opacityLabel->setText(QString("%1%").arg(value));
    emit settingsChanged();
}

void AnnotationSettingsWidget::onAutoSaveToggled(bool enabled) {
    m_autoSaveIntervalSpin->setEnabled(enabled);
    emit settingsChanged();
}

void AnnotationSettingsWidget::onChooseCustomColor() {
    auto* colorDialog = new ElaColorDialog(this);
    colorDialog->setCurrentColor(m_customColor);
    connect(colorDialog, &ElaColorDialog::colorSelected, this,
            [this](const QColor& color) {
                m_customColor = color;
                emit settingsChanged();
            });
    colorDialog->exec();
    colorDialog->deleteLater();
}

void AnnotationSettingsWidget::updateControlsState() {
    QString colorData = m_defaultColorCombo->currentData().toString();
    m_customColorBtn->setVisible(colorData == "custom");
    m_autoSaveIntervalSpin->setEnabled(m_autoSaveSwitch->getIsToggled());
}

void AnnotationSettingsWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void AnnotationSettingsWidget::retranslateUi() {
    // Retranslation would be implemented here
}
