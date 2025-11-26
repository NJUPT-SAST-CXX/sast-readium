#include "AnnotationToolbar.h"
#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QFontDatabase>
#include <QStyle>
#include "../../managers/I18nManager.h"
#include "ElaScrollPageArea.h"

AnnotationToolbar::AnnotationToolbar(QWidget* parent)
    : QWidget(parent),
      m_toolGroup(nullptr),
      m_toolTitle(nullptr),
      m_toolLayout(nullptr),
      m_toolButtonGroup(nullptr),
      m_highlightBtn(nullptr),
      m_noteBtn(nullptr),
      m_freeTextBtn(nullptr),
      m_underlineBtn(nullptr),
      m_strikeOutBtn(nullptr),
      m_rectangleBtn(nullptr),
      m_circleBtn(nullptr),
      m_lineBtn(nullptr),
      m_arrowBtn(nullptr),
      m_inkBtn(nullptr),
      m_propertiesGroup(nullptr),
      m_propertiesTitle(nullptr),
      m_propertiesLayout(nullptr),
      m_colorButton(nullptr),
      m_colorDialog(nullptr),
      m_opacityLabel(nullptr),
      m_opacitySlider(nullptr),
      m_lineWidthLabel(nullptr),
      m_lineWidthSpinBox(nullptr),
      m_fontSizeLabel(nullptr),
      m_fontSizeSpinBox(nullptr),
      m_fontFamilyLabel(nullptr),
      m_fontFamilyCombo(nullptr),
      m_actionsGroup(nullptr),
      m_actionsTitle(nullptr),
      m_actionsLayout(nullptr),
      m_clearAllBtn(nullptr),
      m_saveBtn(nullptr),
      m_loadBtn(nullptr),
      m_currentTool(AnnotationType::Highlight),
      m_currentColor(Qt::yellow),
      m_currentOpacity(0.7),
      m_currentLineWidth(2.0),
      m_currentFontSize(12),
      m_currentFontFamily("Arial") {
    setupUI();
    setupConnections();
    resetToDefaults();

    // Connect to language change signal (disambiguate overloaded signal)
    connect(&I18nManager::instance(),
            static_cast<void (I18nManager::*)(I18nManager::Language)>(
                &I18nManager::languageChanged),
            this, [this](I18nManager::Language) { retranslateUi(); });
}

