#include "PluginManagerPage.h"

// Business Logic
#include "controller/ServiceLocator.h"
#include "plugin/PluginManager.h"

// Plugin Configuration Dialogs
#include "ui/dialogs/PluginConfigDialog.h"
#include "ui/dialogs/PluginSetupWizard.h"

// ElaWidgetTools
#include "ElaComboBox.h"
#include "ElaContentDialog.h"
#include "ElaInteractiveCard.h"
#include "ElaLineEdit.h"
#include "ElaMessageBar.h"
#include "ElaProgressRing.h"
#include "ElaPushButton.h"
#include "ElaTableView.h"
#include "ElaText.h"
#include "ElaToggleSwitch.h"
#include "ElaToolTip.h"

// Qt
#include <QEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSplitter>

#include <QStandardItemModel>
#include <QTextEdit>
#include <QVBoxLayout>
#include "ElaContentDialog.h"

// Logging
#include "logging/SimpleLogging.h"

// ============================================================================
// Construction and Destruction
// ============================================================================

PluginManagerPage::PluginManagerPage(QWidget* parent)
    : ElaScrollPage(parent),
      m_pluginManager(nullptr),
      m_enableToggle(nullptr),
      m_installProgressRing(nullptr),
      m_cardViewWidget(nullptr),
      m_selectedPluginName(),
      m_filterText(),
      m_filterCategory(0),
      m_useCardView(false),
      m_isInstalling(false) {
    SLOG_INFO("PluginManagerPage: Constructor started");

    // Set window title for navigation
    setWindowTitle(tr("Plugin Manager"));

    // Set title visibility
    setTitleVisible(false);

    // Set margins
    setContentsMargins(2, 2, 0, 0);

    // Get PluginManager from ServiceLocator
    m_pluginManager = ServiceLocator::instance().getService<PluginManager>();
    if (!m_pluginManager) {
        SLOG_ERROR(
            "PluginManagerPage: Failed to get PluginManager from "
            "ServiceLocator");
    }

    setupUi();
    connectSignals();
    refreshPluginList();

    SLOG_INFO("PluginManagerPage: Constructor completed");
}

PluginManagerPage::~PluginManagerPage() {
    SLOG_INFO("PluginManagerPage: Destructor called");
}

// ============================================================================
// UI Initialization
// ============================================================================

void PluginManagerPage::setupUi() {
    // Create central widget
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(10);

    // Setup toolbar
    setupToolbar();
    mainLayout->addWidget(m_toolbarWidget);

    // Create splitter for list and details
    m_mainSplitter = new QSplitter(Qt::Horizontal, centralWidget);

    // Setup plugin list view
    setupPluginListView();
    m_mainSplitter->addWidget(m_pluginTableView);

    // Setup plugin details panel
    setupPluginDetails();
    m_mainSplitter->addWidget(m_detailsWidget);

    // Set splitter sizes (70% list, 30% details)
    m_mainSplitter->setStretchFactor(0, 7);
    m_mainSplitter->setStretchFactor(1, 3);

    mainLayout->addWidget(m_mainSplitter);

    // Add central widget
    addCentralWidget(centralWidget, true, true, 0.5);

    retranslateUi();
}

void PluginManagerPage::setupToolbar() {
    m_toolbarWidget = new QWidget(this);
    m_toolbarLayout = new QHBoxLayout(m_toolbarWidget);
    m_toolbarLayout->setContentsMargins(5, 5, 5, 5);
    m_toolbarLayout->setSpacing(10);

    // Search box
    m_searchEdit = new ElaLineEdit(m_toolbarWidget);
    m_searchEdit->setPlaceholderText(tr("Search plugins..."));
    m_searchEdit->setFixedWidth(250);
    m_toolbarLayout->addWidget(m_searchEdit);

    // Filter combo
    m_filterCombo = new ElaComboBox(m_toolbarWidget);
    m_filterCombo->addItem(tr("All Plugins"));
    m_filterCombo->addItem(tr("Enabled"));
    m_filterCombo->addItem(tr("Disabled"));
    m_filterCombo->addItem(tr("Loaded"));
    m_filterCombo->addItem(tr("Error"));
    m_filterCombo->setFixedWidth(150);
    m_toolbarLayout->addWidget(m_filterCombo);

    m_toolbarLayout->addStretch();

    // Refresh button
    m_refreshBtn = new ElaPushButton(tr("Refresh"), m_toolbarWidget);
    m_toolbarLayout->addWidget(m_refreshBtn);

    // Install button
    m_installBtn = new ElaPushButton(tr("Install Plugin"), m_toolbarWidget);
    m_toolbarLayout->addWidget(m_installBtn);
}

