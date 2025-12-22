#include "PluginConfigDialog.h"

#include <QCloseEvent>
#include <QEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

#include "ElaCheckBox.h"
#include "ElaLineEdit.h"
#include "ElaMessageBar.h"
#include "ElaPushButton.h"
#include "ElaText.h"

#include "controller/ServiceLocator.h"
#include "model/PluginConfigModel.h"
#include "plugin/PluginManager.h"
#include "ui/widgets/PluginConfigWidget.h"

// ============================================================================
// Construction / Destruction
// ============================================================================

PluginConfigDialog::PluginConfigDialog(const QString& pluginName,
                                       QWidget* parent)
    : QDialog(parent),
      m_pluginName(pluginName),
      m_pluginManager(nullptr),
      m_configModel(nullptr),
      m_headerWidget(nullptr),
      m_pluginNameLabel(nullptr),
      m_pluginVersionLabel(nullptr),
      m_pluginDescriptionLabel(nullptr),
      m_searchEdit(nullptr),
      m_configWidget(nullptr),
      m_showAdvancedCheck(nullptr),
      m_buttonWidget(nullptr),
      m_saveBtn(nullptr),
      m_resetBtn(nullptr),
      m_importBtn(nullptr),
      m_exportBtn(nullptr),
      m_cancelBtn(nullptr),
      m_validationLabel(nullptr),
      m_hasUnsavedChanges(false) {
    // Get PluginManager
    m_pluginManager = ServiceLocator::instance().getService<PluginManager>();

    // Create config model
    if (m_pluginManager) {
        m_configModel =
            std::make_unique<PluginConfigModel>(m_pluginManager, pluginName);

        // Load schema if available
        QJsonObject metadata =
            m_pluginManager->getPluginMetadata(pluginName).configuration;
        if (metadata.contains("configSchema")) {
            m_configModel->setConfigSchema(metadata["configSchema"].toObject());
        } else if (!metadata.isEmpty()) {
            // Use configuration as simple schema
            m_configModel->setConfigSchema(metadata);
        }
    }

    setupUI();
    loadPluginInfo();
    retranslateUI();

    // Set dialog properties
    setWindowTitle(tr("Plugin Configuration - %1").arg(pluginName));
    setMinimumSize(500, 400);
    resize(600, 500);
}

PluginConfigDialog::~PluginConfigDialog() = default;

// ============================================================================
// Public Methods
// ============================================================================

bool PluginConfigDialog::isModified() const {
    return m_configModel && m_configModel->isModified();
}

bool PluginConfigDialog::showConfigDialog(const QString& pluginName,
                                          QWidget* parent) {
    PluginConfigDialog dialog(pluginName, parent);
    return dialog.exec() == QDialog::Accepted;
}

// ============================================================================
// Protected Methods
// ============================================================================

void PluginConfigDialog::closeEvent(QCloseEvent* event) {
    if (m_hasUnsavedChanges) {
        if (!confirmUnsavedChanges()) {
            event->ignore();
            return;
        }
    }
    event->accept();
}

void PluginConfigDialog::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUI();
    }
    QDialog::changeEvent(event);
}

// ============================================================================
// Private Slots
// ============================================================================

void PluginConfigDialog::onSaveClicked() {
    // Validate first
    QStringList errors = m_configWidget->getValidationErrors();
    if (!errors.isEmpty()) {
        ElaMessageBar::error(ElaMessageBarType::TopRight,
                             tr("Validation Error"), errors.first(), 3000,
                             this);
        return;
    }

    // Apply and save
    m_configWidget->applyToModel();
    m_hasUnsavedChanges = false;

    ElaMessageBar::success(ElaMessageBarType::TopRight, tr("Success"),
                           tr("Configuration saved successfully"), 2000, this);

    accept();
}

void PluginConfigDialog::onResetClicked() {
    QMessageBox::StandardButton reply =
        QMessageBox::question(this, tr("Reset Configuration"),
                              tr("Are you sure you want to reset all settings "
                                 "to their default values?"),
                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_configWidget->resetToDefaults();
        m_hasUnsavedChanges = true;
        ElaMessageBar::information(ElaMessageBarType::TopRight, tr("Reset"),
                                   tr("Configuration reset to defaults"), 2000,
                                   this);
    }
}