void AnnotationToolbar::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(8);

    // Tool selection group
    m_toolGroup = new ElaScrollPageArea(this);
    auto* toolVLayout = new QVBoxLayout(m_toolGroup);
    toolVLayout->setContentsMargins(12, 8, 12, 12);

    m_toolTitle = new ElaText(tr("Annotation Tools"), m_toolGroup);
    m_toolTitle->setTextPixelSize(14);
    toolVLayout->addWidget(m_toolTitle);

    auto* toolContent = new QWidget(m_toolGroup);
    m_toolLayout = new QHBoxLayout(toolContent);
    m_toolLayout->setContentsMargins(0, 6, 0, 0);
    toolVLayout->addWidget(toolContent);

    m_toolButtonGroup = new QButtonGroup(this);

    // Create tool buttons
    m_highlightBtn = new ElaPushButton(tr("Highlight"));
    m_highlightBtn->setCheckable(true);
    m_highlightBtn->setToolTip(tr("Highlight text"));
    m_highlightBtn->setProperty("tool",
                                static_cast<int>(AnnotationType::Highlight));

    m_noteBtn = new ElaPushButton(tr("Note"));
    m_noteBtn->setCheckable(true);
    m_noteBtn->setToolTip(tr("Add note"));
    m_noteBtn->setProperty("tool", static_cast<int>(AnnotationType::Note));

    m_freeTextBtn = new ElaPushButton(tr("Text"));
    m_freeTextBtn->setCheckable(true);
    m_freeTextBtn->setToolTip(tr("Free text"));
    m_freeTextBtn->setProperty("tool",
                               static_cast<int>(AnnotationType::FreeText));

    m_underlineBtn = new ElaPushButton(tr("Underline"));
    m_underlineBtn->setCheckable(true);
    m_underlineBtn->setToolTip(tr("Underline text"));
    m_underlineBtn->setProperty("tool",
                                static_cast<int>(AnnotationType::Underline));

    m_strikeOutBtn = new ElaPushButton(tr("Strikeout"));
    m_strikeOutBtn->setCheckable(true);
    m_strikeOutBtn->setToolTip(tr("Strikeout text"));
    m_strikeOutBtn->setProperty("tool",
                                static_cast<int>(AnnotationType::StrikeOut));

    m_rectangleBtn = new ElaPushButton(tr("Rectangle"));
    m_rectangleBtn->setCheckable(true);
    m_rectangleBtn->setToolTip(tr("Draw rectangle"));
    m_rectangleBtn->setProperty("tool",
                                static_cast<int>(AnnotationType::Rectangle));

    m_circleBtn = new ElaPushButton(tr("Circle"));
    m_circleBtn->setCheckable(true);
    m_circleBtn->setToolTip(tr("Draw circle"));
    m_circleBtn->setProperty("tool", static_cast<int>(AnnotationType::Circle));

    m_lineBtn = new ElaPushButton(tr("Line"));
    m_lineBtn->setCheckable(true);
    m_lineBtn->setToolTip(tr("Draw line"));
    m_lineBtn->setProperty("tool", static_cast<int>(AnnotationType::Line));

    m_arrowBtn = new ElaPushButton(tr("Arrow"));
    m_arrowBtn->setCheckable(true);
    m_arrowBtn->setToolTip(tr("Draw arrow"));
    m_arrowBtn->setProperty("tool", static_cast<int>(AnnotationType::Arrow));

    m_inkBtn = new ElaPushButton(tr("Ink"));
    m_inkBtn->setCheckable(true);
    m_inkBtn->setToolTip(tr("Freehand drawing"));
    m_inkBtn->setProperty("tool", static_cast<int>(AnnotationType::Ink));

    // Add buttons to group and layout
    QList<QPushButton*> toolButtons = {
        m_highlightBtn, m_noteBtn,      m_freeTextBtn, m_underlineBtn,
        m_strikeOutBtn, m_rectangleBtn, m_circleBtn,   m_lineBtn,
        m_arrowBtn,     m_inkBtn};

    for (QPushButton* btn : toolButtons) {
        m_toolButtonGroup->addButton(btn);
        m_toolLayout->addWidget(btn);
        btn->setMinimumSize(60, 30);
    }

    m_highlightBtn->setChecked(true);  // Default selection

    // Properties group
    m_propertiesGroup = new ElaScrollPageArea(this);
    auto* propsVLayout = new QVBoxLayout(m_propertiesGroup);
    propsVLayout->setContentsMargins(12, 8, 12, 12);

    m_propertiesTitle = new ElaText(tr("Properties"), m_propertiesGroup);
    m_propertiesTitle->setTextPixelSize(14);
    propsVLayout->addWidget(m_propertiesTitle);

    auto* propsContent = new QWidget(m_propertiesGroup);
    m_propertiesLayout = new QVBoxLayout(propsContent);
    m_propertiesLayout->setContentsMargins(0, 6, 0, 0);
    propsVLayout->addWidget(propsContent);

    // Color selection
    auto* colorLayout = new QHBoxLayout();
    colorLayout->addWidget(new ElaText(tr("Color:")));
    m_colorButton = new ElaPushButton();
    m_colorButton->setMinimumSize(40, 25);
    m_colorButton->setMaximumSize(40, 25);
    m_colorButton->setStyleSheet("border: 1px solid gray;");
    colorLayout->addWidget(m_colorButton);
    colorLayout->addStretch();
    m_propertiesLayout->addLayout(colorLayout);

    // Opacity control
    auto* opacityLayout = new QHBoxLayout();
    m_opacityLabel = new ElaText(tr("Opacity: 70%"));
    opacityLayout->addWidget(m_opacityLabel);
    m_opacitySlider = new ElaSlider(Qt::Horizontal);
    m_opacitySlider->setRange(10, 100);
    m_opacitySlider->setValue(70);
    opacityLayout->addWidget(m_opacitySlider);
    m_propertiesLayout->addLayout(opacityLayout);

    // Line width control
    auto* lineWidthLayout = new QHBoxLayout();
    m_lineWidthLabel = new ElaText(tr("Line Width:"));
    lineWidthLayout->addWidget(m_lineWidthLabel);
    m_lineWidthSpinBox = new ElaSpinBox();
    m_lineWidthSpinBox->setRange(1, 10);
    m_lineWidthSpinBox->setValue(2);
    m_lineWidthSpinBox->setSuffix(" px");
    lineWidthLayout->addWidget(m_lineWidthSpinBox);
    lineWidthLayout->addStretch();
    m_propertiesLayout->addLayout(lineWidthLayout);

    // Font size control
    auto* fontSizeLayout = new QHBoxLayout();
    m_fontSizeLabel = new ElaText(tr("Font Size:"));
    fontSizeLayout->addWidget(m_fontSizeLabel);
    m_fontSizeSpinBox = new ElaSpinBox();
    m_fontSizeSpinBox->setRange(8, 72);
    m_fontSizeSpinBox->setValue(12);
    m_fontSizeSpinBox->setSuffix(" pt");
    fontSizeLayout->addWidget(m_fontSizeSpinBox);
    fontSizeLayout->addStretch();
    m_propertiesLayout->addLayout(fontSizeLayout);

    // Font family control
    auto* fontFamilyLayout = new QHBoxLayout();
    m_fontFamilyLabel = new ElaText(tr("Font:"));
    fontFamilyLayout->addWidget(m_fontFamilyLabel);
    m_fontFamilyCombo = new ElaComboBox();
    m_fontFamilyCombo->addItems(QFontDatabase::families());
    m_fontFamilyCombo->setCurrentText("Arial");
    fontFamilyLayout->addWidget(m_fontFamilyCombo);
    m_propertiesLayout->addLayout(fontFamilyLayout);

    // Actions group
    m_actionsGroup = new ElaScrollPageArea(this);
    auto* actionsVLayout = new QVBoxLayout(m_actionsGroup);
    actionsVLayout->setContentsMargins(12, 8, 12, 12);

    m_actionsTitle = new ElaText(tr("Actions"), m_actionsGroup);
    m_actionsTitle->setTextPixelSize(14);
    actionsVLayout->addWidget(m_actionsTitle);

    auto* actionsContent = new QWidget(m_actionsGroup);
    m_actionsLayout = new QHBoxLayout(actionsContent);
    m_actionsLayout->setContentsMargins(0, 6, 0, 0);
    actionsVLayout->addWidget(actionsContent);

    m_clearAllBtn = new ElaPushButton(tr("Clear All"));
    m_clearAllBtn->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_DialogDiscardButton));
    m_clearAllBtn->setToolTip(tr("Clear all annotations"));

    m_saveBtn = new ElaPushButton(tr("Save"));
    m_saveBtn->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_saveBtn->setToolTip(tr("Save annotations to document"));

    m_loadBtn = new ElaPushButton(tr("Load"));
    m_loadBtn->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton));
    m_loadBtn->setToolTip(tr("Load annotations from document"));

    m_actionsLayout->addWidget(m_clearAllBtn);
    m_actionsLayout->addWidget(m_saveBtn);
    m_actionsLayout->addWidget(m_loadBtn);

    // Add groups to main layout
    mainLayout->addWidget(m_toolGroup);
    mainLayout->addWidget(m_propertiesGroup);
    mainLayout->addWidget(m_actionsGroup);
    mainLayout->addStretch();

    updateColorButton();
    updatePropertyControls();
}