void PluginManagerPage::setupPluginListView() {
    m_pluginTableView = new ElaTableView(this);

    // Create model
    m_pluginListModel = new QStandardItemModel(0, ColumnCount, this);
    m_pluginListModel->setHorizontalHeaderLabels({tr("Name"), tr("Version"),
                                                  tr("Status"), tr("Author"),
                                                  tr("Description")});

    m_pluginTableView->setModel(m_pluginListModel);

    // Configure table view
    m_pluginTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pluginTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pluginTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pluginTableView->setAlternatingRowColors(true);
    m_pluginTableView->setSortingEnabled(true);

    // Configure headers
    QHeaderView* header = m_pluginTableView->horizontalHeader();
    header->setStretchLastSection(true);
    header->setSectionResizeMode(ColumnName, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ColumnVersion, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ColumnStatus, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ColumnAuthor, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ColumnDescription, QHeaderView::Stretch);

    m_pluginTableView->verticalHeader()->hide();
}

void PluginManagerPage::setupPluginDetails() {
    m_detailsWidget = new QWidget(this);
    m_detailsLayout = new QVBoxLayout(m_detailsWidget);
    m_detailsLayout->setContentsMargins(10, 10, 10, 10);
    m_detailsLayout->setSpacing(10);

    // Plugin name
    m_pluginNameLabel = new ElaText(tr("No plugin selected"), m_detailsWidget);
    m_pluginNameLabel->setTextPixelSize(18);
    m_detailsLayout->addWidget(m_pluginNameLabel);

    // Plugin version
    m_pluginVersionLabel = new ElaText("", m_detailsWidget);
    m_pluginVersionLabel->setTextPixelSize(12);
    m_detailsLayout->addWidget(m_pluginVersionLabel);

    // Plugin author
    m_pluginAuthorLabel = new ElaText("", m_detailsWidget);
    m_pluginAuthorLabel->setTextPixelSize(12);
    m_detailsLayout->addWidget(m_pluginAuthorLabel);

    // Plugin status
    m_pluginStatusLabel = new ElaText("", m_detailsWidget);
    m_pluginStatusLabel->setTextPixelSize(12);
    m_detailsLayout->addWidget(m_pluginStatusLabel);

    // Description
    QLabel* descLabel = new QLabel(tr("Description:"), m_detailsWidget);
    m_detailsLayout->addWidget(descLabel);

    m_pluginDescriptionEdit = new QTextEdit(m_detailsWidget);
    m_pluginDescriptionEdit->setReadOnly(true);
    m_pluginDescriptionEdit->setMaximumHeight(100);
    m_detailsLayout->addWidget(m_pluginDescriptionEdit);

    // Dependencies
    QLabel* depsLabel = new QLabel(tr("Dependencies:"), m_detailsWidget);
    m_detailsLayout->addWidget(depsLabel);

    m_pluginDependenciesEdit = new QTextEdit(m_detailsWidget);
    m_pluginDependenciesEdit->setReadOnly(true);
    m_pluginDependenciesEdit->setMaximumHeight(60);
    m_detailsLayout->addWidget(m_pluginDependenciesEdit);

    // Features
    QLabel* featuresLabel = new QLabel(tr("Features:"), m_detailsWidget);
    m_detailsLayout->addWidget(featuresLabel);

    m_pluginFeaturesEdit = new QTextEdit(m_detailsWidget);
    m_pluginFeaturesEdit->setReadOnly(true);
    m_pluginFeaturesEdit->setMaximumHeight(60);
    m_detailsLayout->addWidget(m_pluginFeaturesEdit);

    m_detailsLayout->addStretch();

    // Enable toggle switch with label
    auto* toggleLayout = new QHBoxLayout();
    auto* enableLabel = new ElaText(tr("Enabled:"), m_detailsWidget);
    enableLabel->setTextPixelSize(13);
    toggleLayout->addWidget(enableLabel);

    m_enableToggle = new ElaToggleSwitch(m_detailsWidget);
    m_enableToggle->setEnabled(false);
    toggleLayout->addWidget(m_enableToggle);
    toggleLayout->addStretch();
    m_detailsLayout->addLayout(toggleLayout);

    // Installation progress ring (initially hidden)
    m_installProgressRing = new ElaProgressRing(m_detailsWidget);
    m_installProgressRing->setFixedSize(40, 40);
    m_installProgressRing->setVisible(false);
    m_detailsLayout->addWidget(m_installProgressRing, 0, Qt::AlignCenter);

    // Action buttons
    m_actionButtonsLayout = new QHBoxLayout();
    m_actionButtonsLayout->setSpacing(10);

    m_enableDisableBtn = new ElaPushButton(tr("Enable"), m_detailsWidget);
    m_actionButtonsLayout->addWidget(m_enableDisableBtn);

    m_configureBtn = new ElaPushButton(tr("Configure"), m_detailsWidget);
    m_actionButtonsLayout->addWidget(m_configureBtn);

    m_uninstallBtn = new ElaPushButton(tr("Uninstall"), m_detailsWidget);
    m_actionButtonsLayout->addWidget(m_uninstallBtn);

    m_detailsLayout->addLayout(m_actionButtonsLayout);

    // Initially disable action buttons
    m_enableDisableBtn->setEnabled(false);
    m_configureBtn->setEnabled(false);
    m_uninstallBtn->setEnabled(false);
}

