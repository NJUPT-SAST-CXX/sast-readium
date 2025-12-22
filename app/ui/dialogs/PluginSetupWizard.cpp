#include "PluginSetupWizard.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

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

PluginSetupWizard::PluginSetupWizard(const QString& pluginName, QWidget* parent)
    : QDialog(parent),
      m_pluginName(pluginName),
      m_pluginManager(nullptr),
      m_configModel(nullptr),
      m_mainLayout(nullptr),
      m_stackedWidget(nullptr),
      m_welcomePage(nullptr),
      m_welcomeTitle(nullptr),
      m_welcomeDescription(nullptr),
      m_pluginInfoText(nullptr),
      m_requiredPage(nullptr),
      m_requiredTitle(nullptr),
      m_requiredDescription(nullptr),
      m_requiredConfigWidget(nullptr),
      m_optionalPage(nullptr),
      m_optionalTitle(nullptr),
      m_optionalDescription(nullptr),
      m_optionalConfigWidget(nullptr),
      m_completionPage(nullptr),
      m_completionTitle(nullptr),
      m_completionDescription(nullptr),
      m_summaryText(nullptr),
      m_navWidget(nullptr),
      m_backBtn(nullptr),
      m_nextBtn(nullptr),
      m_skipBtn(nullptr),
      m_finishBtn(nullptr),
      m_cancelBtn(nullptr),
      m_progressLabel(nullptr),
      m_currentPage(WelcomePage),
      m_completed(false),
      m_hasRequiredConfig(false),
      m_hasOptionalConfig(false) {
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
            m_configModel->setConfigSchema(metadata);
        }

        // Check what config we have
        m_hasRequiredConfig = !m_configModel->getRequiredEntries().isEmpty();
        m_hasOptionalConfig = m_configModel->rowCount() >
                              m_configModel->getRequiredEntries().size();
    }

    setupUI();
    loadPluginInfo();
    retranslateUI();
    goToPage(WelcomePage);

    // Set dialog properties
    setWindowTitle(tr("Plugin Setup - %1").arg(pluginName));
    setMinimumSize(550, 450);
    resize(600, 500);
}

PluginSetupWizard::~PluginSetupWizard() = default;

// ============================================================================
// Public Static Methods
// ============================================================================

bool PluginSetupWizard::showSetupWizard(const QString& pluginName,
                                        QWidget* parent) {
    PluginSetupWizard wizard(pluginName, parent);
    wizard.exec();
    return wizard.wasCompleted();
}

bool PluginSetupWizard::needsSetupWizard(const QString& pluginName) {
    auto* pluginManager =
        ServiceLocator::instance().getService<PluginManager>();
    if (!pluginManager) {
        return false;
    }

    // Create temporary model to check
    PluginConfigModel model(pluginManager, pluginName);

    QJsonObject metadata =
        pluginManager->getPluginMetadata(pluginName).configuration;
    if (metadata.contains("configSchema")) {
        model.setConfigSchema(metadata["configSchema"].toObject());
    } else if (!metadata.isEmpty()) {
        model.setConfigSchema(metadata);
    }

    return model.hasRequiredUnset();
}

// ============================================================================
// Protected Methods
// ============================================================================

void PluginSetupWizard::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUI();
    }
    QDialog::changeEvent(event);
}

// ============================================================================
// Private Slots
// ============================================================================

void PluginSetupWizard::onNextClicked() {
    if (!validateCurrentPage()) {
        return;
    }

    WizardPage nextPage = m_currentPage;

    switch (m_currentPage) {
        case WelcomePage:
            if (m_hasRequiredConfig) {
                nextPage = RequiredConfigPage;
            } else if (m_hasOptionalConfig) {
                nextPage = OptionalConfigPage;
            } else {
                nextPage = CompletionPage;
            }
            break;

        case RequiredConfigPage:
            if (m_hasOptionalConfig) {
                nextPage = OptionalConfigPage;
            } else {
                nextPage = CompletionPage;
            }
            break;

        case OptionalConfigPage:
            nextPage = CompletionPage;
            break;

        case CompletionPage:
            // Should not happen
            break;

        default:
            break;
    }

    goToPage(nextPage);
}