void AnnotationToolbar::setupConnections() {
    // Tool selection
    connect(m_toolButtonGroup,
            QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this,
            &AnnotationToolbar::onToolButtonClicked);

    // Color selection
    connect(m_colorButton, &ElaPushButton::clicked, this,
            &AnnotationToolbar::onColorButtonClicked);

    // Property controls
    connect(m_opacitySlider, &QSlider::valueChanged, this,
            &AnnotationToolbar::onOpacitySliderChanged);
    connect(m_lineWidthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &AnnotationToolbar::onLineWidthChanged);
    connect(m_fontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &AnnotationToolbar::onFontSizeChanged);
    connect(m_fontFamilyCombo, &QComboBox::currentTextChanged, this,
            &AnnotationToolbar::onFontFamilyChanged);

    // Actions
    connect(m_clearAllBtn, &QPushButton::clicked, this,
            &AnnotationToolbar::clearAllAnnotations);
    connect(m_saveBtn, &QPushButton::clicked, this,
            &AnnotationToolbar::saveAnnotations);
    connect(m_loadBtn, &QPushButton::clicked, this,
            &AnnotationToolbar::loadAnnotations);
}

void AnnotationToolbar::setCurrentTool(AnnotationType tool) {
    if (m_currentTool != tool) {
        m_currentTool = tool;
        updateToolButtons();
        updatePropertyControls();
        emit toolChanged(tool);
    }
}