void PluginManagerPage::connectSignals() {
    // Toolbar signals
    connect(m_searchEdit, &ElaLineEdit::textChanged, this,
            &PluginManagerPage::onFilterTextChanged);
    connect(m_filterCombo,
            QOverload<int>::of(&ElaComboBox::currentIndexChanged), this,
            &PluginManagerPage::onFilterCategoryChanged);
    connect(m_refreshBtn, &ElaPushButton::clicked, this,
            &PluginManagerPage::onRefreshClicked);
    connect(m_installBtn, &ElaPushButton::clicked, this,
            &PluginManagerPage::onInstallClicked);

    // Table view signals
    connect(m_pluginTableView->selectionModel(),
            &QItemSelectionModel::selectionChanged, this,
            &PluginManagerPage::onPluginSelectionChanged);

    // Action button signals
    connect(m_enableDisableBtn, &ElaPushButton::clicked, this,
            &PluginManagerPage::onEnableDisableClicked);
    connect(m_configureBtn, &ElaPushButton::clicked, this,
            &PluginManagerPage::onConfigureClicked);
    connect(m_uninstallBtn, &ElaPushButton::clicked, this,
            &PluginManagerPage::onUninstallClicked);

    // PluginManager signals
    if (m_pluginManager) {
        connect(m_pluginManager, &PluginManager::pluginLoaded, this,
                &PluginManagerPage::onPluginLoaded);
        connect(m_pluginManager, &PluginManager::pluginUnloaded, this,
                &PluginManagerPage::onPluginUnloaded);
        connect(m_pluginManager, &PluginManager::pluginEnabled, this,
                &PluginManagerPage::onPluginEnabled);
        connect(m_pluginManager, &PluginManager::pluginDisabled, this,
                &PluginManagerPage::onPluginDisabled);
        connect(m_pluginManager, &PluginManager::pluginError, this,
                &PluginManagerPage::onPluginError);
    }
}

void PluginManagerPage::retranslateUi() {
    setWindowTitle(tr("Plugin Manager"));
    m_searchEdit->setPlaceholderText(tr("Search plugins..."));

    // Update filter combo items
    m_filterCombo->setItemText(0, tr("All Plugins"));
    m_filterCombo->setItemText(1, tr("Enabled"));
    m_filterCombo->setItemText(2, tr("Disabled"));
    m_filterCombo->setItemText(3, tr("Loaded"));
    m_filterCombo->setItemText(4, tr("Error"));

    // Update buttons
    m_refreshBtn->setText(tr("Refresh"));
    m_installBtn->setText(tr("Install Plugin"));

    // Update table headers
    m_pluginListModel->setHorizontalHeaderLabels({tr("Name"), tr("Version"),
                                                  tr("Status"), tr("Author"),
                                                  tr("Description")});
}

// ============================================================================
// Plugin Management
// ============================================================================

