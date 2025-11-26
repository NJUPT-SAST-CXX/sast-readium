#include "PluginSettingsWidget.h"

#include <QEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSettings>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaTableView.h"
#include "ElaText.h"
#include "ElaToggleSwitch.h"

PluginSettingsWidget::PluginSettingsWidget(QWidget* parent)
    : QWidget(parent), m_mainLayout(nullptr) {
    setupUi();
    loadSettings();
}

PluginSettingsWidget::~PluginSettingsWidget() = default;

void PluginSettingsWidget::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(16);

    // Directories Section
    auto* dirArea = new ElaScrollPageArea(this);
    auto* dirLayout = new QVBoxLayout(dirArea);
    dirLayout->setContentsMargins(16, 12, 16, 12);

    auto* dirTitle = new ElaText(tr("Plugin Directories"), this);
    dirTitle->setTextPixelSize(14);
    dirLayout->addWidget(dirTitle);

    auto* addRow = new QHBoxLayout();
    m_directoryEdit = new ElaLineEdit(this);
    m_directoryEdit->setPlaceholderText(tr("Plugin directory path..."));
    addRow->addWidget(m_directoryEdit, 1);
    m_addDirBtn = new ElaPushButton(tr("Add"), this);
    addRow->addWidget(m_addDirBtn);
    m_removeDirBtn = new ElaPushButton(tr("Remove"), this);
    addRow->addWidget(m_removeDirBtn);
    dirLayout->addLayout(addRow);

    m_directoriesModel = new QStandardItemModel(this);
    m_directoriesModel->setHorizontalHeaderLabels({tr("Directory")});

    m_directoriesTable = new ElaTableView(this);
    m_directoriesTable->setModel(m_directoriesModel);
    m_directoriesTable->horizontalHeader()->setStretchLastSection(true);
    m_directoriesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_directoriesTable->setMaximumHeight(120);
    dirLayout->addWidget(m_directoriesTable);

    m_mainLayout->addWidget(dirArea);

    // Options Section
    auto* optArea = new ElaScrollPageArea(this);
    auto* optLayout = new QVBoxLayout(optArea);
    optLayout->setContentsMargins(16, 12, 16, 12);

    auto* optTitle = new ElaText(tr("Options"), this);
    optTitle->setTextPixelSize(14);
    optLayout->addWidget(optTitle);

    auto* hotRow = new QHBoxLayout();
    hotRow->addWidget(new ElaText(tr("Enable hot reloading"), this));
    hotRow->addStretch();
    m_hotReloadSwitch = new ElaToggleSwitch(this);
    hotRow->addWidget(m_hotReloadSwitch);
    optLayout->addLayout(hotRow);

    m_mainLayout->addWidget(optArea);

    // Plugins Section
    auto* plugArea = new ElaScrollPageArea(this);
    auto* plugLayout = new QVBoxLayout(plugArea);
    plugLayout->setContentsMargins(16, 12, 16, 12);

    auto* plugTitleRow = new QHBoxLayout();
    auto* plugTitle = new ElaText(tr("Installed Plugins"), this);
    plugTitle->setTextPixelSize(14);
    plugTitleRow->addWidget(plugTitle);
    plugTitleRow->addStretch();
    m_refreshBtn = new ElaPushButton(tr("Refresh"), this);
    plugTitleRow->addWidget(m_refreshBtn);
    plugLayout->addLayout(plugTitleRow);

    m_pluginsModel = new QStandardItemModel(this);
    m_pluginsModel->setHorizontalHeaderLabels(
        {tr("Enabled"), tr("Name"), tr("Version"), tr("Author")});

    m_pluginsTable = new ElaTableView(this);
    m_pluginsTable->setModel(m_pluginsModel);
    m_pluginsTable->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Fixed);
    m_pluginsTable->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);
    m_pluginsTable->setColumnWidth(0, 60);
    m_pluginsTable->setColumnWidth(2, 80);
    m_pluginsTable->setColumnWidth(3, 120);
    m_pluginsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pluginsTable->setMinimumHeight(200);
    plugLayout->addWidget(m_pluginsTable);

    m_pluginInfoText = new ElaText(tr("Select a plugin to view details"), this);
    m_pluginInfoText->setTextPixelSize(11);
    plugLayout->addWidget(m_pluginInfoText);

    m_mainLayout->addWidget(plugArea);
    m_mainLayout->addStretch();

    connect(m_addDirBtn, &ElaPushButton::clicked, this,
            &PluginSettingsWidget::onAddDirectory);
    connect(m_removeDirBtn, &ElaPushButton::clicked, this,
            &PluginSettingsWidget::onRemoveDirectory);
    connect(m_hotReloadSwitch, &ElaToggleSwitch::toggled, this,
            &PluginSettingsWidget::onHotReloadToggled);
    connect(m_pluginsTable, &ElaTableView::clicked, this,
            [this](const QModelIndex& index) {
                onPluginSelected(index.row(), index.column());
            });
    connect(m_refreshBtn, &ElaPushButton::clicked, this,
            &PluginSettingsWidget::refreshPluginList);
}

