#include "AnnotationToolbar.h"
#include <QApplication>
#include <QDebug>
#include <QFontDatabase>
#include <QStyle>

AnnotationToolbar::AnnotationToolbar(QWidget* parent)
    : QWidget(parent),
      m_toolGroup(nullptr),
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
}

void AnnotationToolbar::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(8);

    // Tool selection group
    m_toolGroup = new QGroupBox("注释工具", this);
    m_toolLayout = new QHBoxLayout(m_toolGroup);
    m_toolButtonGroup = new QButtonGroup(this);

    // Create tool buttons
    m_highlightBtn = new ElaPushButton("高亮");
    m_highlightBtn->setCheckable(true);
    m_highlightBtn->setToolTip("文本高亮");
    m_highlightBtn->setProperty("tool",
                                static_cast<int>(AnnotationType::Highlight));

    m_noteBtn = new ElaPushButton("便签");
    m_noteBtn->setCheckable(true);
    m_noteBtn->setToolTip("添加便签");
    m_noteBtn->setProperty("tool", static_cast<int>(AnnotationType::Note));

    m_freeTextBtn = new ElaPushButton("文本");
    m_freeTextBtn->setCheckable(true);
    m_freeTextBtn->setToolTip("自由文本");
    m_freeTextBtn->setProperty("tool",
                               static_cast<int>(AnnotationType::FreeText));

    m_underlineBtn = new ElaPushButton("下划线");
    m_underlineBtn->setCheckable(true);
    m_underlineBtn->setToolTip("文本下划线");
    m_underlineBtn->setProperty("tool",
                                static_cast<int>(AnnotationType::Underline));

    m_strikeOutBtn = new ElaPushButton("删除线");
    m_strikeOutBtn->setCheckable(true);
    m_strikeOutBtn->setToolTip("文本删除线");
    m_strikeOutBtn->setProperty("tool",
                                static_cast<int>(AnnotationType::StrikeOut));

    m_rectangleBtn = new ElaPushButton("矩形");
    m_rectangleBtn->setCheckable(true);
    m_rectangleBtn->setToolTip("绘制矩形");
    m_rectangleBtn->setProperty("tool",
                                static_cast<int>(AnnotationType::Rectangle));

    m_circleBtn = new ElaPushButton("圆形");
    m_circleBtn->setCheckable(true);
    m_circleBtn->setToolTip("绘制圆形");
    m_circleBtn->setProperty("tool", static_cast<int>(AnnotationType::Circle));

    m_lineBtn = new ElaPushButton("直线");
    m_lineBtn->setCheckable(true);
    m_lineBtn->setToolTip("绘制直线");
    m_lineBtn->setProperty("tool", static_cast<int>(AnnotationType::Line));

    m_arrowBtn = new ElaPushButton("箭头");
    m_arrowBtn->setCheckable(true);
    m_arrowBtn->setToolTip("绘制箭头");
    m_arrowBtn->setProperty("tool", static_cast<int>(AnnotationType::Arrow));

    m_inkBtn = new ElaPushButton("手绘");
    m_inkBtn->setCheckable(true);
    m_inkBtn->setToolTip("自由手绘");
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
    m_propertiesGroup = new QGroupBox("属性", this);
    m_propertiesLayout = new QVBoxLayout(m_propertiesGroup);

    // Color selection
    auto* colorLayout = new QHBoxLayout();
    colorLayout->addWidget(new ElaText("颜色:"));
    m_colorButton = new ElaPushButton();
    m_colorButton->setMinimumSize(40, 25);
    m_colorButton->setMaximumSize(40, 25);
    m_colorButton->setStyleSheet("border: 1px solid gray;");
    colorLayout->addWidget(m_colorButton);
    colorLayout->addStretch();
    m_propertiesLayout->addLayout(colorLayout);

    // Opacity control
    auto* opacityLayout = new QHBoxLayout();
    m_opacityLabel = new ElaText("透明度: 70%");
    opacityLayout->addWidget(m_opacityLabel);
    m_opacitySlider = new ElaSlider(Qt::Horizontal);
    m_opacitySlider->setRange(10, 100);
    m_opacitySlider->setValue(70);
    opacityLayout->addWidget(m_opacitySlider);
    m_propertiesLayout->addLayout(opacityLayout);

    // Line width control
    auto* lineWidthLayout = new QHBoxLayout();
    m_lineWidthLabel = new ElaText("线宽:");
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
    m_fontSizeLabel = new ElaText("字号:");
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
    m_fontFamilyLabel = new ElaText("字体:");
    fontFamilyLayout->addWidget(m_fontFamilyLabel);
    m_fontFamilyCombo = new ElaComboBox();
    m_fontFamilyCombo->addItems(QFontDatabase::families());
    m_fontFamilyCombo->setCurrentText("Arial");
    fontFamilyLayout->addWidget(m_fontFamilyCombo);
    m_propertiesLayout->addLayout(fontFamilyLayout);

    // Actions group
    m_actionsGroup = new QGroupBox("操作", this);
    m_actionsLayout = new QHBoxLayout(m_actionsGroup);

    m_clearAllBtn = new ElaPushButton("清除全部");
    m_clearAllBtn->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_DialogDiscardButton));
    m_clearAllBtn->setToolTip("清除所有注释");

    m_saveBtn = new ElaPushButton("保存");
    m_saveBtn->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_saveBtn->setToolTip("保存注释到文档");

    m_loadBtn = new ElaPushButton("加载");
    m_loadBtn->setIcon(
        QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton));
    m_loadBtn->setToolTip("从文档加载注释");

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
            QString("透明度: %1%").arg(static_cast<int>(opacity * 100)));
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