void PluginConfigDialog::onImportClicked() {
    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Import Configuration"), QString(),
        tr("JSON Files (*.json);;All Files (*)"));

    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        ElaMessageBar::error(ElaMessageBarType::TopRight, tr("Error"),
                             tr("Failed to open file"), 3000, this);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        ElaMessageBar::error(ElaMessageBarType::TopRight, tr("Error"),
                             tr("Invalid configuration file"), 3000, this);
        return;
    }

    m_configModel->setConfiguration(doc.object());
    m_configWidget->rebuildUI();
    m_hasUnsavedChanges = true;

    ElaMessageBar::success(ElaMessageBarType::TopRight, tr("Success"),
                           tr("Configuration imported"), 2000, this);
}

void PluginConfigDialog::onExportClicked() {
    QString filePath = QFileDialog::getSaveFileName(
        this, tr("Export Configuration"),
        QString("%1_config.json").arg(m_pluginName),
        tr("JSON Files (*.json);;All Files (*)"));

    if (filePath.isEmpty()) {
        return;
    }

    QJsonObject config = m_configModel->getConfiguration();
    QJsonDocument doc(config);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        ElaMessageBar::error(ElaMessageBarType::TopRight, tr("Error"),
                             tr("Failed to save file"), 3000, this);
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));

    ElaMessageBar::success(ElaMessageBarType::TopRight, tr("Success"),
                           tr("Configuration exported"), 2000, this);
}

void PluginConfigDialog::onShowAdvancedToggled(bool checked) {
    m_configWidget->setShowAdvanced(checked);
}

void PluginConfigDialog::onConfigurationChanged() {
    m_hasUnsavedChanges = true;
    updateValidationDisplay();
}

void PluginConfigDialog::onValidationStateChanged(bool isValid) {
    m_saveBtn->setEnabled(isValid);
    updateValidationDisplay();
}

// ============================================================================
// Private Methods
// ============================================================================

void PluginConfigDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);

    setupHeader();
    mainLayout->addWidget(m_headerWidget);

    // Search and options bar
    auto* optionsBar = new QWidget(this);
    auto* optionsLayout = new QHBoxLayout(optionsBar);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(12);

    // Search field
    m_searchEdit = new ElaLineEdit(optionsBar);
    m_searchEdit->setPlaceholderText(tr("Search settings..."));
    m_searchEdit->setClearButtonEnabled(true);
    connect(
        m_searchEdit, &ElaLineEdit::textChanged, this,
        [this](const QString& text) { m_configWidget->setSearchFilter(text); });
    optionsLayout->addWidget(m_searchEdit, 1);

    // Advanced toggle
    m_showAdvancedCheck = new ElaCheckBox(tr("Show advanced"), optionsBar);
    m_showAdvancedCheck->setChecked(true);
    connect(m_showAdvancedCheck, &ElaCheckBox::toggled, this,
            &PluginConfigDialog::onShowAdvancedToggled);
    optionsLayout->addWidget(m_showAdvancedCheck);

    mainLayout->addWidget(optionsBar);

    setupConfigWidget();
    mainLayout->addWidget(m_configWidget, 1);

    // Validation label
    m_validationLabel = new ElaText("", this);
    m_validationLabel->setTextPixelSize(11);
    m_validationLabel->setVisible(false);
    mainLayout->addWidget(m_validationLabel);

    setupButtons();
    mainLayout->addWidget(m_buttonWidget);
}