void AnnotationToolbar::setCurrentColor(const QColor& color) {
    if (m_currentColor != color) {
        m_currentColor = color;
        updateColorButton();
        emit colorChanged(color);
    }
}

void AnnotationToolbar::setCurrentOpacity(double opacity) {
    if (m_currentOpacity != opacity) {
        m_currentOpacity = opacity;
        m_opacitySlider->setValue(static_cast<int>(opacity * 100));
        m_opacityLabel->setText(
            tr("Opacity: %1%").arg(static_cast<int>(opacity * 100)));
        emit opacityChanged(opacity);
    }
}

void AnnotationToolbar::setCurrentLineWidth(double width) {
    if (m_currentLineWidth != width) {
        m_currentLineWidth = width;
        m_lineWidthSpinBox->setValue(static_cast<int>(width));
        emit lineWidthChanged(width);
    }
}

void AnnotationToolbar::setCurrentFontSize(int size) {
    if (m_currentFontSize != size) {
        m_currentFontSize = size;
        m_fontSizeSpinBox->setValue(size);
        emit fontSizeChanged(size);
    }
}

void AnnotationToolbar::setCurrentFontFamily(const QString& family) {
    if (m_currentFontFamily != family) {
        m_currentFontFamily = family;
        m_fontFamilyCombo->setCurrentText(family);
        emit fontFamilyChanged(family);
    }
}

void AnnotationToolbar::setEnabled(bool enabled) {
    QWidget::setEnabled(enabled);
}

void AnnotationToolbar::resetToDefaults() {
    setCurrentTool(AnnotationType::Highlight);
    setCurrentColor(Qt::yellow);
    setCurrentOpacity(0.7);
    setCurrentLineWidth(2.0);
    setCurrentFontSize(12);
    setCurrentFontFamily("Arial");
}

void AnnotationToolbar::onToolButtonClicked() {
    auto* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        int toolValue = button->property("tool").toInt();
        setCurrentTool(static_cast<AnnotationType>(toolValue));
    }
}

void AnnotationToolbar::onColorButtonClicked() {
    if (!m_colorDialog) {
        m_colorDialog = new QColorDialog(this);
        m_colorDialog->setOption(QColorDialog::ShowAlphaChannel, false);
    }

    m_colorDialog->setCurrentColor(m_currentColor);
    if (m_colorDialog->exec() == QDialog::Accepted) {
        setCurrentColor(m_colorDialog->currentColor());
    }
}

void AnnotationToolbar::onOpacitySliderChanged(int value) {
    double opacity = value / 100.0;
    setCurrentOpacity(opacity);
}

void AnnotationToolbar::onLineWidthChanged(int value) {
    setCurrentLineWidth(static_cast<double>(value));
}

void AnnotationToolbar::onFontSizeChanged(int size) {
    setCurrentFontSize(size);
}

void AnnotationToolbar::onFontFamilyChanged(const QString& family) {
    setCurrentFontFamily(family);
}

void AnnotationToolbar::updateToolButtons() {
    // Find and check the button corresponding to current tool
    for (QAbstractButton* button : m_toolButtonGroup->buttons()) {
        auto* pushButton = qobject_cast<QPushButton*>(button);
        if (pushButton) {
            int toolValue = pushButton->property("tool").toInt();
            pushButton->setChecked(static_cast<AnnotationType>(toolValue) ==
                                   m_currentTool);
        }
    }
}

void AnnotationToolbar::updateColorButton() {
    QString colorStyle =
        QString("background-color: %1; border: 1px solid gray;")
            .arg(m_currentColor.name());
    m_colorButton->setStyleSheet(colorStyle);
}

