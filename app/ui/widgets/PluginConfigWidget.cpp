#include "PluginConfigWidget.h"

#include <QColorDialog>
#include <QEvent>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QTextEdit>
#include <QVBoxLayout>

#include "ElaComboBox.h"
#include "ElaDoubleSpinBox.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaSlider.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ElaToggleSwitch.h"

// ============================================================================
// Construction / Destruction
// ============================================================================

PluginConfigWidget::PluginConfigWidget(QWidget* parent)
    : QWidget(parent),
      m_mainLayout(nullptr),
      m_scrollArea(nullptr),
      m_scrollContent(nullptr),
      m_contentLayout(nullptr),
      m_model(nullptr),
      m_requiredOnly(false),
      m_showAdvanced(true),
      m_rebuildingUI(false),
      m_searchFilter() {
    setupUI();
}

PluginConfigWidget::~PluginConfigWidget() = default;

// ============================================================================
// Public Methods
// ============================================================================

void PluginConfigWidget::setModel(PluginConfigModel* model) {
    if (m_model == model) {
        return;
    }

    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
    }

    m_model = model;

    if (m_model) {
        connect(m_model, &PluginConfigModel::configurationChanged, this,
                &PluginConfigWidget::rebuildUI);
    }

    rebuildUI();
}

void PluginConfigWidget::rebuildUI() {
    if (m_rebuildingUI) {
        return;
    }

    m_rebuildingUI = true;
    clearUI();

    if (!m_model) {
        m_rebuildingUI = false;
        return;
    }

    // Get groups from model
    QList<PluginConfigModel::ConfigGroup> groups = m_model->getGroups();

    // If no groups defined, create a default one
    if (groups.isEmpty()) {
        PluginConfigModel::ConfigGroup defaultGroup("general", tr("General"));
        groups.append(defaultGroup);
    }

    // Create UI for each group
    for (const PluginConfigModel::ConfigGroup& group : groups) {
        // Skip advanced groups if not showing
        if (group.isAdvanced && !m_showAdvanced) {
            continue;
        }

        // Get entries for this group
        QVector<PluginConfigModel::ConfigEntry> entries =
            m_model->getEntriesForGroup(group.id);

        // Filter for required only if needed
        if (m_requiredOnly) {
            QVector<PluginConfigModel::ConfigEntry> filtered;
            for (const auto& entry : entries) {
                if (entry.isRequired) {
                    filtered.append(entry);
                }
            }
            entries = filtered;
        }

        // Skip empty groups
        if (entries.isEmpty()) {
            continue;
        }

        // Create group widget
        QWidget* groupWidget = createGroupWidget(group, entries);
        m_contentLayout->addWidget(groupWidget);
    }

    m_contentLayout->addStretch();

    m_rebuildingUI = false;
    emit validationStateChanged(isValid());
}

void PluginConfigWidget::setRequiredOnly(bool requiredOnly) {
    if (m_requiredOnly == requiredOnly) {
        return;
    }
    m_requiredOnly = requiredOnly;
    rebuildUI();
}

void PluginConfigWidget::setShowAdvanced(bool showAdvanced) {
    if (m_showAdvanced == showAdvanced) {
        return;
    }
    m_showAdvanced = showAdvanced;
    rebuildUI();
}

QStringList PluginConfigWidget::getValidationErrors() const {
    if (!m_model) {
        return QStringList();
    }
    return m_model->validateAllEntries();
}

bool PluginConfigWidget::isValid() const {
    return getValidationErrors().isEmpty();
}

void PluginConfigWidget::applyToModel() {
    if (!m_model) {
        return;
    }

    // Values are already being applied in real-time through slots
    // Just save the configuration
    m_model->saveConfiguration();
}

void PluginConfigWidget::resetToDefaults() {
    if (!m_model) {
        return;
    }

    m_model->resetToDefaults();
    rebuildUI();
}

// ============================================================================
// Protected Methods
// ============================================================================

void PluginConfigWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUI();
    }
    QWidget::changeEvent(event);
}

// ============================================================================
// Private Slots
// ============================================================================