void PluginSetupWizard::onBackClicked() {
    WizardPage prevPage = m_currentPage;

    switch (m_currentPage) {
        case WelcomePage:
            // Already at start
            break;

        case RequiredConfigPage:
            prevPage = WelcomePage;
            break;

        case OptionalConfigPage:
            if (m_hasRequiredConfig) {
                prevPage = RequiredConfigPage;
            } else {
                prevPage = WelcomePage;
            }
            break;

        case CompletionPage:
            if (m_hasOptionalConfig) {
                prevPage = OptionalConfigPage;
            } else if (m_hasRequiredConfig) {
                prevPage = RequiredConfigPage;
            } else {
                prevPage = WelcomePage;
            }
            break;

        default:
            break;
    }

    goToPage(prevPage);
}

void PluginSetupWizard::onSkipClicked() {
    // Skip to completion without optional config
    goToPage(CompletionPage);
}

void PluginSetupWizard::onFinishClicked() {
    saveConfiguration();
    m_completed = true;
    accept();
}

void PluginSetupWizard::onValidationStateChanged(bool isValid) {
    if (m_currentPage == RequiredConfigPage) {
        m_nextBtn->setEnabled(isValid);
    }
}

// ============================================================================
// Private Methods
// ============================================================================

void PluginSetupWizard::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(24, 24, 24, 24);
    m_mainLayout->setSpacing(16);

    // Progress label
    m_progressLabel = new ElaText("", this);
    m_progressLabel->setTextPixelSize(11);
    m_mainLayout->addWidget(m_progressLabel);

    // Stacked widget for pages
    m_stackedWidget = new QStackedWidget(this);
    m_mainLayout->addWidget(m_stackedWidget, 1);

    createWelcomePage();
    createRequiredConfigPage();
    createOptionalConfigPage();
    createCompletionPage();

    setupNavigation();
    m_mainLayout->addWidget(m_navWidget);
}

void PluginSetupWizard::createWelcomePage() {
    m_welcomePage = new QWidget(m_stackedWidget);
    auto* layout = new QVBoxLayout(m_welcomePage);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    m_welcomeTitle = new ElaText(tr("Welcome"), m_welcomePage);
    m_welcomeTitle->setTextPixelSize(20);
    layout->addWidget(m_welcomeTitle);

    m_welcomeDescription = new ElaText("", m_welcomePage);
    m_welcomeDescription->setTextPixelSize(13);
    m_welcomeDescription->setWordWrap(true);
    layout->addWidget(m_welcomeDescription);

    m_pluginInfoText = new ElaText("", m_welcomePage);
    m_pluginInfoText->setTextPixelSize(11);
    m_pluginInfoText->setWordWrap(true);
    layout->addWidget(m_pluginInfoText);

    layout->addStretch();

    m_stackedWidget->addWidget(m_welcomePage);
}

void PluginSetupWizard::createRequiredConfigPage() {
    m_requiredPage = new QWidget(m_stackedWidget);
    auto* layout = new QVBoxLayout(m_requiredPage);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    m_requiredTitle = new ElaText(tr("Required Settings"), m_requiredPage);
    m_requiredTitle->setTextPixelSize(18);
    layout->addWidget(m_requiredTitle);

    m_requiredDescription = new ElaText("", m_requiredPage);
    m_requiredDescription->setTextPixelSize(12);
    m_requiredDescription->setWordWrap(true);
    layout->addWidget(m_requiredDescription);

    m_requiredConfigWidget = new PluginConfigWidget(m_requiredPage);
    m_requiredConfigWidget->setModel(m_configModel.get());
    m_requiredConfigWidget->setRequiredOnly(true);

    connect(m_requiredConfigWidget, &PluginConfigWidget::validationStateChanged,
            this, &PluginSetupWizard::onValidationStateChanged);

    layout->addWidget(m_requiredConfigWidget, 1);

    m_stackedWidget->addWidget(m_requiredPage);
}

void PluginSetupWizard::createOptionalConfigPage() {
    m_optionalPage = new QWidget(m_stackedWidget);
    auto* layout = new QVBoxLayout(m_optionalPage);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    m_optionalTitle = new ElaText(tr("Optional Settings"), m_optionalPage);
    m_optionalTitle->setTextPixelSize(18);
    layout->addWidget(m_optionalTitle);

    m_optionalDescription = new ElaText("", m_optionalPage);
    m_optionalDescription->setTextPixelSize(12);
    m_optionalDescription->setWordWrap(true);
    layout->addWidget(m_optionalDescription);

    m_optionalConfigWidget = new PluginConfigWidget(m_optionalPage);
    m_optionalConfigWidget->setModel(m_configModel.get());
    m_optionalConfigWidget->setRequiredOnly(false);
    // Don't show required items again since we already configured them
    // Actually, show all but the user can see which are already set

    layout->addWidget(m_optionalConfigWidget, 1);

    m_stackedWidget->addWidget(m_optionalPage);
}