void AnnotationToolbar::updatePropertyControls() {
    // Show/hide controls based on current tool
    bool showLineWidth = (m_currentTool == AnnotationType::Rectangle ||
                          m_currentTool == AnnotationType::Circle ||
                          m_currentTool == AnnotationType::Line ||
                          m_currentTool == AnnotationType::Arrow ||
                          m_currentTool == AnnotationType::Ink);

    bool showFontControls = (m_currentTool == AnnotationType::FreeText ||
                             m_currentTool == AnnotationType::Note);

    m_lineWidthLabel->setVisible(showLineWidth);
    m_lineWidthSpinBox->setVisible(showLineWidth);

    m_fontSizeLabel->setVisible(showFontControls);
    m_fontSizeSpinBox->setVisible(showFontControls);
    m_fontFamilyLabel->setVisible(showFontControls);
    m_fontFamilyCombo->setVisible(showFontControls);
}

void AnnotationToolbar::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
}

void AnnotationToolbar::retranslateUi() {
    // Update section titles
    if (m_toolTitle) {
        m_toolTitle->setText(tr("Annotation Tools"));
    }
    if (m_propertiesTitle) {
        m_propertiesTitle->setText(tr("Properties"));
    }
    if (m_actionsTitle) {
        m_actionsTitle->setText(tr("Actions"));
    }

    // Update tool button texts and tooltips
    if (m_highlightBtn) {
        m_highlightBtn->setText(tr("Highlight"));
        m_highlightBtn->setToolTip(tr("Highlight text"));
    }
    if (m_noteBtn) {
        m_noteBtn->setText(tr("Note"));
        m_noteBtn->setToolTip(tr("Add note"));
    }
    if (m_freeTextBtn) {
        m_freeTextBtn->setText(tr("Text"));
        m_freeTextBtn->setToolTip(tr("Free text"));
    }
    if (m_underlineBtn) {
        m_underlineBtn->setText(tr("Underline"));
        m_underlineBtn->setToolTip(tr("Underline text"));
    }
    if (m_strikeOutBtn) {
        m_strikeOutBtn->setText(tr("Strikeout"));
        m_strikeOutBtn->setToolTip(tr("Strikeout text"));
    }
    if (m_rectangleBtn) {
        m_rectangleBtn->setText(tr("Rectangle"));
        m_rectangleBtn->setToolTip(tr("Draw rectangle"));
    }
    if (m_circleBtn) {
        m_circleBtn->setText(tr("Circle"));
        m_circleBtn->setToolTip(tr("Draw circle"));
    }
    if (m_lineBtn) {
        m_lineBtn->setText(tr("Line"));
        m_lineBtn->setToolTip(tr("Draw line"));
    }
    if (m_arrowBtn) {
        m_arrowBtn->setText(tr("Arrow"));
        m_arrowBtn->setToolTip(tr("Draw arrow"));
    }
    if (m_inkBtn) {
        m_inkBtn->setText(tr("Ink"));
        m_inkBtn->setToolTip(tr("Freehand drawing"));
    }

    // Update property labels
    if (m_opacityLabel) {
        m_opacityLabel->setText(
            tr("Opacity: %1%").arg(static_cast<int>(m_currentOpacity * 100)));
    }
    if (m_lineWidthLabel) {
        m_lineWidthLabel->setText(tr("Line Width:"));
    }
    if (m_fontSizeLabel) {
        m_fontSizeLabel->setText(tr("Font Size:"));
    }
    if (m_fontFamilyLabel) {
        m_fontFamilyLabel->setText(tr("Font:"));
    }

    // Update action button texts and tooltips
    if (m_clearAllBtn) {
        m_clearAllBtn->setText(tr("Clear All"));
        m_clearAllBtn->setToolTip(tr("Clear all annotations"));
    }
    if (m_saveBtn) {
        m_saveBtn->setText(tr("Save"));
        m_saveBtn->setToolTip(tr("Save annotations to document"));
    }
    if (m_loadBtn) {
        m_loadBtn->setText(tr("Load"));
        m_loadBtn->setToolTip(tr("Load annotations from document"));
    }
}