void PluginConfigWidget::onBoolValueChanged(bool value) {
    auto* toggle = qobject_cast<ElaToggleSwitch*>(sender());
    if (!toggle)
        return;

    QString key = toggle->property("configKey").toString();
    updateModelValue(key, value);
}

void PluginConfigWidget::onIntValueChanged(int value) {
    QWidget* widget = qobject_cast<QWidget*>(sender());
    if (!widget)
        return;

    QString key = widget->property("configKey").toString();
    updateModelValue(key, value);
}

void PluginConfigWidget::onDoubleValueChanged(double value) {
    auto* spinBox = qobject_cast<ElaDoubleSpinBox*>(sender());
    if (!spinBox)
        return;

    QString key = spinBox->property("configKey").toString();
    updateModelValue(key, value);
}

void PluginConfigWidget::onStringValueChanged(const QString& value) {
    auto* lineEdit = qobject_cast<ElaLineEdit*>(sender());
    if (!lineEdit)
        return;

    QString key = lineEdit->property("configKey").toString();
    updateModelValue(key, value);
}

void PluginConfigWidget::onEnumValueChanged(int index) {
    auto* comboBox = qobject_cast<ElaComboBox*>(sender());
    if (!comboBox)
        return;

    QString key = comboBox->property("configKey").toString();
    QString value = comboBox->itemText(index);
    updateModelValue(key, value);
}

void PluginConfigWidget::onPathBrowseClicked() {
    auto* button = qobject_cast<ElaPushButton*>(sender());
    if (!button)
        return;

    QString key = button->property("configKey").toString();
    auto* lineEdit = qobject_cast<ElaLineEdit*>(m_editors.value(key));
    if (!lineEdit)
        return;

    QString currentPath = lineEdit->text();
    QString path = QFileDialog::getExistingDirectory(
        this, tr("Select Directory"), currentPath);

    if (!path.isEmpty()) {
        lineEdit->setText(path);
        updateModelValue(key, path);
    }
}

void PluginConfigWidget::onColorPickerClicked() {
    auto* button = qobject_cast<ElaPushButton*>(sender());
    if (!button)
        return;

    QString key = button->property("configKey").toString();

    QColor currentColor;
    if (m_model) {
        currentColor = QColor(m_model->getValue(key).toString());
    }

    QColor color =
        QColorDialog::getColor(currentColor, this, tr("Select Color"));
    if (color.isValid()) {
        button->setStyleSheet(QString("background-color: %1; border: 1px solid "
                                      "#ccc; border-radius: 4px;")
                                  .arg(color.name()));

        // Update the color label if it exists
        QWidget* parent = button->parentWidget();
        if (parent) {
            auto* colorLabel = parent->findChild<ElaText*>();
            if (colorLabel) {
                colorLabel->setText(color.name());
            }
        }

        updateModelValue(key, color.name());
    }
}

// ============================================================================
// Private Methods
// ============================================================================

void PluginConfigWidget::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_scrollContent = new QWidget(m_scrollArea);
    m_contentLayout = new QVBoxLayout(m_scrollContent);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(12);

    m_scrollArea->setWidget(m_scrollContent);
    m_mainLayout->addWidget(m_scrollArea);
}