void PluginSettingsWidget::loadSettings() {
    QSettings s("SAST", "Readium");
    s.beginGroup("Plugins");

    QStringList dirs = s.value("directories").toStringList();
    m_directoriesModel->setRowCount(dirs.size());
    for (int i = 0; i < dirs.size(); ++i) {
        m_directoriesModel->setItem(i, 0, new QStandardItem(dirs[i]));
    }

    m_hotReloadSwitch->setIsToggled(s.value("hot_reload", false).toBool());
    s.endGroup();

    populatePluginTable();
}

void PluginSettingsWidget::saveSettings() {
    QSettings s("SAST", "Readium");
    s.beginGroup("Plugins");

    QStringList dirs;
    for (int i = 0; i < m_directoriesModel->rowCount(); ++i) {
        if (auto* item = m_directoriesModel->item(i, 0)) {
            dirs << item->text();
        }
    }
    s.setValue("directories", dirs);
    s.setValue("hot_reload", m_hotReloadSwitch->getIsToggled());

    s.endGroup();
    emit settingsChanged();
}

void PluginSettingsWidget::resetToDefaults() {
    m_directoriesModel->setRowCount(0);
    m_hotReloadSwitch->setIsToggled(false);
    emit settingsChanged();
}

void PluginSettingsWidget::refreshPluginList() { populatePluginTable(); }

void PluginSettingsWidget::onAddDirectory() {
    QString dir = m_directoryEdit->text();
    if (dir.isEmpty()) {
        dir = QFileDialog::getExistingDirectory(this,
                                                tr("Select Plugin Directory"));
    }
    if (!dir.isEmpty()) {
        int row = m_directoriesModel->rowCount();
        m_directoriesModel->insertRow(row);
        m_directoriesModel->setItem(row, 0, new QStandardItem(dir));
        m_directoryEdit->clear();
        emit settingsChanged();
    }
}

void PluginSettingsWidget::onRemoveDirectory() {
    int row = m_directoriesTable->currentIndex().row();
    if (row >= 0) {
        m_directoriesModel->removeRow(row);
        emit settingsChanged();
    }
}

void PluginSettingsWidget::onHotReloadToggled(bool enabled) {
    Q_UNUSED(enabled)
    emit settingsChanged();
}

void PluginSettingsWidget::onPluginToggled(int row, int column) {
    Q_UNUSED(row)
    Q_UNUSED(column)
    emit settingsChanged();
}

void PluginSettingsWidget::onPluginSelected(int row, int column) {
    Q_UNUSED(column)
    if (row >= 0 && row < m_pluginsModel->rowCount()) {
        QString name = m_pluginsModel->item(row, 1)->text();
        QString ver = m_pluginsModel->item(row, 2)->text();
        QString author = m_pluginsModel->item(row, 3)->text();
        m_pluginInfoText->setText(
            tr("Plugin: %1 v%2 by %3").arg(name, ver, author));
    }
}

void PluginSettingsWidget::populatePluginTable() {
    m_pluginsModel->setRowCount(0);
    // Placeholder - actual implementation would query PluginManager
}

void PluginSettingsWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange)
        retranslateUi();
    QWidget::changeEvent(event);
}

void PluginSettingsWidget::retranslateUi() {}