void PluginManagerPage::refreshPluginList() {
    if (!m_pluginManager) {
        SLOG_ERROR("PluginManagerPage: PluginManager not available");
        return;
    }

    SLOG_INFO("PluginManagerPage: Refreshing plugin list");

    // Rescan for plugins
    m_pluginManager->scanForPlugins();

    // Populate the list
    populatePluginList();
}

void PluginManagerPage::populatePluginList() {
    if (!m_pluginManager) {
        return;
    }

    // Clear current list
    m_pluginListModel->removeRows(0, m_pluginListModel->rowCount());

    // Get all plugins
    QHash<QString, PluginMetadata> allPlugins =
        m_pluginManager->getAllPluginMetadata();

    for (auto it = allPlugins.begin(); it != allPlugins.end(); ++it) {
        const QString& pluginName = it.key();
        const PluginMetadata& metadata = it.value();

        // Create row items
        QList<QStandardItem*> rowItems;

        // Name
        QStandardItem* nameItem = new QStandardItem(metadata.name);
        nameItem->setData(pluginName, Qt::UserRole);  // Store plugin name
        rowItems.append(nameItem);

        // Version
        rowItems.append(new QStandardItem(metadata.version));

        // Status
        QString statusText = getPluginStatusText(pluginName);
        QStandardItem* statusItem = new QStandardItem(statusText);
        statusItem->setForeground(QBrush(getPluginStatusColor(pluginName)));
        rowItems.append(statusItem);

        // Author
        rowItems.append(new QStandardItem(metadata.author));

        // Description
        rowItems.append(new QStandardItem(metadata.description));

        m_pluginListModel->appendRow(rowItems);
    }

    // Apply current filter
    applyFilter();

    SLOG_INFO(
        QString("PluginManagerPage: Plugin list populated with %1 plugins")
            .arg(allPlugins.size()));
}

void PluginManagerPage::updatePluginDetails(const QString& pluginName) {
    if (!m_pluginManager) {
        return;
    }

    PluginMetadata metadata = m_pluginManager->getPluginMetadata(pluginName);

    if (metadata.name.isEmpty()) {
        clearPluginDetails();
        return;
    }

    // Update labels
    m_pluginNameLabel->setText(metadata.name);
    m_pluginVersionLabel->setText(tr("Version: %1").arg(metadata.version));
    m_pluginAuthorLabel->setText(tr("Author: %1").arg(metadata.author));

    QString statusText = getPluginStatusText(pluginName);
    m_pluginStatusLabel->setText(tr("Status: %1").arg(statusText));

    // Update description
    m_pluginDescriptionEdit->setText(metadata.description);

    // Update dependencies
    if (metadata.dependencies.isEmpty()) {
        m_pluginDependenciesEdit->setText(tr("None"));
    } else {
        m_pluginDependenciesEdit->setText(metadata.dependencies.join("\n"));
    }

    // Update features
    if (metadata.features.isEmpty()) {
        m_pluginFeaturesEdit->setText(tr("None"));
    } else {
        m_pluginFeaturesEdit->setText(metadata.features.join("\n"));
    }

    // Update action buttons
    m_enableDisableBtn->setEnabled(true);
    m_uninstallBtn->setEnabled(true);
    m_configureBtn->setEnabled(m_pluginManager->isPluginLoaded(pluginName));

    if (metadata.isEnabled) {
        m_enableDisableBtn->setText(tr("Disable"));
    } else {
        m_enableDisableBtn->setText(tr("Enable"));
    }
}

void PluginManagerPage::clearPluginDetails() {
    m_pluginNameLabel->setText(tr("No plugin selected"));
    m_pluginVersionLabel->clear();
    m_pluginAuthorLabel->clear();
    m_pluginStatusLabel->clear();
    m_pluginDescriptionEdit->clear();
    m_pluginDependenciesEdit->clear();
    m_pluginFeaturesEdit->clear();

    m_enableDisableBtn->setEnabled(false);
    m_configureBtn->setEnabled(false);
    m_uninstallBtn->setEnabled(false);
}