void PluginConfigWidget::clearUI() {
    // Clear editors tracking
    m_editors.clear();
    m_labels.clear();
    m_errorLabels.clear();

    // Remove all widgets from content layout
    while (QLayoutItem* item = m_contentLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

void PluginConfigWidget::retranslateUI() { rebuildUI(); }

void PluginConfigWidget::updateValidationDisplay() {
    if (!m_model) {
        return;
    }

    // Get all validation errors
    QStringList errors = m_model->validateAllEntries();

    // Build a map of key -> error message
    QHash<QString, QString> errorMap;
    for (const QString& error : errors) {
        // Parse error to find which key it relates to
        for (auto it = m_editors.begin(); it != m_editors.end(); ++it) {
            QString key = it.key();
            QString displayName = key;

            // Get display name from model if available
            for (int i = 0; i < m_model->rowCount(); ++i) {
                if (m_model->getEntry(i).key == key) {
                    displayName = m_model->getEntry(i).displayName;
                    if (displayName.isEmpty()) {
                        displayName = key;
                    }
                    break;
                }
            }

            if (error.contains(key) || error.contains(displayName)) {
                errorMap[key] = error;
                break;
            }
        }
    }

    // Update error labels
    for (auto it = m_errorLabels.begin(); it != m_errorLabels.end(); ++it) {
        QString key = it.key();
        QLabel* errorLabel = it.value();

        if (errorMap.contains(key)) {
            errorLabel->setText(errorMap[key]);
            errorLabel->setStyleSheet("color: #e74c3c; font-size: 11px;");
            errorLabel->setVisible(true);

            // Also highlight the label
            if (m_labels.contains(key)) {
                m_labels[key]->setStyleSheet("color: #e74c3c;");
            }
        } else {
            errorLabel->clear();
            errorLabel->setVisible(false);

            // Reset label style
            if (m_labels.contains(key)) {
                m_labels[key]->setStyleSheet("");
            }
        }
    }
}

void PluginConfigWidget::setSearchFilter(const QString& searchText) {
    if (m_searchFilter == searchText) {
        return;
    }
    m_searchFilter = searchText.trimmed().toLower();

    // Show/hide entries based on search filter
    for (auto it = m_editors.begin(); it != m_editors.end(); ++it) {
        QString key = it.key();
        QWidget* editor = it.value();

        bool visible = true;
        if (!m_searchFilter.isEmpty()) {
            // Check if key or display name matches
            visible = key.toLower().contains(m_searchFilter);

            // Also check display name from label
            if (!visible && m_labels.contains(key)) {
                QString labelText = m_labels[key]->text().toLower();
                visible = labelText.contains(m_searchFilter);
            }
        }

        // Find the row container and show/hide it
        QWidget* container = editor->parentWidget();
        while (container && container != m_scrollContent) {
            container = container->parentWidget();
        }

        // Show/hide the editor and its label
        if (editor) {
            editor->setVisible(visible);
        }
        if (m_labels.contains(key)) {
            m_labels[key]->setVisible(visible);
        }
        if (m_errorLabels.contains(key)) {
            m_errorLabels[key]->setVisible(
                visible && !m_errorLabels[key]->text().isEmpty());
        }
    }
}

QWidget* PluginConfigWidget::createEditorForEntry(
    const PluginConfigModel::ConfigEntry& entry) {
    if (entry.type == "bool") {
        return createBoolEditor(entry);
    } else if (entry.type == "int") {
        return createIntEditor(entry);
    } else if (entry.type == "double") {
        return createDoubleEditor(entry);
    } else if (entry.type == "enum") {
        return createEnumEditor(entry);
    } else if (entry.type == "path") {
        return createPathEditor(entry);
    } else if (entry.type == "file") {
        return createFileEditor(entry);
    } else if (entry.type == "color") {
        return createColorEditor(entry);
    } else if (entry.type == "text" || entry.type == "textarea") {
        return createTextAreaEditor(entry);
    } else {
        // Default to string editor
        return createStringEditor(entry);
    }
}

QWidget* PluginConfigWidget::createBoolEditor(
    const PluginConfigModel::ConfigEntry& entry) {
    auto* toggle = new ElaToggleSwitch(this);
    toggle->setProperty("configKey", entry.key);
    toggle->setIsToggled(entry.value.toBool());
    toggle->setEnabled(!entry.isReadOnly);

    connect(toggle, &ElaToggleSwitch::toggled, this,
            &PluginConfigWidget::onBoolValueChanged);

    m_editors[entry.key] = toggle;
    return toggle;
}

QWidget* PluginConfigWidget::createIntEditor(
    const PluginConfigModel::ConfigEntry& entry) {
    // Use slider if range is defined and reasonable
    bool useSlider = !entry.minValue.isNull() && !entry.maxValue.isNull() &&
                     (entry.maxValue.toInt() - entry.minValue.toInt()) <= 1000;

    if (useSlider) {
        auto* container = new QWidget(this);
        auto* layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);

        auto* slider = new ElaSlider(Qt::Horizontal, container);
        slider->setProperty("configKey", entry.key);
        slider->setMinimum(entry.minValue.toInt());
        slider->setMaximum(entry.maxValue.toInt());
        slider->setValue(entry.value.toInt());
        slider->setEnabled(!entry.isReadOnly);

        auto* valueLabel =
            new ElaText(QString::number(entry.value.toInt()), container);
        valueLabel->setFixedWidth(50);

        connect(slider, &ElaSlider::valueChanged, this,
                [this, valueLabel](int value) {
                    valueLabel->setText(QString::number(value));
                    onIntValueChanged(value);
                });

        layout->addWidget(slider, 1);
        layout->addWidget(valueLabel);

        m_editors[entry.key] = slider;
        return container;
    } else {
        auto* spinBox = new ElaSpinBox(this);
        spinBox->setProperty("configKey", entry.key);
        spinBox->setValue(entry.value.toInt());
        spinBox->setEnabled(!entry.isReadOnly);

        if (!entry.minValue.isNull()) {
            spinBox->setMinimum(entry.minValue.toInt());
        } else {
            spinBox->setMinimum(INT_MIN);
        }

        if (!entry.maxValue.isNull()) {
            spinBox->setMaximum(entry.maxValue.toInt());
        } else {
            spinBox->setMaximum(INT_MAX);
        }

        connect(spinBox, QOverload<int>::of(&ElaSpinBox::valueChanged), this,
                &PluginConfigWidget::onIntValueChanged);

        m_editors[entry.key] = spinBox;
        return spinBox;
    }
}

QWidget* PluginConfigWidget::createDoubleEditor(
    const PluginConfigModel::ConfigEntry& entry) {
    auto* spinBox = new ElaDoubleSpinBox(this);
    spinBox->setProperty("configKey", entry.key);
    spinBox->setValue(entry.value.toDouble());
    spinBox->setEnabled(!entry.isReadOnly);
    spinBox->setDecimals(2);

    if (!entry.minValue.isNull()) {
        spinBox->setMinimum(entry.minValue.toDouble());
    }

    if (!entry.maxValue.isNull()) {
        spinBox->setMaximum(entry.maxValue.toDouble());
    }

    connect(spinBox, QOverload<double>::of(&ElaDoubleSpinBox::valueChanged),
            this, &PluginConfigWidget::onDoubleValueChanged);

    m_editors[entry.key] = spinBox;
    return spinBox;
}

QWidget* PluginConfigWidget::createStringEditor(
    const PluginConfigModel::ConfigEntry& entry) {
    auto* lineEdit = new ElaLineEdit(this);
    lineEdit->setProperty("configKey", entry.key);
    lineEdit->setText(entry.value.toString());
    lineEdit->setEnabled(!entry.isReadOnly);

    if (!entry.placeholder.isEmpty()) {
        lineEdit->setPlaceholderText(entry.placeholder);
    }

    connect(lineEdit, &ElaLineEdit::textChanged, this,
            &PluginConfigWidget::onStringValueChanged);

    m_editors[entry.key] = lineEdit;
    return lineEdit;
}

QWidget* PluginConfigWidget::createTextAreaEditor(
    const PluginConfigModel::ConfigEntry& entry) {
    auto* textEdit = new QTextEdit(this);
    textEdit->setProperty("configKey", entry.key);
    textEdit->setPlainText(entry.value.toString());
    textEdit->setReadOnly(entry.isReadOnly);
    textEdit->setMinimumHeight(80);
    textEdit->setMaximumHeight(150);

    if (!entry.placeholder.isEmpty()) {
        textEdit->setPlaceholderText(entry.placeholder);
    }

    connect(textEdit, &QTextEdit::textChanged, this, [this, textEdit]() {
        QString key = textEdit->property("configKey").toString();
        updateModelValue(key, textEdit->toPlainText());
    });

    m_editors[entry.key] = textEdit;
    return textEdit;
}

QWidget* PluginConfigWidget::createEnumEditor(
    const PluginConfigModel::ConfigEntry& entry) {
    auto* comboBox = new ElaComboBox(this);
    comboBox->setProperty("configKey", entry.key);
    comboBox->addItems(entry.enumValues);
    comboBox->setEnabled(!entry.isReadOnly);

    // Set current value
    int idx = entry.enumValues.indexOf(entry.value.toString());
    if (idx >= 0) {
        comboBox->setCurrentIndex(idx);
    }

    connect(comboBox, QOverload<int>::of(&ElaComboBox::currentIndexChanged),
            this, &PluginConfigWidget::onEnumValueChanged);

    m_editors[entry.key] = comboBox;
    return comboBox;
}

QWidget* PluginConfigWidget::createPathEditor(
    const PluginConfigModel::ConfigEntry& entry) {
    auto* container = new QWidget(this);
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* lineEdit = new ElaLineEdit(container);
    lineEdit->setProperty("configKey", entry.key);
    lineEdit->setText(entry.value.toString());
    lineEdit->setEnabled(!entry.isReadOnly);

    if (!entry.placeholder.isEmpty()) {
        lineEdit->setPlaceholderText(entry.placeholder);
    } else {
        lineEdit->setPlaceholderText(tr("Select path..."));
    }

    auto* browseBtn = new ElaPushButton(tr("Browse"), container);
    browseBtn->setProperty("configKey", entry.key);
    browseBtn->setEnabled(!entry.isReadOnly);
    browseBtn->setFixedWidth(80);

    connect(lineEdit, &ElaLineEdit::textChanged, this,
            &PluginConfigWidget::onStringValueChanged);
    connect(browseBtn, &ElaPushButton::clicked, this,
            &PluginConfigWidget::onPathBrowseClicked);

    layout->addWidget(lineEdit, 1);
    layout->addWidget(browseBtn);

    m_editors[entry.key] = lineEdit;
    return container;
}

QWidget* PluginConfigWidget::createFileEditor(
    const PluginConfigModel::ConfigEntry& entry) {
    auto* container = new QWidget(this);
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* lineEdit = new ElaLineEdit(container);
    lineEdit->setProperty("configKey", entry.key);
    lineEdit->setText(entry.value.toString());
    lineEdit->setEnabled(!entry.isReadOnly);

    if (!entry.placeholder.isEmpty()) {
        lineEdit->setPlaceholderText(entry.placeholder);
    } else {
        lineEdit->setPlaceholderText(tr("Select file..."));
    }

    auto* browseBtn = new ElaPushButton(tr("Browse"), container);
    browseBtn->setProperty("configKey", entry.key);
    browseBtn->setEnabled(!entry.isReadOnly);
    browseBtn->setFixedWidth(80);

    connect(lineEdit, &ElaLineEdit::textChanged, this,
            &PluginConfigWidget::onStringValueChanged);

    connect(browseBtn, &ElaPushButton::clicked, this, [this, lineEdit]() {
        QString currentPath = lineEdit->text();
        QString filter = tr("All Files (*)");
        QString path = QFileDialog::getOpenFileName(this, tr("Select File"),
                                                    currentPath, filter);
        if (!path.isEmpty()) {
            lineEdit->setText(path);
        }
    });

    layout->addWidget(lineEdit, 1);
    layout->addWidget(browseBtn);

    m_editors[entry.key] = lineEdit;
    return container;
}

QWidget* PluginConfigWidget::createColorEditor(
    const PluginConfigModel::ConfigEntry& entry) {
    auto* container = new QWidget(this);
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* colorBtn = new ElaPushButton(container);
    colorBtn->setProperty("configKey", entry.key);
    colorBtn->setFixedSize(40, 30);
    colorBtn->setEnabled(!entry.isReadOnly);
    colorBtn->setCursor(Qt::PointingHandCursor);

    QColor color(entry.value.toString());
    QString colorStyle =
        "background-color: %1; border: 1px solid #ccc; border-radius: 4px;";
    if (color.isValid()) {
        colorBtn->setStyleSheet(colorStyle.arg(color.name()));
    } else {
        colorBtn->setStyleSheet(colorStyle.arg("#ffffff"));
    }

    auto* colorLabel =
        new ElaText(color.isValid() ? color.name() : tr("No color"), container);
    colorLabel->setTextPixelSize(12);

    connect(colorBtn, &ElaPushButton::clicked, this,
            &PluginConfigWidget::onColorPickerClicked);

    layout->addWidget(colorBtn);
    layout->addWidget(colorLabel, 1);

    m_editors[entry.key] = colorBtn;
    return container;
}

QWidget* PluginConfigWidget::createGroupWidget(
    const PluginConfigModel::ConfigGroup& group,
    const QVector<PluginConfigModel::ConfigEntry>& entries) {
    auto* area = new ElaScrollPageArea(this);
    auto* layout = new QVBoxLayout(area);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);

    // Group title
    auto* title = new ElaText(group.displayName, area);
    title->setTextPixelSize(14);
    layout->addWidget(title);

    // Group description
    if (!group.description.isEmpty()) {
        auto* desc = new ElaText(group.description, area);
        desc->setTextPixelSize(11);
        layout->addWidget(desc);
    }

    // Create entries
    auto* entriesWidget = new QWidget(area);
    auto* entriesLayout = new QGridLayout(entriesWidget);
    entriesLayout->setContentsMargins(0, 8, 0, 0);
    entriesLayout->setSpacing(10);
    entriesLayout->setColumnStretch(1, 1);

    int row = 0;
    for (const PluginConfigModel::ConfigEntry& entry : entries) {
        // Label
        QString labelText =
            entry.displayName.isEmpty() ? entry.key : entry.displayName;
        if (entry.isRequired) {
            labelText += " *";
        }

        auto* label = new QLabel(labelText, entriesWidget);
        label->setToolTip(entry.description);
        m_labels[entry.key] = label;

        // Editor container (editor + error label)
        auto* editorContainer = new QWidget(entriesWidget);
        auto* editorLayout = new QVBoxLayout(editorContainer);
        editorLayout->setContentsMargins(0, 0, 0, 0);
        editorLayout->setSpacing(2);

        // Editor
        QWidget* editor = createEditorForEntry(entry);
        editorLayout->addWidget(editor);

        // Error label (hidden by default)
        auto* errorLabel = new QLabel(editorContainer);
        errorLabel->setStyleSheet("color: #e74c3c; font-size: 11px;");
        errorLabel->setWordWrap(true);
        errorLabel->setVisible(false);
        editorLayout->addWidget(errorLabel);
        m_errorLabels[entry.key] = errorLabel;

        // Description (as tooltip on editor)
        if (!entry.description.isEmpty() && editor) {
            editor->setToolTip(entry.description);
        }

        entriesLayout->addWidget(label, row, 0, Qt::AlignTop);
        entriesLayout->addWidget(editorContainer, row, 1);

        row++;
    }

    layout->addWidget(entriesWidget);

    return area;
}

void PluginConfigWidget::updateModelValue(const QString& key,
                                          const QVariant& value) {
    if (!m_model || m_rebuildingUI) {
        return;
    }

    m_model->setValue(key, value);
    updateValidationDisplay();
    emit configurationChanged();
    emit validationStateChanged(isValid());
}

QVariant PluginConfigWidget::getEditorValue(const QString& key) const {
    QWidget* editor = m_editors.value(key);
    if (!editor) {
        return QVariant();
    }

    if (auto* toggle = qobject_cast<ElaToggleSwitch*>(editor)) {
        return toggle->getIsToggled();
    }
    if (auto* spinBox = qobject_cast<ElaSpinBox*>(editor)) {
        return spinBox->value();
    }
    if (auto* dblSpinBox = qobject_cast<ElaDoubleSpinBox*>(editor)) {
        return dblSpinBox->value();
    }
    if (auto* lineEdit = qobject_cast<ElaLineEdit*>(editor)) {
        return lineEdit->text();
    }
    if (auto* comboBox = qobject_cast<ElaComboBox*>(editor)) {
        return comboBox->currentText();
    }
    if (auto* slider = qobject_cast<ElaSlider*>(editor)) {
        return slider->value();
    }

    return QVariant();
}