void PluginConfigDialog::setupHeader() {
    m_headerWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(m_headerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    m_pluginNameLabel = new ElaText("", m_headerWidget);
    m_pluginNameLabel->setTextPixelSize(18);
    layout->addWidget(m_pluginNameLabel);

    m_pluginVersionLabel = new ElaText("", m_headerWidget);
    m_pluginVersionLabel->setTextPixelSize(12);
    layout->addWidget(m_pluginVersionLabel);

    m_pluginDescriptionLabel = new ElaText("", m_headerWidget);
    m_pluginDescriptionLabel->setTextPixelSize(11);
    m_pluginDescriptionLabel->setWordWrap(true);
    layout->addWidget(m_pluginDescriptionLabel);
}

void PluginConfigDialog::setupConfigWidget() {
    m_configWidget = new PluginConfigWidget(this);
    m_configWidget->setModel(m_configModel.get());

    connect(m_configWidget, &PluginConfigWidget::configurationChanged, this,
            &PluginConfigDialog::onConfigurationChanged);
    connect(m_configWidget, &PluginConfigWidget::validationStateChanged, this,
            &PluginConfigDialog::onValidationStateChanged);
}

void PluginConfigDialog::setupButtons() {
    m_buttonWidget = new QWidget(this);
    auto* layout = new QHBoxLayout(m_buttonWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_importBtn = new ElaPushButton(tr("Import"), m_buttonWidget);
    m_importBtn->setFixedWidth(80);
    connect(m_importBtn, &ElaPushButton::clicked, this,
            &PluginConfigDialog::onImportClicked);
    layout->addWidget(m_importBtn);

    m_exportBtn = new ElaPushButton(tr("Export"), m_buttonWidget);
    m_exportBtn->setFixedWidth(80);
    connect(m_exportBtn, &ElaPushButton::clicked, this,
            &PluginConfigDialog::onExportClicked);
    layout->addWidget(m_exportBtn);

    layout->addStretch();

    m_resetBtn = new ElaPushButton(tr("Reset"), m_buttonWidget);
    m_resetBtn->setFixedWidth(80);
    connect(m_resetBtn, &ElaPushButton::clicked, this,
            &PluginConfigDialog::onResetClicked);
    layout->addWidget(m_resetBtn);

    m_cancelBtn = new ElaPushButton(tr("Cancel"), m_buttonWidget);
    m_cancelBtn->setFixedWidth(80);
    connect(m_cancelBtn, &ElaPushButton::clicked, this, &QDialog::reject);
    layout->addWidget(m_cancelBtn);

    m_saveBtn = new ElaPushButton(tr("Save"), m_buttonWidget);
    m_saveBtn->setFixedWidth(80);
    connect(m_saveBtn, &ElaPushButton::clicked, this,
            &PluginConfigDialog::onSaveClicked);
    layout->addWidget(m_saveBtn);
}

void PluginConfigDialog::retranslateUI() {
    setWindowTitle(tr("Plugin Configuration - %1").arg(m_pluginName));

    if (m_showAdvancedCheck) {
        m_showAdvancedCheck->setText(tr("Show advanced settings"));
    }

    if (m_importBtn)
        m_importBtn->setText(tr("Import"));
    if (m_exportBtn)
        m_exportBtn->setText(tr("Export"));
    if (m_resetBtn)
        m_resetBtn->setText(tr("Reset"));
    if (m_cancelBtn)
        m_cancelBtn->setText(tr("Cancel"));
    if (m_saveBtn)
        m_saveBtn->setText(tr("Save"));

    loadPluginInfo();
}

void PluginConfigDialog::loadPluginInfo() {
    if (!m_pluginManager) {
        return;
    }

    PluginMetadata metadata = m_pluginManager->getPluginMetadata(m_pluginName);

    if (m_pluginNameLabel) {
        m_pluginNameLabel->setText(metadata.name);
    }

    if (m_pluginVersionLabel) {
        m_pluginVersionLabel->setText(
            tr("Version %1 by %2").arg(metadata.version, metadata.author));
    }

    if (m_pluginDescriptionLabel) {
        m_pluginDescriptionLabel->setText(metadata.description);
    }
}

void PluginConfigDialog::updateValidationDisplay() {
    if (!m_configWidget || !m_validationLabel) {
        return;
    }

    QStringList errors = m_configWidget->getValidationErrors();

    if (errors.isEmpty()) {
        m_validationLabel->setVisible(false);
    } else {
        m_validationLabel->setText(
            QString("<span style='color: red;'>%1</span>").arg(errors.first()));
        m_validationLabel->setVisible(true);
    }
}

bool PluginConfigDialog::confirmUnsavedChanges() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Unsaved Changes"),
        tr("You have unsaved changes. Do you want to discard them?"),
        QMessageBox::Yes | QMessageBox::No);

    return reply == QMessageBox::Yes;
}