void PluginManagerPage::applyFilter() {
    if (!m_pluginManager) {
        return;
    }

    for (int row = 0; row < m_pluginListModel->rowCount(); ++row) {
        QStandardItem* nameItem = m_pluginListModel->item(row, ColumnName);
        if (!nameItem) {
            continue;
        }

        QString pluginName = nameItem->data(Qt::UserRole).toString();
        PluginMetadata metadata =
            m_pluginManager->getPluginMetadata(pluginName);

        bool visible = true;

        // Apply text filter
        if (!m_filterText.isEmpty()) {
            bool matchesText =
                metadata.name.contains(m_filterText, Qt::CaseInsensitive) ||
                metadata.description.contains(m_filterText,
                                              Qt::CaseInsensitive) ||
                metadata.author.contains(m_filterText, Qt::CaseInsensitive);
            visible = visible && matchesText;
        }

        // Apply category filter
        switch (m_filterCategory) {
            case 1:  // Enabled
                visible = visible && metadata.isEnabled;
                break;
            case 2:  // Disabled
                visible = visible && !metadata.isEnabled;
                break;
            case 3:  // Loaded
                visible = visible && metadata.isLoaded;
                break;
            case 4:  // Error
                visible =
                    visible &&
                    !m_pluginManager->getPluginErrors(pluginName).isEmpty();
                break;
            default:  // All
                break;
        }

        m_pluginTableView->setRowHidden(row, !visible);
    }
}

// ============================================================================
// Slots - UI Event Handlers
// ============================================================================

void PluginManagerPage::onPluginSelectionChanged() {
    QModelIndexList selection =
        m_pluginTableView->selectionModel()->selectedRows();

    if (selection.isEmpty()) {
        m_selectedPluginName.clear();
        clearPluginDetails();
        return;
    }

    QModelIndex index = selection.first();
    QStandardItem* nameItem = m_pluginListModel->item(index.row(), ColumnName);

    if (nameItem) {
        m_selectedPluginName = nameItem->data(Qt::UserRole).toString();
        updatePluginDetails(m_selectedPluginName);
    }
}

void PluginManagerPage::onEnableDisableClicked() {
    if (m_selectedPluginName.isEmpty() || !m_pluginManager) {
        return;
    }

    bool currentlyEnabled =
        m_pluginManager->isPluginEnabled(m_selectedPluginName);

    if (!currentlyEnabled) {
        // Enabling plugin - check if setup wizard is needed
        if (m_pluginManager->needsSetupWizard(m_selectedPluginName)) {
            // Show setup wizard
            PluginSetupWizard wizard(m_selectedPluginName, this);
            if (wizard.exec() != QDialog::Accepted || !wizard.wasCompleted()) {
                // User cancelled or didn't complete wizard
                ElaMessageBar::warning(ElaMessageBarType::TopRight,
                                       tr("Setup Required"),
                                       tr("Plugin setup was not completed. "
                                          "The plugin will remain disabled."),
                                       3000, this);
                return;
            }

            // Mark as configured after successful wizard completion
            m_pluginManager->markPluginConfigured(m_selectedPluginName, true);
        }
    }

    m_pluginManager->setPluginEnabled(m_selectedPluginName, !currentlyEnabled);

    // Update UI
    refreshPluginList();
    updatePluginDetails(m_selectedPluginName);

    QString message =
        currentlyEnabled ? tr("Plugin disabled") : tr("Plugin enabled");
    SLOG_INFO(QString("PluginManagerPage: %1: %2")
                  .arg(message, m_selectedPluginName));

    ElaMessageBar::success(ElaMessageBarType::TopRight, tr("Success"), message,
                           2000, this);
}

void PluginManagerPage::onInstallClicked() {
    QString filePath = QFileDialog::getOpenFileName(
        this, tr("Select Plugin File"), QString(),
        tr("Plugin Files (*.dll *.so *.dylib);;All Files (*)"));

    if (filePath.isEmpty()) {
        return;
    }

    if (!m_pluginManager) {
        return;
    }

    if (m_pluginManager->installPlugin(filePath)) {
        auto* dialog = new ElaContentDialog(this);
        dialog->setWindowTitle(tr("Success"));
        auto* w = new QWidget(dialog);
        auto* l = new QVBoxLayout(w);
        l->addWidget(new ElaText(tr("Plugin installed successfully"), w));
        dialog->setCentralWidget(w);
        dialog->setLeftButtonText(QString());
        dialog->setMiddleButtonText(QString());
        dialog->setRightButtonText(tr("OK"));
        connect(dialog, &ElaContentDialog::rightButtonClicked, dialog,
                &ElaContentDialog::close);
        dialog->exec();
        dialog->deleteLater();

        refreshPluginList();
        emit pluginInstalled(filePath);
    } else {
        auto* dialog = new ElaContentDialog(this);
        dialog->setWindowTitle(tr("Error"));
        auto* w = new QWidget(dialog);
        auto* l = new QVBoxLayout(w);
        l->addWidget(new ElaText(tr("Failed to install plugin"), w));
        dialog->setCentralWidget(w);
        dialog->setLeftButtonText(QString());
        dialog->setMiddleButtonText(QString());
        dialog->setRightButtonText(tr("OK"));
        connect(dialog, &ElaContentDialog::rightButtonClicked, dialog,
                &ElaContentDialog::close);
        dialog->exec();
        dialog->deleteLater();
    }
}