void PluginSetupWizard::createCompletionPage() {
    m_completionPage = new QWidget(m_stackedWidget);
    auto* layout = new QVBoxLayout(m_completionPage);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    m_completionTitle = new ElaText(tr("Setup Complete"), m_completionPage);
    m_completionTitle->setTextPixelSize(20);
    layout->addWidget(m_completionTitle);

    m_completionDescription = new ElaText("", m_completionPage);
    m_completionDescription->setTextPixelSize(13);
    m_completionDescription->setWordWrap(true);
    layout->addWidget(m_completionDescription);

    m_summaryText = new ElaText("", m_completionPage);
    m_summaryText->setTextPixelSize(11);
    m_summaryText->setWordWrap(true);
    layout->addWidget(m_summaryText);

    layout->addStretch();

    m_stackedWidget->addWidget(m_completionPage);
}

void PluginSetupWizard::setupNavigation() {
    m_navWidget = new QWidget(this);
    auto* layout = new QHBoxLayout(m_navWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_cancelBtn = new ElaPushButton(tr("Cancel"), m_navWidget);
    m_cancelBtn->setFixedWidth(80);
    connect(m_cancelBtn, &ElaPushButton::clicked, this, &QDialog::reject);
    layout->addWidget(m_cancelBtn);

    layout->addStretch();

    m_backBtn = new ElaPushButton(tr("Back"), m_navWidget);
    m_backBtn->setFixedWidth(80);
    connect(m_backBtn, &ElaPushButton::clicked, this,
            &PluginSetupWizard::onBackClicked);
    layout->addWidget(m_backBtn);

    m_skipBtn = new ElaPushButton(tr("Skip"), m_navWidget);
    m_skipBtn->setFixedWidth(80);
    connect(m_skipBtn, &ElaPushButton::clicked, this,
            &PluginSetupWizard::onSkipClicked);
    layout->addWidget(m_skipBtn);

    m_nextBtn = new ElaPushButton(tr("Next"), m_navWidget);
    m_nextBtn->setFixedWidth(80);
    connect(m_nextBtn, &ElaPushButton::clicked, this,
            &PluginSetupWizard::onNextClicked);
    layout->addWidget(m_nextBtn);

    m_finishBtn = new ElaPushButton(tr("Finish"), m_navWidget);
    m_finishBtn->setFixedWidth(80);
    connect(m_finishBtn, &ElaPushButton::clicked, this,
            &PluginSetupWizard::onFinishClicked);
    layout->addWidget(m_finishBtn);
}

void PluginSetupWizard::retranslateUI() {
    setWindowTitle(tr("Plugin Setup - %1").arg(m_pluginName));

    if (m_welcomeTitle) {
        m_welcomeTitle->setText(tr("Welcome"));
    }
    if (m_requiredTitle) {
        m_requiredTitle->setText(tr("Required Settings"));
    }
    if (m_optionalTitle) {
        m_optionalTitle->setText(tr("Optional Settings"));
    }
    if (m_completionTitle) {
        m_completionTitle->setText(tr("Setup Complete"));
    }

    if (m_cancelBtn)
        m_cancelBtn->setText(tr("Cancel"));
    if (m_backBtn)
        m_backBtn->setText(tr("Back"));
    if (m_skipBtn)
        m_skipBtn->setText(tr("Skip"));
    if (m_nextBtn)
        m_nextBtn->setText(tr("Next"));
    if (m_finishBtn)
        m_finishBtn->setText(tr("Finish"));

    loadPluginInfo();
    updateNavigation();
}

void PluginSetupWizard::loadPluginInfo() {
    if (!m_pluginManager) {
        return;
    }

    PluginMetadata metadata = m_pluginManager->getPluginMetadata(m_pluginName);

    // Welcome page
    if (m_welcomeDescription) {
        m_welcomeDescription->setText(
            tr("This wizard will help you configure the %1 plugin.\n\n"
               "Click Next to continue.")
                .arg(metadata.name));
    }

    if (m_pluginInfoText) {
        QString info = tr("<b>%1</b> v%2<br>by %3<br><br>%4")
                           .arg(metadata.name, metadata.version,
                                metadata.author, metadata.description);
        m_pluginInfoText->setText(info);
    }

    // Required page
    if (m_requiredDescription) {
        m_requiredDescription->setText(tr(
            "Please configure the following required settings. "
            "These settings are necessary for the plugin to work correctly."));
    }

    // Optional page
    if (m_optionalDescription) {
        m_optionalDescription->setText(
            tr("You can customize these optional settings or use the default "
               "values. "
               "Click Skip to use defaults and finish setup."));
    }

    // Completion page
    if (m_completionDescription) {
        m_completionDescription->setText(tr(
            "The plugin has been configured successfully!\n\n"
            "Click Finish to save your settings and start using the plugin."));
    }

    // Summary
    if (m_summaryText && m_configModel) {
        QStringList summary;
        summary << tr("<b>Configuration Summary:</b><br>");

        for (int i = 0; i < m_configModel->rowCount(); ++i) {
            QModelIndex keyIdx =
                m_configModel->index(i, PluginConfigModel::KeyColumn);
            QModelIndex valIdx =
                m_configModel->index(i, PluginConfigModel::ValueColumn);
            QString key = m_configModel->data(keyIdx).toString();
            QString value = m_configModel->data(valIdx).toString();
            summary << QString("• %1: %2").arg(key, value);
        }

        m_summaryText->setText(summary.join("<br>"));
    }
}

void PluginSetupWizard::goToPage(WizardPage page) {
    m_currentPage = page;
    m_stackedWidget->setCurrentIndex(static_cast<int>(page));
    updateNavigation();

    // Refresh config widgets when entering their pages
    if (page == RequiredConfigPage && m_requiredConfigWidget) {
        m_requiredConfigWidget->rebuildUI();
    } else if (page == OptionalConfigPage && m_optionalConfigWidget) {
        m_optionalConfigWidget->rebuildUI();
    } else if (page == CompletionPage) {
        loadPluginInfo();  // Refresh summary
    }
}

void PluginSetupWizard::updateNavigation() {
    // Calculate total steps (excluding pages that don't exist)
    int totalSteps = 2;  // Welcome + Completion
    if (m_hasRequiredConfig)
        totalSteps++;
    if (m_hasOptionalConfig)
        totalSteps++;

    int currentStep = 1;
    QString stepName;

    switch (m_currentPage) {
        case WelcomePage:
            currentStep = 1;
            stepName = tr("Welcome");
            m_backBtn->setVisible(false);
            m_nextBtn->setVisible(true);
            m_skipBtn->setVisible(false);
            m_finishBtn->setVisible(false);
            break;

        case RequiredConfigPage:
            currentStep = 2;
            stepName = tr("Required Settings");
            m_backBtn->setVisible(true);
            m_nextBtn->setVisible(true);
            m_skipBtn->setVisible(false);
            m_finishBtn->setVisible(false);
            // Enable/disable next based on validation
            m_nextBtn->setEnabled(m_requiredConfigWidget &&
                                  m_requiredConfigWidget->isValid());
            break;

        case OptionalConfigPage:
            currentStep = m_hasRequiredConfig ? 3 : 2;
            stepName = tr("Optional Settings");
            m_backBtn->setVisible(true);
            m_nextBtn->setVisible(true);
            m_skipBtn->setVisible(true);
            m_finishBtn->setVisible(false);
            m_nextBtn->setEnabled(true);
            break;

        case CompletionPage:
            currentStep = totalSteps;
            stepName = tr("Complete");
            m_backBtn->setVisible(true);
            m_nextBtn->setVisible(false);
            m_skipBtn->setVisible(false);
            m_finishBtn->setVisible(true);
            break;

        default:
            break;
    }

    // Update progress label
    if (m_progressLabel) {
        m_progressLabel->setText(tr("Step %1 of %2: %3")
                                     .arg(currentStep)
                                     .arg(totalSteps)
                                     .arg(stepName));
    }
}

bool PluginSetupWizard::validateCurrentPage() {
    if (m_currentPage == RequiredConfigPage && m_requiredConfigWidget) {
        QStringList errors = m_requiredConfigWidget->getValidationErrors();
        if (!errors.isEmpty()) {
            ElaMessageBar::error(ElaMessageBarType::TopRight,
                                 tr("Validation Error"), errors.first(), 3000,
                                 this);
            return false;
        }
    }
    return true;
}

void PluginSetupWizard::saveConfiguration() {
    if (m_configModel) {
        m_configModel->saveConfiguration();
    }

    // Mark plugin as configured
    if (m_pluginManager) {
        QSettings settings("SAST", "Readium-Plugins");
        settings.setValue(m_pluginName + "/configured", true);
    }
}