void PluginManagerPage::onUninstallClicked() {
    if (m_selectedPluginName.isEmpty() || !m_pluginManager) {
        return;
    }

    auto* dialog = new ElaContentDialog(this);
    dialog->setWindowTitle(tr("Confirm Uninstall"));
    auto* w = new QWidget(dialog);
    auto* l = new QVBoxLayout(w);
    l->addWidget(
        new ElaText(tr("Are you sure you want to uninstall plugin '%1'?")
                        .arg(m_selectedPluginName),
                    w));
    dialog->setCentralWidget(w);
    dialog->setLeftButtonText(tr("Cancel"));
    dialog->setRightButtonText(tr("Uninstall"));

    bool confirmed = false;
    connect(dialog, &ElaContentDialog::rightButtonClicked, this,
            [&confirmed, dialog]() {
                confirmed = true;
                dialog->close();
            });
    connect(dialog, &ElaContentDialog::leftButtonClicked, dialog,
            &ElaContentDialog::close);
    dialog->exec();
    dialog->deleteLater();

    if (!confirmed) {
        return;
    }

    if (m_pluginManager->uninstallPlugin(m_selectedPluginName)) {
        auto* successDialog = new ElaContentDialog(this);
        successDialog->setWindowTitle(tr("Success"));
        auto* sw = new QWidget(successDialog);
        auto* sl = new QVBoxLayout(sw);
        sl->addWidget(new ElaText(tr("Plugin uninstalled successfully"), sw));
        successDialog->setCentralWidget(sw);
        successDialog->setLeftButtonText(QString());
        successDialog->setMiddleButtonText(QString());
        successDialog->setRightButtonText(tr("OK"));
        connect(successDialog, &ElaContentDialog::rightButtonClicked,
                successDialog, &ElaContentDialog::close);
        successDialog->exec();
        successDialog->deleteLater();

        refreshPluginList();
        clearPluginDetails();
        emit pluginUninstalled(m_selectedPluginName);
    } else {
        auto* errorDialog = new ElaContentDialog(this);
        errorDialog->setWindowTitle(tr("Error"));
        auto* ew = new QWidget(errorDialog);
        auto* el = new QVBoxLayout(ew);
        el->addWidget(new ElaText(tr("Failed to uninstall plugin"), ew));
        errorDialog->setCentralWidget(ew);
        errorDialog->setLeftButtonText(QString());
        errorDialog->setMiddleButtonText(QString());
        errorDialog->setRightButtonText(tr("OK"));
        connect(errorDialog, &ElaContentDialog::rightButtonClicked, errorDialog,
                &ElaContentDialog::close);
        errorDialog->exec();
        errorDialog->deleteLater();
    }
}

void PluginManagerPage::onConfigureClicked() {
    if (m_selectedPluginName.isEmpty() || !m_pluginManager) {
        return;
    }

    // Check if plugin has configuration
    if (!m_pluginManager->hasConfigSchema(m_selectedPluginName)) {
        ElaMessageBar::information(
            ElaMessageBarType::TopRight, tr("No Configuration"),
            tr("This plugin has no configurable settings."), 2000, this);
        return;
    }

    // Show configuration dialog
    PluginConfigDialog dialog(m_selectedPluginName, this);
    if (dialog.exec() == QDialog::Accepted) {
        ElaMessageBar::success(
            ElaMessageBarType::TopRight, tr("Configuration Saved"),
            tr("Plugin configuration has been saved."), 2000, this);

        // Mark plugin as configured
        m_pluginManager->markPluginConfigured(m_selectedPluginName, true);
    }
}

void PluginManagerPage::onRefreshClicked() { refreshPluginList(); }

void PluginManagerPage::onFilterTextChanged(const QString& text) {
    m_filterText = text;
    applyFilter();
}

void PluginManagerPage::onFilterCategoryChanged(int index) {
    m_filterCategory = index;
    applyFilter();
}

// ============================================================================
// Slots - PluginManager Event Handlers
// ============================================================================

void PluginManagerPage::onPluginLoaded(const QString& pluginName) {
    SLOG_INFO(QString("PluginManagerPage: Plugin loaded: %1").arg(pluginName));
    refreshPluginList();

    if (m_selectedPluginName == pluginName) {
        updatePluginDetails(pluginName);
    }
}

void PluginManagerPage::onPluginUnloaded(const QString& pluginName) {
    SLOG_INFO(
        QString("PluginManagerPage: Plugin unloaded: %1").arg(pluginName));
    refreshPluginList();

    if (m_selectedPluginName == pluginName) {
        updatePluginDetails(pluginName);
    }
}

void PluginManagerPage::onPluginEnabled(const QString& pluginName) {
    SLOG_INFO(QString("PluginManagerPage: Plugin enabled: %1").arg(pluginName));
    refreshPluginList();

    if (m_selectedPluginName == pluginName) {
        updatePluginDetails(pluginName);
    }
}

void PluginManagerPage::onPluginDisabled(const QString& pluginName) {
    SLOG_INFO(
        QString("PluginManagerPage: Plugin disabled: %1").arg(pluginName));
    refreshPluginList();

    if (m_selectedPluginName == pluginName) {
        updatePluginDetails(pluginName);
    }
}

void PluginManagerPage::onPluginError(const QString& pluginName,
                                      const QString& error) {
    SLOG_ERROR(QString("PluginManagerPage: Plugin error [%1]: %2")
                   .arg(pluginName, error));

    auto* dialog = new ElaContentDialog(this);
    dialog->setWindowTitle(tr("Plugin Error"));
    auto* w = new QWidget(dialog);
    auto* l = new QVBoxLayout(w);
    l->addWidget(new ElaText(
        tr("Plugin '%1' encountered an error:\n%2").arg(pluginName, error), w));
    dialog->setCentralWidget(w);
    dialog->setLeftButtonText(QString());
    dialog->setMiddleButtonText(QString());
    dialog->setRightButtonText(tr("OK"));
    connect(dialog, &ElaContentDialog::rightButtonClicked, dialog,
            &ElaContentDialog::close);
    dialog->exec();
    dialog->deleteLater();

    refreshPluginList();
}

// ============================================================================
// Helper Methods
// ============================================================================

QIcon PluginManagerPage::getPluginIcon(const QString& pluginName) const {
    Q_UNUSED(pluginName);
    // TODO: Implement plugin icon retrieval
    return QIcon();
}

QString PluginManagerPage::getPluginStatusText(
    const QString& pluginName) const {
    if (!m_pluginManager) {
        return tr("Unknown");
    }

    bool isLoaded = m_pluginManager->isPluginLoaded(pluginName);
    bool isEnabled = m_pluginManager->isPluginEnabled(pluginName);
    bool hasErrors = !m_pluginManager->getPluginErrors(pluginName).isEmpty();

    if (hasErrors) {
        return tr("Error");
    } else if (isLoaded) {
        return tr("Loaded");
    } else if (isEnabled) {
        return tr("Enabled");
    } else {
        return tr("Disabled");
    }
}

QColor PluginManagerPage::getPluginStatusColor(
    const QString& pluginName) const {
    if (!m_pluginManager) {
        return Qt::gray;
    }

    bool isLoaded = m_pluginManager->isPluginLoaded(pluginName);
    bool hasErrors = !m_pluginManager->getPluginErrors(pluginName).isEmpty();

    if (hasErrors) {
        return Qt::red;
    } else if (isLoaded) {
        return Qt::green;
    } else {
        return Qt::gray;
    }
}

void PluginManagerPage::loadPluginSettings() {
    if (m_pluginManager) {
        m_pluginManager->loadSettings();
    }
}

void PluginManagerPage::savePluginSettings() {
    if (m_pluginManager) {
        m_pluginManager->saveSettings();
    }
}

void PluginManagerPage::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    ElaScrollPage::changeEvent(event);
}
